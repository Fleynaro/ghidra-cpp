export module ghidra.service.pe_loader;

import std;
import ghidra.core;
import pe_loader;

export namespace ghidra::services::pe_loader {

namespace core = ghidra::core;

/// Owns one immutable checked PE image for memory/provider consumers.
class PeImage final : public core::contracts::IMemoryProvider {
public:
    /// Takes ownership of one successfully parsed PE image and its core regions.
    PeImage(std::shared_ptr<const pe::LoadedPeImage> image, std::vector<core::MemoryRegion> regions)
        : image_(std::move(image)), regions_(std::move(regions)) {}

    /// Reads exactly `size` bytes from the preferred virtual image address.
    [[nodiscard]] core::Result<core::Bytes> read(core::Address address, std::size_t size) const override {
        if (!image_ || address.space.name() != "ram")
            return std::unexpected(core::Error::make(core::DiagnosticCode::invalid_argument,
                                                     "PE memory reads require the ram address space"));
        auto bytes = image_->read_memory(address.offset, size);
        if (!bytes)
            return std::unexpected(core::Error::make(core::DiagnosticCode::io_failure, bytes.error().message));
        return core::Bytes{std::move(*bytes)};
    }

    /// Returns the generic mapped region containing an address.
    [[nodiscard]] std::optional<core::MemoryRegion> region_at(core::Address address) const override {
        const auto iterator =
            std::ranges::find_if(regions_, [&](const auto& region) { return region.range.contains(address); });
        return iterator == regions_.end() ? std::nullopt : std::optional{*iterator};
    }

    /// Returns immutable generic memory-region snapshots.
    [[nodiscard]] std::vector<core::MemoryRegion> regions() const override {
        return regions_;
    }

    /// Returns the checked entry-point VA used to seed analysis.
    [[nodiscard]] core::Result<core::Address> entry_point() const {
        const auto address = image_->entry_point_va();
        if (!address)
            return std::unexpected(core::Error::make(core::DiagnosticCode::parse_failure, address.error().message));
        return core::Address{core::AddressSpaceId{"ram"}, *address};
    }

    /// Returns whether a complete range is executable according to PE permissions.
    [[nodiscard]] bool is_executable(core::Address address, std::uint64_t size = 1) const noexcept {
        return address.space.name() == "ram" && image_->is_executable(address.offset, size);
    }

    /// Returns the underlying read-only image for service-local PE metadata mapping.
    [[nodiscard]] const pe::LoadedPeImage& parsed_image() const noexcept {
        return *image_;
    }

private:
    std::shared_ptr<const pe::LoadedPeImage> image_;
    std::vector<core::MemoryRegion> regions_;
};

/// Adapts the checked PE parser into the core loader contract and immutable image provider.
class PeLoaderService final : public core::contracts::IPELoader {
public:
    /// Constructs a stateless loader; parsed images are owned by returned handles.
    PeLoaderService() = default;

    /// Parses an artifact and returns serializable generic load facts.
    [[nodiscard]] core::Result<core::contracts::PeLoadResult>
    load(const core::BinaryArtifact& artifact, const core::contracts::LoadOptions& options) override {
        auto image = parse(artifact.locator, options);
        if (!image)
            return std::unexpected(image.error());
        return make_result(artifact, **image);
    }

    /// Opens an artifact as an immutable memory provider for decoder/decompiler services.
    [[nodiscard]] core::Result<std::shared_ptr<PeImage>> open(const std::filesystem::path& path,
                                                              const core::contracts::LoadOptions& options = {}) const {
        auto image = parse(path, options);
        if (!image)
            return std::unexpected(image.error());
        auto regions = make_regions(**image);
        return std::make_shared<PeImage>(std::move(*image), std::move(regions));
    }

private:
    /// Parses one file through the existing checked PE implementation.
    [[nodiscard]] static core::Result<std::shared_ptr<pe::LoadedPeImage>>
    parse(const std::filesystem::path& path, const core::contracts::LoadOptions& options) {
        pe::LoadOptions parser_options;
        parser_options.strict = options.strict;
        parser_options.parse_directories = options.parse_directories;
        parser_options.maximum_image_size = options.maximum_image_size;
        auto image = pe::PeLoader::load_file(path, parser_options);
        if (!image)
            return std::unexpected(core::Error::make(core::DiagnosticCode::parse_failure, image.error().message,
                                                     "Verify the executable path and PE format."));
        return std::make_shared<pe::LoadedPeImage>(std::move(*image));
    }

    /// Converts PE memory blocks to address-space-aware core regions.
    [[nodiscard]] static std::vector<core::MemoryRegion> make_regions(const pe::LoadedPeImage& image) {
        std::vector<core::MemoryRegion> regions;
        std::size_t ordinal{};
        for (const auto& source : image.memory_regions()) {
            if (source.size == 0)
                continue;
            const core::Address start{core::AddressSpaceId{"ram"}, source.start};
            const core::Address end{core::AddressSpaceId{"ram"}, source.start + source.size - 1};
            regions.push_back(core::MemoryRegion{
                core::EntityId{core::make_identifier("memory", ordinal++)}, core::AddressRange{start, end}, source.name,
                core::MemoryPermissions{source.readable, source.writable, source.executable}, source.initialized,
                source.headers, true, std::nullopt, "pe_loader"});
        }
        return regions;
    }

    /// Builds the contract DTO while retaining all parser details behind the service boundary.
    [[nodiscard]] static core::Result<core::contracts::PeLoadResult> make_result(core::BinaryArtifact artifact,
                                                                                 const pe::LoadedPeImage& image) {
        core::contracts::PeLoadResult result;
        result.artifact = std::move(artifact);
        result.memory_regions = make_regions(image);
        result.details.machine = std::string(pe::machine_name(image.coff_header().machine));
        result.details.image_base = image.optional_header().image_base;
        result.details.entry_point_rva = image.optional_header().address_of_entry_point;
        result.details.image_size = image.image_size();
        for (const auto& descriptor : image.imports())
            for (const auto& symbol : descriptor.symbols)
                result.details.imports.push_back(descriptor.dll_name + "!" + symbol.name);
        for (const auto& symbol : image.exported_symbols())
            if (symbol.name)
                result.details.exports.push_back(*symbol.name);
        result.architecture.language_id =
            result.details.machine == "x86-64" ? "x86:LE:64:default" : "x86:LE:32:default";
        result.architecture.compiler_spec_id = "windows";
        result.architecture.architecture_id = result.details.machine;
        result.architecture.pointer_size = image.optional_header().pe32_plus ? 8 : 4;
        result.architecture.spaces.push_back(
            core::AddressSpaceDescriptor{core::AddressSpaceId{"ram"}, core::AddressSpaceKind::ram, 64, 1,
                                         result.architecture.pointer_size, false, false, true});
        result.status =
            image.is_partial() ? core::contracts::PeParseStatus::partial : core::contracts::PeParseStatus::complete;
        return result;
    }
};

} // namespace ghidra::services::pe_loader
