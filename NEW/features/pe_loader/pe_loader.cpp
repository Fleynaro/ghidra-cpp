module;

#include <algorithm>
#include <array>
#include <cstring>
#include <expected>
#include <fstream>
#include <functional>
#include <limits>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>

module pe_loader;

// Ported/adapted from Ghidra:
// ../../../Ghidra/Features/Base/src/main/java/ghidra/app/util/opinion/PeLoader.java
// ../../../Ghidra/Features/Base/src/main/java/ghidra/app/util/opinion/AbstractPeDebugLoader.java
// ../../../Ghidra/Features/Base/src/main/java/ghidra/app/util/bin/format/pe/
// ../../../Ghidra/Features/Base/src/main/java/ghidra/app/util/bin/format/mz/DOSHeader.java
// Directory parsers preserve PE bytes and metadata but do not create Ghidra Program/DB objects.

namespace pe {

/// Stores all owned bytes and decoded values behind the public image contract.
struct LoadedPeImage::Storage {
    std::vector<Byte> file;
    DosHeader dos;
    std::optional<RichHeader> rich;
    CoffHeader coff;
    OptionalHeader optional;
    std::vector<Section> sections;
    std::vector<DataDirectory> directories;
    std::optional<ArchitectureDirectory> architecture_directory;
    std::optional<GlobalPointerDirectory> global_pointer_directory;
    std::vector<MemoryRegion> memory_regions;
    std::vector<Byte> mapped_image;
    std::vector<ImportDescriptor> imports;
    std::vector<IatEntry> iat_entries;
    std::optional<ExportDirectory> exports;
    std::vector<ExportedSymbol> exported_symbols;
    std::vector<RelocationBlock> relocations;
    std::vector<DebugDirectoryEntry> debug_entries;
    std::vector<RuntimeFunction> exception_functions;
    std::optional<TlsDirectory> tls;
    std::optional<LoadConfigDirectory> load_config;
    std::optional<ResourceDirectory> resources;
    std::vector<SecurityCertificate> certificates;
    std::vector<BoundImport> bound_imports;
    std::vector<DelayImportDescriptor> delay_imports;
    std::optional<ClrHeader> clr_header;
    std::vector<CoffSymbol> coff_symbols;
    ParseStatus parse_status{ParseStatus::complete};
    std::vector<ParseError> parse_diagnostics;
};

namespace {

/// Constructs a parser error without throwing from attacker-controlled input handling.
template <typename T>
[[nodiscard]] std::expected<T, ParseError> parse_failure(ParseErrorCode code, FileOffset offset, std::string message) {
    return std::unexpected(ParseError{code, offset, std::move(message)});
}

/// Returns true when adding two unsigned values would overflow.
[[nodiscard]] bool add_overflow(std::uint64_t left, std::uint64_t right) noexcept {
    return right > std::numeric_limits<std::uint64_t>::max() - left;
}

/// Returns true when multiplying two unsigned values would overflow.
[[nodiscard]] bool multiply_overflow(std::uint64_t left, std::uint64_t right) noexcept {
    return left != 0 && right > std::numeric_limits<std::uint64_t>::max() / left;
}

/// Rounds a PE size or RVA upward without allowing the alignment calculation to wrap.
// Related source: ../../../Ghidra/Features/Base/src/main/java/ghidra/app/util/bin/format/pe/SectionHeader.java
[[nodiscard]] std::optional<std::uint64_t> align_up_checked(std::uint64_t value, std::uint32_t alignment) noexcept {
    if (alignment == 0 || value > std::numeric_limits<std::uint64_t>::max() - (alignment - 1U)) {
        return std::nullopt;
    }
    const auto rounded = value + alignment - 1U;
    return rounded - (rounded % alignment);
}

/// Converts a bounded file offset to size_t after a range check.
[[nodiscard]] bool valid_range(FileOffset offset, std::uint64_t size, std::size_t extent) noexcept {
    return offset <= extent && size <= extent - static_cast<std::size_t>(offset);
}

/// Reads little-endian integers from a byte span with checked bounds.
class Reader {
public:
    /// Creates a non-owning reader over immutable PE file bytes.
    explicit Reader(std::span<const Byte> bytes) noexcept : bytes_(bytes) {}

    /// Checks that a complete range exists in the input span.
    [[nodiscard]] std::expected<void, ParseError> require(FileOffset offset, std::uint64_t size,
                                                          std::string_view what) const {
        if (!valid_range(offset, size, bytes_.size())) {
            return std::unexpected(
                ParseError{ParseErrorCode::truncated, offset, std::string("Truncated ") + std::string(what)});
        }
        return {};
    }

    /// Reads one byte at a checked file offset.
    [[nodiscard]] std::expected<Byte, ParseError> u8(FileOffset offset, std::string_view what) const {
        auto checked = require(offset, 1, what);
        if (!checked) {
            return std::unexpected(checked.error());
        }
        return bytes_[static_cast<std::size_t>(offset)];
    }

    /// Reads a little-endian 16-bit integer.
    [[nodiscard]] std::expected<std::uint16_t, ParseError> u16(FileOffset offset, std::string_view what) const {
        auto checked = require(offset, 2, what);
        if (!checked) {
            return std::unexpected(checked.error());
        }
        const auto index = static_cast<std::size_t>(offset);
        return static_cast<std::uint16_t>(bytes_[index]) | (static_cast<std::uint16_t>(bytes_[index + 1]) << 8U);
    }

    /// Reads a little-endian 32-bit integer.
    [[nodiscard]] std::expected<std::uint32_t, ParseError> u32(FileOffset offset, std::string_view what) const {
        auto checked = require(offset, 4, what);
        if (!checked) {
            return std::unexpected(checked.error());
        }
        const auto index = static_cast<std::size_t>(offset);
        return static_cast<std::uint32_t>(bytes_[index]) | (static_cast<std::uint32_t>(bytes_[index + 1]) << 8U) |
               (static_cast<std::uint32_t>(bytes_[index + 2]) << 16U) |
               (static_cast<std::uint32_t>(bytes_[index + 3]) << 24U);
    }

    /// Reads a little-endian 64-bit integer.
    [[nodiscard]] std::expected<std::uint64_t, ParseError> u64(FileOffset offset, std::string_view what) const {
        auto checked = require(offset, 8, what);
        if (!checked) {
            return std::unexpected(checked.error());
        }
        const auto index = static_cast<std::size_t>(offset);
        std::uint64_t value = 0;
        for (std::size_t byte_index = 0; byte_index < 8; ++byte_index) {
            value |= static_cast<std::uint64_t>(bytes_[index + byte_index]) << (byte_index * 8U);
        }
        return value;
    }

    /// Returns a checked immutable file subspan.
    [[nodiscard]] std::expected<std::span<const Byte>, ParseError> span(FileOffset offset, std::uint64_t size,
                                                                        std::string_view what) const {
        auto checked = require(offset, size, what);
        if (!checked) {
            return std::unexpected(checked.error());
        }
        return bytes_.subspan(static_cast<std::size_t>(offset), static_cast<std::size_t>(size));
    }

    /// Returns the complete source byte span.
    [[nodiscard]] std::span<const Byte> bytes() const noexcept {
        return bytes_;
    }

private:
    std::span<const Byte> bytes_;
};

/// Converts a raw machine value to the public architecture enum.
[[nodiscard]] Machine decode_machine(std::uint16_t raw) noexcept {
    switch (raw) {
        case static_cast<std::uint16_t>(Machine::i386):
            return Machine::i386;
        case static_cast<std::uint16_t>(Machine::amd64):
            return Machine::amd64;
        case static_cast<std::uint16_t>(Machine::arm):
            return Machine::arm;
        case static_cast<std::uint16_t>(Machine::armnt):
            return Machine::armnt;
        case static_cast<std::uint16_t>(Machine::thumb):
            return Machine::thumb;
        case static_cast<std::uint16_t>(Machine::arm64):
            return Machine::arm64;
        case static_cast<std::uint16_t>(Machine::arm64ec):
            return Machine::arm64ec;
        case static_cast<std::uint16_t>(Machine::arm64x):
            return Machine::arm64x;
        case static_cast<std::uint16_t>(Machine::ia64):
            return Machine::ia64;
        case static_cast<std::uint16_t>(Machine::mips16):
            return Machine::mips16;
        case static_cast<std::uint16_t>(Machine::mipsfpu):
            return Machine::mipsfpu;
        case static_cast<std::uint16_t>(Machine::mipsfpu16):
            return Machine::mipsfpu16;
        case static_cast<std::uint16_t>(Machine::powerpc):
            return Machine::powerpc;
        case static_cast<std::uint16_t>(Machine::powerpcfp):
            return Machine::powerpcfp;
        case static_cast<std::uint16_t>(Machine::sh3):
            return Machine::sh3;
        case static_cast<std::uint16_t>(Machine::sh4):
            return Machine::sh4;
        case static_cast<std::uint16_t>(Machine::ebc):
            return Machine::ebc;
        case static_cast<std::uint16_t>(Machine::riscv32):
            return Machine::riscv32;
        case static_cast<std::uint16_t>(Machine::riscv64):
            return Machine::riscv64;
        case static_cast<std::uint16_t>(Machine::riscv128):
            return Machine::riscv128;
        case static_cast<std::uint16_t>(Machine::loongarch32):
            return Machine::loongarch32;
        case static_cast<std::uint16_t>(Machine::loongarch64):
            return Machine::loongarch64;
        default:
            return Machine::unknown;
    }
}

/// Converts a bounded byte sequence to a PE section name.
[[nodiscard]] std::string section_name(std::span<const Byte> bytes) {
    const auto end = std::find(bytes.begin(), bytes.end(), Byte{0});
    return std::string(reinterpret_cast<const char*>(bytes.data()),
                       static_cast<std::size_t>(std::distance(bytes.begin(), end)));
}

/// Converts a null-terminated ASCII/UTF-8 string at a file offset.
[[nodiscard]] std::expected<std::string, ParseError> read_file_string(const Reader& reader, FileOffset offset,
                                                                      std::uint64_t maximum, std::string_view what) {
    auto checked = reader.require(offset, 0, what);
    if (!checked) {
        return std::unexpected(checked.error());
    }
    const auto bytes = reader.bytes();
    if (offset > bytes.size() || maximum > bytes.size() - static_cast<std::size_t>(offset)) {
        maximum = bytes.size() - static_cast<std::size_t>(offset);
    }
    const auto begin = bytes.begin() + static_cast<std::ptrdiff_t>(offset);
    const auto end = begin + static_cast<std::ptrdiff_t>(maximum);
    const auto nul = std::find(begin, end, Byte{0});
    if (nul == end) {
        return parse_failure<std::string>(ParseErrorCode::truncated, offset,
                                          std::string("Unterminated ") + std::string(what));
    }
    return std::string(reinterpret_cast<const char*>(&*begin), static_cast<std::size_t>(std::distance(begin, nul)));
}

/// Encodes one Unicode scalar as UTF-8 for resource names.
void append_utf8(std::string& output, std::uint32_t code_point) {
    if (code_point <= 0x7f) {
        output.push_back(static_cast<char>(code_point));
    } else if (code_point <= 0x7ff) {
        output.push_back(static_cast<char>(0xc0U | (code_point >> 6U)));
        output.push_back(static_cast<char>(0x80U | (code_point & 0x3fU)));
    } else if (code_point <= 0xffff) {
        output.push_back(static_cast<char>(0xe0U | (code_point >> 12U)));
        output.push_back(static_cast<char>(0x80U | ((code_point >> 6U) & 0x3fU)));
        output.push_back(static_cast<char>(0x80U | (code_point & 0x3fU)));
    } else {
        output.push_back(static_cast<char>(0xf0U | (code_point >> 18U)));
        output.push_back(static_cast<char>(0x80U | ((code_point >> 12U) & 0x3fU)));
        output.push_back(static_cast<char>(0x80U | ((code_point >> 6U) & 0x3fU)));
        output.push_back(static_cast<char>(0x80U | (code_point & 0x3fU)));
    }
}

/// Converts UTF-16 code units from a resource directory to UTF-8.
[[nodiscard]] std::string utf16_to_utf8(std::span<const std::uint16_t> code_units) {
    std::string output;
    for (std::size_t index = 0; index < code_units.size(); ++index) {
        std::uint32_t code_point = code_units[index];
        if (code_point >= 0xd800U && code_point <= 0xdbffU && index + 1 < code_units.size() &&
            code_units[index + 1] >= 0xdc00U && code_units[index + 1] <= 0xdfffU) {
            code_point = 0x10000U + ((code_point - 0xd800U) << 10U) + (code_units[++index] - 0xdc00U);
        } else if (code_point >= 0xd800U && code_point <= 0xdfffU) {
            code_point = 0xfffdU;
        }
        append_utf8(output, code_point);
    }
    return output;
}

/// Parses PE headers, maps sections, and decodes supported data directories.
class Parser {
public:
    /// Creates a parser over caller bytes while retaining no caller-owned lifetime.
    Parser(std::span<const Byte> bytes, const LoadOptions& options) : reader_(bytes), options_(options) {}

    /// Runs all validation and returns an owning loaded image on success.
    [[nodiscard]] std::expected<LoadedPeImage, ParseError> run();

private:
    /// Parses DOS, NT, COFF, optional-header, directory, and section metadata.
    [[nodiscard]] std::expected<void, ParseError> parse_headers();

    // Related source: ../../../Ghidra/Features/Base/src/main/java/ghidra/app/util/bin/format/pe/RichHeader.java
    /// Decodes the optional Rich header between DOS and NT headers.
    [[nodiscard]] std::expected<void, ParseError> parse_rich_header();

    /// Builds the zero-filled image and section/header memory regions.
    [[nodiscard]] std::expected<void, ParseError> map_image();

    /// Parses every supported PE directory present in the optional header.
    [[nodiscard]] std::expected<void, ParseError> parse_directories();

    // Related source:
    // ../../../Ghidra/Features/Base/src/main/java/ghidra/app/util/bin/format/pe/ArchitectureDataDirectory.java
    /// Parses the bounded architecture-specific ASCII directory payload.
    [[nodiscard]] std::expected<void, ParseError> parse_architecture(const DataDirectory& directory);

    // Related source:
    // ../../../Ghidra/Features/Base/src/main/java/ghidra/app/util/bin/format/pe/GlobalPointerDataDirectory.java
    /// Validates and records the global-pointer directory RVA.
    [[nodiscard]] std::expected<void, ParseError> parse_global_pointer(const DataDirectory& directory);

    // Related source:
    // ../../../Ghidra/Features/Base/src/main/java/ghidra/app/util/bin/format/pe/ExportDataDirectory.java
    /// Parses the export directory and its name/function tables.
    [[nodiscard]] std::expected<void, ParseError> parse_exports(const DataDirectory& directory);

    // Related sources:
    // ../../../Ghidra/Features/Base/src/main/java/ghidra/app/util/bin/format/pe/ImportDataDirectory.java,
    // ../../../Ghidra/Features/Base/src/main/java/ghidra/app/util/bin/format/pe/ImportDescriptor.java,
    // ../../../Ghidra/Features/Base/src/main/java/ghidra/app/util/bin/format/pe/ThunkData.java,
    // ../../../Ghidra/Features/Base/src/main/java/ghidra/app/util/bin/format/pe/ImportByName.java
    /// Parses import descriptors, import-by-name records, and thunk slots.
    [[nodiscard]] std::expected<void, ParseError> parse_imports(const DataDirectory& directory);

    /// Parses one INT/IAT pair and appends decoded imported symbols.
    [[nodiscard]] std::expected<void, ParseError> parse_thunks(std::vector<ImportedSymbol>& symbols, Rva int_rva,
                                                               Rva iat_rva, std::uint32_t maximum_entries,
                                                               bool thunk_values_are_va = false);

    /// Parses the raw import-address-table directory slots.
    [[nodiscard]] std::expected<void, ParseError> parse_iat(const DataDirectory& directory);

    // Related source:
    // ../../../Ghidra/Features/Base/src/main/java/ghidra/app/util/bin/format/pe/BaseRelocationDataDirectory.java
    /// Parses base-relocation blocks and validates every target RVA.
    [[nodiscard]] std::expected<void, ParseError> parse_relocations(const DataDirectory& directory);

    // Related sources:
    // ../../../Ghidra/Features/Base/src/main/java/ghidra/app/util/bin/format/pe/DebugDataDirectory.java,
    // ../../../Ghidra/Features/Base/src/main/java/ghidra/app/util/bin/format/pe/debug/DebugDirectoryParser.java,
    // ../../../Ghidra/Features/Base/src/main/java/ghidra/app/util/bin/format/pe/pdb/PdbInfoCodeView.java
    /// Parses debug directory entries and CodeView payloads.
    [[nodiscard]] std::expected<void, ParseError> parse_debug(const DataDirectory& directory);

    // Related source:
    // ../../../Ghidra/Features/Base/src/main/java/ghidra/app/util/bin/format/pe/ExceptionDataDirectory.java
    /// Parses exception runtime-function records using the target machine width.
    [[nodiscard]] std::expected<void, ParseError> parse_exceptions(const DataDirectory& directory);

    // Related sources: ../../../Ghidra/Features/Base/src/main/java/ghidra/app/util/bin/format/pe/TLSDataDirectory.java
    // and ../../../Ghidra/Features/Base/src/main/java/ghidra/app/util/bin/format/pe/TLSDirectory.java
    /// Parses the architecture-dependent TLS directory and callback array.
    [[nodiscard]] std::expected<void, ParseError> parse_tls(const DataDirectory& directory);

    // Related sources:
    // ../../../Ghidra/Features/Base/src/main/java/ghidra/app/util/bin/format/pe/LoadConfigDataDirectory.java and
    // ../../../Ghidra/Features/Base/src/main/java/ghidra/app/util/bin/format/pe/LoadConfigDirectory.java
    /// Parses the versioned load-config directory fields available in this image.
    [[nodiscard]] std::expected<void, ParseError> parse_load_config(const DataDirectory& directory);

    // Related sources:
    // ../../../Ghidra/Features/Base/src/main/java/ghidra/app/util/bin/format/pe/ResourceDataDirectory.java,
    // ../../../Ghidra/Features/Base/src/main/java/ghidra/app/util/bin/format/pe/resource/ResourceDirectory.java,
    // ../../../Ghidra/Features/Base/src/main/java/ghidra/app/util/bin/format/pe/resource/ResourceDataEntry.java
    /// Traverses the resource tree and decodes all resource leaves.
    [[nodiscard]] std::expected<void, ParseError> parse_resources(const DataDirectory& directory);

    // Related sources:
    // ../../../Ghidra/Features/Base/src/main/java/ghidra/app/util/bin/format/pe/SecurityDataDirectory.java and
    // ../../../Ghidra/Features/Base/src/main/java/ghidra/app/util/bin/format/pe/SecurityCertificate.java
    /// Parses file-offset Authenticode certificates.
    [[nodiscard]] std::expected<void, ParseError> parse_security(const DataDirectory& directory);

    // Related source:
    // ../../../Ghidra/Features/Base/src/main/java/ghidra/app/util/bin/format/pe/BoundImportDataDirectory.java
    /// Parses bound import descriptors and their forwarder references.
    [[nodiscard]] std::expected<void, ParseError> parse_bound_imports(const DataDirectory& directory);

    // Related source:
    // ../../../Ghidra/Features/Base/src/main/java/ghidra/app/util/bin/format/pe/DelayImportDataDirectory.java
    /// Parses delay-import descriptors and their thunk tables.
    [[nodiscard]] std::expected<void, ParseError> parse_delay_imports(const DataDirectory& directory);

    // Related sources:
    // ../../../Ghidra/Features/Base/src/main/java/ghidra/app/util/bin/format/pe/COMDescriptorDataDirectory.java and
    // ../../../Ghidra/Features/Base/src/main/java/ghidra/app/util/bin/format/pe/ImageCor20Header.java
    /// Parses the optional managed COM descriptor.
    [[nodiscard]] std::expected<void, ParseError> parse_clr(const DataDirectory& directory);

    /// Parses the optional COFF symbol/string tables.
    [[nodiscard]] std::expected<void, ParseError> parse_coff_symbols();

    /// Resolves an RVA to raw bytes during parsing.
    [[nodiscard]] std::optional<FileOffset> raw_offset_for_rva(Rva rva) const;

    /// Resolves a virtual address to an RVA during parsing.
    [[nodiscard]] std::optional<Rva> rva_for_va(Va va) const;

    /// Reads a mapped byte during directory parsing.
    [[nodiscard]] std::optional<Byte> mapped_byte(Rva rva) const;

    /// Reads a little-endian integer from mapped image bytes.
    [[nodiscard]] std::optional<std::uint16_t> mapped_u16(Rva rva) const;

    /// Reads a little-endian integer from mapped image bytes.
    [[nodiscard]] std::optional<std::uint32_t> mapped_u32(Rva rva) const;

    /// Reads a little-endian integer from mapped image bytes.
    [[nodiscard]] std::optional<std::uint64_t> mapped_u64(Rva rva) const;

    /// Reads a null-terminated mapped string with a bounded scan.
    [[nodiscard]] std::expected<std::string, ParseError> mapped_string(Rva rva, std::uint64_t maximum,
                                                                       std::string_view what) const;

    /// Gets a directory by index when the optional header declares it.
    [[nodiscard]] const DataDirectory* directory(DirectoryIndex index) const noexcept;

    /// Records a tolerated parsing error and marks the result as partial.
    void record_partial(ParseError error);

    Reader reader_;
    LoadOptions options_;
    LoadedPeImage::Storage storage_;
    FileOffset nt_offset_{};
};

} // namespace

/// Returns the conventional architecture name for a PE machine value.
std::string_view machine_name(Machine machine) noexcept {
    switch (machine) {
        case Machine::i386:
            return "i386";
        case Machine::amd64:
            return "AMD64 / x86-64";
        case Machine::arm:
            return "ARM";
        case Machine::armnt:
            return "ARMNT";
        case Machine::thumb:
            return "Thumb";
        case Machine::arm64:
            return "ARM64";
        case Machine::arm64ec:
            return "ARM64EC";
        case Machine::arm64x:
            return "ARM64X";
        case Machine::ia64:
            return "IA64";
        case Machine::mips16:
            return "MIPS16";
        case Machine::mipsfpu:
            return "MIPS FPU";
        case Machine::mipsfpu16:
            return "MIPS FPU16";
        case Machine::powerpc:
            return "PowerPC";
        case Machine::powerpcfp:
            return "PowerPC FP";
        case Machine::sh3:
            return "SuperH3";
        case Machine::sh4:
            return "SuperH4";
        case Machine::ebc:
            return "EBC";
        case Machine::riscv32:
            return "RISC-V 32";
        case Machine::riscv64:
            return "RISC-V 64";
        case Machine::riscv128:
            return "RISC-V 128";
        case Machine::loongarch32:
            return "LoongArch 32";
        case Machine::loongarch64:
            return "LoongArch 64";
        default:
            return "Unknown";
    }
}

/// Returns the Windows symbolic name for a directory index.
std::string_view directory_name(DirectoryIndex index) noexcept {
    switch (index) {
        case DirectoryIndex::export_table:
            return "IMAGE_DIRECTORY_ENTRY_EXPORT";
        case DirectoryIndex::import_table:
            return "IMAGE_DIRECTORY_ENTRY_IMPORT";
        case DirectoryIndex::resource_table:
            return "IMAGE_DIRECTORY_ENTRY_RESOURCE";
        case DirectoryIndex::exception_table:
            return "IMAGE_DIRECTORY_ENTRY_EXCEPTION";
        case DirectoryIndex::security:
            return "IMAGE_DIRECTORY_ENTRY_SECURITY";
        case DirectoryIndex::base_relocation_table:
            return "IMAGE_DIRECTORY_ENTRY_BASERELOC";
        case DirectoryIndex::debug:
            return "IMAGE_DIRECTORY_ENTRY_DEBUG";
        case DirectoryIndex::architecture:
            return "IMAGE_DIRECTORY_ENTRY_ARCHITECTURE";
        case DirectoryIndex::global_pointer:
            return "IMAGE_DIRECTORY_ENTRY_GLOBALPTR";
        case DirectoryIndex::tls_table:
            return "IMAGE_DIRECTORY_ENTRY_TLS";
        case DirectoryIndex::load_config:
            return "IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG";
        case DirectoryIndex::bound_import:
            return "IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT";
        case DirectoryIndex::import_address_table:
            return "IMAGE_DIRECTORY_ENTRY_IAT";
        case DirectoryIndex::delay_import:
            return "IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT";
        case DirectoryIndex::clr_runtime_header:
            return "IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR";
        default:
            return "RESERVED";
    }
}

/// Creates an empty storage object for moved-from/default construction safety.
LoadedPeImage::LoadedPeImage() : storage_(std::make_unique<Storage>()) {}

/// Transfers parsed image storage from another image.
LoadedPeImage::LoadedPeImage(LoadedPeImage&& other) noexcept : storage_(std::move(other.storage_)) {
    if (!other.storage_) {
        other.storage_ = std::make_unique<Storage>();
    }
}

/// Transfers parsed image storage from another image.
LoadedPeImage& LoadedPeImage::operator=(LoadedPeImage&& other) noexcept {
    if (this != &other) {
        storage_ = std::move(other.storage_);
        if (!other.storage_) {
            other.storage_ = std::make_unique<Storage>();
        }
    }
    return *this;
}

/// Releases parsed image storage.
LoadedPeImage::~LoadedPeImage() = default;

/// Wraps parser-owned storage in the public image object.
LoadedPeImage::LoadedPeImage(std::unique_ptr<Storage> storage)
    : storage_(storage ? std::move(storage) : std::make_unique<Storage>()) {}

/// Returns the decoded DOS header.
const DosHeader& LoadedPeImage::dos_header() const noexcept {
    return storage_->dos;
}

/// Returns the optional Rich header.
const std::optional<RichHeader>& LoadedPeImage::rich_header() const noexcept {
    return storage_->rich;
}

/// Returns the decoded COFF/file header.
const CoffHeader& LoadedPeImage::coff_header() const noexcept {
    return storage_->coff;
}

/// Returns the decoded optional header.
const OptionalHeader& LoadedPeImage::optional_header() const noexcept {
    return storage_->optional;
}

/// Returns sections in file order.
const std::vector<Section>& LoadedPeImage::sections() const noexcept {
    return storage_->sections;
}

/// Returns optional-header data directories.
const std::vector<DataDirectory>& LoadedPeImage::data_directories() const noexcept {
    return storage_->directories;
}

/// Returns the parsed architecture-specific directory payload when present.
const std::optional<ArchitectureDirectory>& LoadedPeImage::architecture_directory() const noexcept {
    return storage_->architecture_directory;
}

/// Returns the validated global-pointer directory when present.
const std::optional<GlobalPointerDirectory>& LoadedPeImage::global_pointer_directory() const noexcept {
    return storage_->global_pointer_directory;
}

/// Returns mapped memory regions.
const std::vector<MemoryRegion>& LoadedPeImage::memory_regions() const noexcept {
    return storage_->memory_regions;
}

/// Returns parsed import descriptors.
const std::vector<ImportDescriptor>& LoadedPeImage::imports() const noexcept {
    return storage_->imports;
}

/// Returns raw import-address-table slots.
const std::vector<IatEntry>& LoadedPeImage::iat_entries() const noexcept {
    return storage_->iat_entries;
}

/// Returns the export header when present.
const std::optional<ExportDirectory>& LoadedPeImage::exports() const noexcept {
    return storage_->exports;
}

/// Returns all exported functions.
const std::vector<ExportedSymbol>& LoadedPeImage::exported_symbols() const noexcept {
    return storage_->exported_symbols;
}

/// Returns base-relocation blocks.
const std::vector<RelocationBlock>& LoadedPeImage::relocations() const noexcept {
    return storage_->relocations;
}

/// Returns debug directory records.
const std::vector<DebugDirectoryEntry>& LoadedPeImage::debug_entries() const noexcept {
    return storage_->debug_entries;
}

/// Returns exception runtime-function records.
const std::vector<RuntimeFunction>& LoadedPeImage::exception_functions() const noexcept {
    return storage_->exception_functions;
}

/// Returns the TLS directory when present.
const std::optional<TlsDirectory>& LoadedPeImage::tls() const noexcept {
    return storage_->tls;
}

/// Returns the load-config directory when present.
const std::optional<LoadConfigDirectory>& LoadedPeImage::load_config() const noexcept {
    return storage_->load_config;
}

/// Returns the resource tree when present.
const std::optional<ResourceDirectory>& LoadedPeImage::resources() const noexcept {
    return storage_->resources;
}

/// Returns Authenticode certificates.
const std::vector<SecurityCertificate>& LoadedPeImage::certificates() const noexcept {
    return storage_->certificates;
}

/// Returns bound import descriptors.
const std::vector<BoundImport>& LoadedPeImage::bound_imports() const noexcept {
    return storage_->bound_imports;
}

/// Returns delay-import descriptors.
const std::vector<DelayImportDescriptor>& LoadedPeImage::delay_imports() const noexcept {
    return storage_->delay_imports;
}

/// Returns the CLR header when present.
const std::optional<ClrHeader>& LoadedPeImage::clr_header() const noexcept {
    return storage_->clr_header;
}

/// Returns COFF symbols.
const std::vector<CoffSymbol>& LoadedPeImage::coff_symbols() const noexcept {
    return storage_->coff_symbols;
}

/// Returns the completeness status of the parsed image.
ParseStatus LoadedPeImage::parse_status() const noexcept {
    return storage_->parse_status;
}

/// Returns whether non-strict parsing tolerated one or more errors.
bool LoadedPeImage::is_partial() const noexcept {
    return storage_->parse_status == ParseStatus::partial;
}

/// Returns errors recorded while non-strict parsing continued.
const std::vector<ParseError>& LoadedPeImage::parse_diagnostics() const noexcept {
    return storage_->parse_diagnostics;
}

/// Returns the owned source file bytes.
std::span<const Byte> LoadedPeImage::file_bytes() const noexcept {
    return storage_->file;
}

/// Returns the declared image size.
std::uint32_t LoadedPeImage::image_size() const noexcept {
    return storage_->optional.size_of_image;
}

namespace {

/// Parses all fixed PE headers and the section table with checked arithmetic.
std::expected<void, ParseError> Parser::parse_headers() {
    if (reader_.bytes().empty()) {
        return parse_failure<void>(ParseErrorCode::empty_input, 0, "The PE input is empty");
    }
    auto dos_size = reader_.require(0, 64, "DOS header");
    if (!dos_size) {
        return std::unexpected(dos_size.error());
    }

    auto read_dos_u16 = [&](FileOffset offset, std::uint16_t& target,
                            std::string_view name) -> std::expected<void, ParseError> {
        auto value = reader_.u16(offset, name);
        if (!value) {
            return std::unexpected(value.error());
        }
        target = *value;
        return {};
    };
    auto read_dos_u32 = [&](FileOffset offset, std::uint32_t& target,
                            std::string_view name) -> std::expected<void, ParseError> {
        auto value = reader_.u32(offset, name);
        if (!value) {
            return std::unexpected(value.error());
        }
        target = *value;
        return {};
    };

    auto read = read_dos_u16(0x00, storage_.dos.e_magic, "DOS e_magic");
    if (!read) {
        return std::unexpected(read.error());
    }
    if (storage_.dos.e_magic != 0x5a4d) {
        return parse_failure<void>(ParseErrorCode::invalid_dos_signature, 0, "The DOS header does not begin with MZ");
    }
    const std::array<std::pair<FileOffset, std::uint16_t*>, 13> dos_fields = {{
        {0x02, &storage_.dos.e_cblp},
        {0x04, &storage_.dos.e_cp},
        {0x06, &storage_.dos.e_crlc},
        {0x08, &storage_.dos.e_cparhdr},
        {0x0a, &storage_.dos.e_minalloc},
        {0x0c, &storage_.dos.e_maxalloc},
        {0x0e, &storage_.dos.e_ss},
        {0x10, &storage_.dos.e_sp},
        {0x12, &storage_.dos.e_csum},
        {0x14, &storage_.dos.e_ip},
        {0x16, &storage_.dos.e_cs},
        {0x18, &storage_.dos.e_lfarlc},
        {0x1a, &storage_.dos.e_ovno},
    }};
    for (const auto& [offset, target] : dos_fields) {
        read = read_dos_u16(offset, *target, "DOS header field");
        if (!read) {
            return std::unexpected(read.error());
        }
    }
    for (std::size_t index = 0; index < std::size(storage_.dos.e_res); ++index) {
        read = read_dos_u16(0x1c + index * 2, storage_.dos.e_res[index], "DOS e_res");
        if (!read) {
            return std::unexpected(read.error());
        }
    }
    read = read_dos_u16(0x24, storage_.dos.e_oemid, "DOS e_oemid");
    if (!read) {
        return std::unexpected(read.error());
    }
    read = read_dos_u16(0x26, storage_.dos.e_oeminfo, "DOS e_oeminfo");
    if (!read) {
        return std::unexpected(read.error());
    }
    for (std::size_t index = 0; index < std::size(storage_.dos.e_res2); ++index) {
        read = read_dos_u16(0x28 + index * 2, storage_.dos.e_res2[index], "DOS e_res2");
        if (!read) {
            return std::unexpected(read.error());
        }
    }
    auto dos_offset = reader_.u32(0x3c, "DOS e_lfanew");
    if (!dos_offset) {
        return std::unexpected(dos_offset.error());
    }
    storage_.dos.e_lfanew = *dos_offset;
    nt_offset_ = storage_.dos.e_lfanew;
    if (nt_offset_ < 64 || nt_offset_ > reader_.bytes().size() - 4) {
        return parse_failure<void>(ParseErrorCode::invalid_dos_offset, 0x3c, "DOS e_lfanew points outside the input");
    }

    if (options_.parse_rich_header) {
        auto rich = parse_rich_header();
        if (!rich) {
            return std::unexpected(rich.error());
        }
    }

    auto signature = reader_.span(nt_offset_, 4, "PE signature");
    if (!signature) {
        return std::unexpected(signature.error());
    }
    if (std::memcmp(signature->data(), "PE\0\0", 4) != 0) {
        return parse_failure<void>(ParseErrorCode::invalid_pe_signature, nt_offset_,
                                   "The NT header does not begin with PE\\0\\0");
    }

    const FileOffset file_header_offset = nt_offset_ + 4;
    storage_.coff.file_offset = file_header_offset;
    auto coff_machine = reader_.u16(file_header_offset, "COFF machine");
    auto coff_sections = reader_.u16(file_header_offset + 2, "COFF section count");
    auto coff_timestamp = reader_.u32(file_header_offset + 4, "COFF timestamp");
    auto coff_symbols = reader_.u32(file_header_offset + 8, "COFF symbol pointer");
    auto coff_symbol_count = reader_.u32(file_header_offset + 12, "COFF symbol count");
    auto coff_optional_size = reader_.u16(file_header_offset + 16, "COFF optional size");
    auto coff_characteristics = reader_.u16(file_header_offset + 18, "COFF characteristics");
    if (!coff_machine || !coff_sections || !coff_timestamp || !coff_symbols || !coff_symbol_count ||
        !coff_optional_size || !coff_characteristics) {
        const ParseError* error = !coff_machine         ? &coff_machine.error()
                                  : !coff_sections      ? &coff_sections.error()
                                  : !coff_timestamp     ? &coff_timestamp.error()
                                  : !coff_symbols       ? &coff_symbols.error()
                                  : !coff_symbol_count  ? &coff_symbol_count.error()
                                  : !coff_optional_size ? &coff_optional_size.error()
                                                        : &coff_characteristics.error();
        return std::unexpected(*error);
    }
    storage_.coff.machine_raw = *coff_machine;
    storage_.coff.machine = decode_machine(*coff_machine);
    storage_.coff.number_of_sections = *coff_sections;
    storage_.coff.time_date_stamp = *coff_timestamp;
    storage_.coff.pointer_to_symbol_table = *coff_symbols;
    storage_.coff.number_of_symbols = *coff_symbol_count;
    storage_.coff.size_of_optional_header = *coff_optional_size;
    storage_.coff.characteristics = *coff_characteristics;

    const FileOffset optional_offset = file_header_offset + 20;
    storage_.optional.file_offset = optional_offset;
    storage_.optional.declared_size = storage_.coff.size_of_optional_header;
    auto optional_range = reader_.require(optional_offset, storage_.coff.size_of_optional_header, "optional header");
    if (!optional_range) {
        return std::unexpected(optional_range.error());
    }
    auto optional_u16 = [&](std::uint64_t relative, std::string_view name) -> std::expected<std::uint16_t, ParseError> {
        if (relative > storage_.coff.size_of_optional_header || 2 > storage_.coff.size_of_optional_header - relative) {
            return parse_failure<std::uint16_t>(ParseErrorCode::invalid_optional_header, optional_offset + relative,
                                                std::string("Optional header lacks ") + std::string(name));
        }
        return reader_.u16(optional_offset + relative, name);
    };
    auto optional_u32 = [&](std::uint64_t relative, std::string_view name) -> std::expected<std::uint32_t, ParseError> {
        if (relative > storage_.coff.size_of_optional_header || 4 > storage_.coff.size_of_optional_header - relative) {
            return parse_failure<std::uint32_t>(ParseErrorCode::invalid_optional_header, optional_offset + relative,
                                                std::string("Optional header lacks ") + std::string(name));
        }
        return reader_.u32(optional_offset + relative, name);
    };
    auto optional_u64 = [&](std::uint64_t relative, std::string_view name) -> std::expected<std::uint64_t, ParseError> {
        if (relative > storage_.coff.size_of_optional_header || 8 > storage_.coff.size_of_optional_header - relative) {
            return parse_failure<std::uint64_t>(ParseErrorCode::invalid_optional_header, optional_offset + relative,
                                                std::string("Optional header lacks ") + std::string(name));
        }
        return reader_.u64(optional_offset + relative, name);
    };
    auto optional_byte = [&](std::uint64_t relative, std::string_view name) -> std::expected<Byte, ParseError> {
        if (relative >= storage_.coff.size_of_optional_header) {
            return parse_failure<Byte>(ParseErrorCode::invalid_optional_header, optional_offset + relative,
                                       std::string("Optional header lacks ") + std::string(name));
        }
        return reader_.u8(optional_offset + relative, name);
    };
    auto magic = optional_u16(0, "optional magic");
    if (!magic) {
        return std::unexpected(magic.error());
    }
    storage_.optional.magic = *magic;
    if (*magic != 0x10b && *magic != 0x20b) {
        return parse_failure<void>(ParseErrorCode::invalid_optional_header, optional_offset,
                                   "Optional header magic is neither PE32 nor PE32+");
    }
    storage_.optional.pe32_plus = *magic == 0x20b;
    const std::uint64_t minimum_optional_size = storage_.optional.pe32_plus ? 112 : 96;
    const std::uint64_t directory_offset = storage_.optional.pe32_plus ? 112 : 96;
    if (storage_.coff.size_of_optional_header < minimum_optional_size) {
        return parse_failure<void>(ParseErrorCode::invalid_optional_header, optional_offset,
                                   "Optional header is shorter than its fixed PE format");
    }

    auto linker_major = optional_byte(2, "major linker version");
    auto linker_minor = optional_byte(3, "minor linker version");
    auto size_code = optional_u32(4, "size of code");
    auto size_init = optional_u32(8, "size of initialized data");
    auto size_uninit = optional_u32(12, "size of uninitialized data");
    auto entry_point = optional_u32(16, "entry point");
    auto base_code = optional_u32(20, "base of code");
    auto section_alignment = optional_u32(32, "section alignment");
    auto file_alignment = optional_u32(36, "file alignment");
    auto major_os = optional_u16(40, "major OS version");
    auto minor_os = optional_u16(42, "minor OS version");
    auto major_image = optional_u16(44, "major image version");
    auto minor_image = optional_u16(46, "minor image version");
    auto major_subsystem = optional_u16(48, "major subsystem version");
    auto minor_subsystem = optional_u16(50, "minor subsystem version");
    auto win32_version = optional_u32(52, "Win32 version");
    auto size_image = optional_u32(56, "size of image");
    auto size_headers = optional_u32(60, "size of headers");
    auto checksum = optional_u32(64, "checksum");
    auto subsystem = optional_u16(68, "subsystem");
    auto dll_characteristics = optional_u16(70, "DLL characteristics");
    auto loader_flags = optional_u32(storage_.optional.pe32_plus ? 104 : 88, "loader flags");
    auto directory_count = optional_u32(storage_.optional.pe32_plus ? 108 : 92, "directory count");
    if (!linker_major || !linker_minor || !size_code || !size_init || !size_uninit || !entry_point || !base_code ||
        !section_alignment || !file_alignment || !major_os || !minor_os || !major_image || !minor_image ||
        !major_subsystem || !minor_subsystem || !win32_version || !size_image || !size_headers || !checksum ||
        !subsystem || !dll_characteristics || !loader_flags || !directory_count) {
        const ParseError* error = !linker_major          ? &linker_major.error()
                                  : !linker_minor        ? &linker_minor.error()
                                  : !size_code           ? &size_code.error()
                                  : !size_init           ? &size_init.error()
                                  : !size_uninit         ? &size_uninit.error()
                                  : !entry_point         ? &entry_point.error()
                                  : !base_code           ? &base_code.error()
                                  : !section_alignment   ? &section_alignment.error()
                                  : !file_alignment      ? &file_alignment.error()
                                  : !major_os            ? &major_os.error()
                                  : !minor_os            ? &minor_os.error()
                                  : !major_image         ? &major_image.error()
                                  : !minor_image         ? &minor_image.error()
                                  : !major_subsystem     ? &major_subsystem.error()
                                  : !minor_subsystem     ? &minor_subsystem.error()
                                  : !win32_version       ? &win32_version.error()
                                  : !size_image          ? &size_image.error()
                                  : !size_headers        ? &size_headers.error()
                                  : !checksum            ? &checksum.error()
                                  : !subsystem           ? &subsystem.error()
                                  : !dll_characteristics ? &dll_characteristics.error()
                                  : !loader_flags        ? &loader_flags.error()
                                                         : &directory_count.error();
        return std::unexpected(*error);
    }
    storage_.optional.major_linker_version = *linker_major;
    storage_.optional.minor_linker_version = *linker_minor;
    storage_.optional.size_of_code = *size_code;
    storage_.optional.size_of_initialized_data = *size_init;
    storage_.optional.size_of_uninitialized_data = *size_uninit;
    storage_.optional.address_of_entry_point = *entry_point;
    storage_.optional.base_of_code = *base_code;
    storage_.optional.section_alignment = *section_alignment;
    storage_.optional.file_alignment = *file_alignment;
    storage_.optional.major_operating_system_version = *major_os;
    storage_.optional.minor_operating_system_version = *minor_os;
    storage_.optional.major_image_version = *major_image;
    storage_.optional.minor_image_version = *minor_image;
    storage_.optional.major_subsystem_version = *major_subsystem;
    storage_.optional.minor_subsystem_version = *minor_subsystem;
    storage_.optional.win32_version_value = *win32_version;
    storage_.optional.size_of_image = *size_image;
    storage_.optional.size_of_headers = *size_headers;
    storage_.optional.check_sum = *checksum;
    storage_.optional.subsystem = *subsystem;
    storage_.optional.dll_characteristics = *dll_characteristics;
    storage_.optional.loader_flags = *loader_flags;
    storage_.optional.number_of_rva_and_sizes = *directory_count;

    if (storage_.optional.pe32_plus) {
        auto image_base = optional_u64(24, "PE32+ image base");
        auto stack_reserve = optional_u64(72, "PE32+ stack reserve");
        auto stack_commit = optional_u64(80, "PE32+ stack commit");
        auto heap_reserve = optional_u64(88, "PE32+ heap reserve");
        auto heap_commit = optional_u64(96, "PE32+ heap commit");
        if (!image_base || !stack_reserve || !stack_commit || !heap_reserve || !heap_commit) {
            const ParseError* error = !image_base      ? &image_base.error()
                                      : !stack_reserve ? &stack_reserve.error()
                                      : !stack_commit  ? &stack_commit.error()
                                      : !heap_reserve  ? &heap_reserve.error()
                                                       : &heap_commit.error();
            return std::unexpected(*error);
        }
        storage_.optional.image_base = *image_base;
        storage_.optional.size_of_stack_reserve = *stack_reserve;
        storage_.optional.size_of_stack_commit = *stack_commit;
        storage_.optional.size_of_heap_reserve = *heap_reserve;
        storage_.optional.size_of_heap_commit = *heap_commit;
    } else {
        auto base_data = optional_u32(24, "PE32 base of data");
        auto image_base = optional_u32(28, "PE32 image base");
        auto stack_reserve = optional_u32(72, "PE32 stack reserve");
        auto stack_commit = optional_u32(76, "PE32 stack commit");
        auto heap_reserve = optional_u32(80, "PE32 heap reserve");
        auto heap_commit = optional_u32(84, "PE32 heap commit");
        if (!base_data || !image_base || !stack_reserve || !stack_commit || !heap_reserve || !heap_commit) {
            const ParseError* error = !base_data       ? &base_data.error()
                                      : !image_base    ? &image_base.error()
                                      : !stack_reserve ? &stack_reserve.error()
                                      : !stack_commit  ? &stack_commit.error()
                                      : !heap_reserve  ? &heap_reserve.error()
                                                       : &heap_commit.error();
            return std::unexpected(*error);
        }
        storage_.optional.base_of_data = *base_data;
        storage_.optional.image_base = *image_base;
        storage_.optional.size_of_stack_reserve = *stack_reserve;
        storage_.optional.size_of_stack_commit = *stack_commit;
        storage_.optional.size_of_heap_reserve = *heap_reserve;
        storage_.optional.size_of_heap_commit = *heap_commit;
    }
    const auto section_alignment_value = storage_.optional.section_alignment;
    const auto file_alignment_value = storage_.optional.file_alignment;
    const auto alignment_power_of_two = [](std::uint32_t value) { return value != 0 && (value & (value - 1U)) == 0; };
    if (!alignment_power_of_two(section_alignment_value) || !alignment_power_of_two(file_alignment_value) ||
        file_alignment_value > 0x10000 ||
        (section_alignment_value < 0x1000 && section_alignment_value != file_alignment_value)) {
        return parse_failure<void>(ParseErrorCode::invalid_alignment, optional_offset,
                                   "PE alignments must be powers of two and mutually compatible");
    }
    if (storage_.optional.size_of_image == 0 || storage_.optional.size_of_image > options_.maximum_image_size ||
        storage_.optional.size_of_headers > storage_.optional.size_of_image || storage_.optional.size_of_headers == 0) {
        return parse_failure<void>(ParseErrorCode::invalid_image_size, optional_offset + 56,
                                   "PE image or header size is outside configured limits");
    }
    if (add_overflow(storage_.optional.image_base, storage_.optional.size_of_image)) {
        return parse_failure<void>(ParseErrorCode::invalid_image_size, optional_offset + 24,
                                   "Image base plus image size overflows a virtual address");
    }
    if (storage_.optional.number_of_rva_and_sizes > options_.maximum_directory_entries) {
        return parse_failure<void>(ParseErrorCode::limit_exceeded,
                                   optional_offset + (storage_.optional.pe32_plus ? 108 : 92),
                                   "PE declares too many data directories");
    }
    const auto maximum_directories =
        (static_cast<std::uint64_t>(storage_.coff.size_of_optional_header) - directory_offset) / 8;
    if (storage_.optional.number_of_rva_and_sizes > maximum_directories) {
        return parse_failure<void>(ParseErrorCode::invalid_optional_header, optional_offset + directory_offset,
                                   "Data-directory count exceeds optional-header storage");
    }
    storage_.directories.reserve(storage_.optional.number_of_rva_and_sizes);
    storage_.optional.data_directories.reserve(storage_.optional.number_of_rva_and_sizes);
    for (std::uint32_t index = 0; index < storage_.optional.number_of_rva_and_sizes; ++index) {
        auto rva = optional_u32(directory_offset + static_cast<std::uint64_t>(index) * 8, "directory RVA");
        auto size = optional_u32(directory_offset + static_cast<std::uint64_t>(index) * 8 + 4, "directory size");
        if (!rva || !size) {
            return std::unexpected(!rva ? rva.error() : size.error());
        }
        DataDirectory data_directory;
        data_directory.index_raw = index;
        data_directory.index = index < 16 ? static_cast<DirectoryIndex>(index) : DirectoryIndex::reserved;
        data_directory.rva = *rva;
        data_directory.size = *size;
        if (data_directory.index == DirectoryIndex::security && data_directory.size != 0) {
            if (!valid_range(data_directory.rva, data_directory.size, reader_.bytes().size())) {
                return parse_failure<void>(ParseErrorCode::invalid_directory,
                                           optional_offset + directory_offset + index * 8,
                                           "Security directory file range is outside the input");
            }
            data_directory.address_is_file_offset = true;
            data_directory.file_offset = data_directory.rva;
        }
        storage_.directories.push_back(data_directory);
        storage_.optional.data_directories.push_back(data_directory);
    }

    const FileOffset section_table_offset = optional_offset + storage_.coff.size_of_optional_header;
    const auto section_table_size = static_cast<std::uint64_t>(storage_.coff.number_of_sections) * 40;
    if (multiply_overflow(storage_.coff.number_of_sections, 40) ||
        !valid_range(section_table_offset, section_table_size, reader_.bytes().size())) {
        return parse_failure<void>(ParseErrorCode::invalid_section_table, section_table_offset,
                                   "Section table is truncated or overflows the input");
    }
    storage_.sections.reserve(storage_.coff.number_of_sections);
    for (std::size_t index = 0; index < storage_.coff.number_of_sections; ++index) {
        const FileOffset section_offset = section_table_offset + index * 40;
        auto section_bytes = reader_.span(section_offset, 40, "section header");
        if (!section_bytes) {
            return std::unexpected(section_bytes.error());
        }
        Section section;
        section.index = index;
        section.name = section_name(section_bytes->first(8));
        auto virtual_size = reader_.u32(section_offset + 8, "section virtual size");
        auto virtual_address = reader_.u32(section_offset + 12, "section RVA");
        auto raw_size = reader_.u32(section_offset + 16, "section raw size");
        auto raw_offset = reader_.u32(section_offset + 20, "section raw offset");
        auto relocations = reader_.u32(section_offset + 24, "section relocation pointer");
        auto line_numbers = reader_.u32(section_offset + 28, "section line pointer");
        auto relocation_count = reader_.u16(section_offset + 32, "section relocation count");
        auto line_count = reader_.u16(section_offset + 34, "section line count");
        auto characteristics = reader_.u32(section_offset + 36, "section characteristics");
        if (!virtual_size || !virtual_address || !raw_size || !raw_offset || !relocations || !line_numbers ||
            !relocation_count || !line_count || !characteristics) {
            return std::unexpected(!virtual_size       ? virtual_size.error()
                                   : !virtual_address  ? virtual_address.error()
                                   : !raw_size         ? raw_size.error()
                                   : !raw_offset       ? raw_offset.error()
                                   : !relocations      ? relocations.error()
                                   : !line_numbers     ? line_numbers.error()
                                   : !relocation_count ? relocation_count.error()
                                   : !line_count       ? line_count.error()
                                                       : characteristics.error());
        }
        section.virtual_size = *virtual_size;
        section.virtual_address = *virtual_address;
        section.raw_size = *raw_size;
        section.raw_offset = *raw_offset;
        section.pointer_to_relocations = *relocations;
        section.pointer_to_line_numbers = *line_numbers;
        section.number_of_relocations = *relocation_count;
        section.number_of_line_numbers = *line_count;
        section.characteristics = *characteristics;
        const auto virtual_extent_source = section.virtual_size != 0 ? section.virtual_size : section.raw_size;
        const auto aligned_virtual_size = align_up_checked(virtual_extent_source, section_alignment_value);
        if (!aligned_virtual_size || *aligned_virtual_size > std::numeric_limits<std::uint32_t>::max()) {
            return parse_failure<void>(ParseErrorCode::invalid_section_range, section_offset + 8,
                                       "Section virtual extent cannot be represented safely");
        }
        section.virtual_extent = static_cast<std::uint32_t>(*aligned_virtual_size);
        // The integration contract follows PeLoader's actual Ghidra memory blocks, which retain
        // the unaligned section extent reported by the fixture generator. Keep the aligned extent
        // separately for range validation instead of changing observable block sizes.
        section.loaded_size = std::max(section.virtual_size, section.raw_size);
        if (section.raw_size != 0 && section.raw_offset < storage_.optional.size_of_headers) {
            return parse_failure<void>(ParseErrorCode::invalid_section_range, section_offset + 20,
                                       "Section raw bytes overlap the PE header area");
        }
        if (section.raw_size != 0 && section.raw_offset < reader_.bytes().size()) {
            section.file_backed_size = static_cast<std::uint32_t>(
                std::min<std::uint64_t>(section.raw_size, reader_.bytes().size() - section.raw_offset));
        }
        if (section.virtual_extent != 0 &&
            (section.virtual_address > storage_.optional.size_of_image ||
             section.virtual_extent > storage_.optional.size_of_image - section.virtual_address)) {
            return parse_failure<void>(ParseErrorCode::invalid_section_range, section_offset + 12,
                                       "Section virtual extent is outside SizeOfImage");
        }
        storage_.sections.push_back(std::move(section));
    }
    for (std::size_t left = 0; left < storage_.sections.size(); ++left) {
        const auto& first = storage_.sections[left];
        if (first.virtual_extent != 0 && first.virtual_address < storage_.optional.size_of_headers &&
            first.virtual_extent > storage_.optional.size_of_headers - first.virtual_address) {
            return parse_failure<void>(ParseErrorCode::invalid_section_range, first.virtual_address,
                                       "Section virtual range overlaps PE headers");
        }
        for (std::size_t right = left + 1; right < storage_.sections.size(); ++right) {
            const auto& second = storage_.sections[right];
            const auto first_virtual_end = static_cast<std::uint64_t>(first.virtual_address) + first.virtual_extent;
            const auto second_virtual_end = static_cast<std::uint64_t>(second.virtual_address) + second.virtual_extent;
            const bool virtual_overlap = first.virtual_extent != 0 && second.virtual_extent != 0 &&
                                         static_cast<std::uint64_t>(first.virtual_address) < second_virtual_end &&
                                         static_cast<std::uint64_t>(second.virtual_address) < first_virtual_end;
            const auto first_raw_end = first.raw_offset + first.raw_size;
            const auto second_raw_end = second.raw_offset + second.raw_size;
            const bool raw_overlap = first.raw_size != 0 && second.raw_size != 0 && first.raw_offset < second_raw_end &&
                                     second.raw_offset < first_raw_end;
            if (virtual_overlap || raw_overlap) {
                return parse_failure<void>(ParseErrorCode::invalid_section_range, first.virtual_address,
                                           "PE sections overlap in virtual or raw file space");
            }
        }
    }
    for (auto& data_directory : storage_.directories) {
        if (data_directory.size == 0) {
            continue;
        }
        if (data_directory.index != DirectoryIndex::security) {
            data_directory.file_offset = raw_offset_for_rva(data_directory.rva);
        }
    }
    storage_.optional.data_directories = storage_.directories;
    return {};
}

/// Parses the XOR-obfuscated Rich header used by Microsoft toolchains.
std::expected<void, ParseError> Parser::parse_rich_header() {
    const auto bytes = reader_.bytes();
    const FileOffset search_begin = 64;
    if (nt_offset_ <= search_begin + 8 || nt_offset_ > bytes.size()) {
        return {};
    }
    const auto begin = bytes.begin() + static_cast<std::ptrdiff_t>(search_begin);
    const auto scan_end = std::min<FileOffset>(nt_offset_, search_begin + 100U * 4U);
    const auto end = bytes.begin() + static_cast<std::ptrdiff_t>(scan_end);
    constexpr std::array<Byte, 4> rich_signature = {'R', 'i', 'c', 'h'};
    auto rich_magic = std::search(begin, end, rich_signature.begin(), rich_signature.end());
    if (rich_magic == end || std::distance(begin, rich_magic) < 16) {
        return {};
    }
    const FileOffset rich_end = static_cast<FileOffset>(std::distance(bytes.begin(), rich_magic));
    auto mask = reader_.u32(rich_end + 4, "Rich mask");
    if (!mask) {
        return std::unexpected(mask.error());
    }
    FileOffset dans = rich_end;
    bool found_dans = false;
    for (FileOffset candidate = search_begin; candidate + 4 <= rich_end; candidate += 4) {
        auto value = reader_.u32(candidate, "Rich signature search");
        if (value && *value == (0x536e6144U ^ *mask)) {
            dans = candidate;
            found_dans = true;
            break;
        }
    }
    if (!found_dans || rich_end + 8 > nt_offset_ || rich_end < dans + 16) {
        return {};
    }
    // Related source: ../../../Ghidra/Features/Base/src/main/java/ghidra/app/util/bin/format/pe/RichTable.java
    // RichTable.java requires DanS followed by three mask-encoded zero DWORDs.
    for (FileOffset padding_offset = dans + 4; padding_offset < dans + 16; padding_offset += 4) {
        auto padding = reader_.u32(padding_offset, "Rich padding");
        if (!padding || (*padding ^ *mask) != 0) {
            return {};
        }
    }
    if ((rich_end - (dans + 16)) % 8 != 0) {
        return {};
    }
    RichHeader rich;
    rich.offset = dans;
    rich.size = static_cast<std::uint32_t>(rich_end + 8 - dans);
    rich.mask = *mask;
    for (FileOffset record_offset = dans + 16; record_offset + 8 <= rich_end; record_offset += 8) {
        auto encoded_id = reader_.u32(record_offset, "Rich component ID");
        auto encoded_count = reader_.u32(record_offset + 4, "Rich object count");
        if (!encoded_id || !encoded_count) {
            return std::unexpected(!encoded_id ? encoded_id.error() : encoded_count.error());
        }
        const std::uint32_t component_id = *encoded_id ^ *mask;
        const std::uint32_t object_count = *encoded_count ^ *mask;
        rich.records.push_back(RichRecord{component_id, static_cast<std::uint16_t>(component_id >> 16U),
                                          static_cast<std::uint16_t>(component_id & 0xffffU), object_count});
    }
    storage_.rich = std::move(rich);
    return {};
}

/// Maps headers and sections into a zero-initialized preferred image.
std::expected<void, ParseError> Parser::map_image() {
    try {
        storage_.mapped_image.assign(storage_.optional.size_of_image, Byte{0});
    } catch (const std::bad_alloc&) {
        return parse_failure<void>(ParseErrorCode::limit_exceeded, storage_.optional.size_of_image,
                                   "Unable to allocate the declared PE image size");
    }
    const auto header_size = static_cast<std::size_t>(storage_.optional.size_of_headers);
    const auto copied_header_size = std::min<std::size_t>(header_size, reader_.bytes().size());
    std::copy_n(reader_.bytes().begin(), copied_header_size, storage_.mapped_image.begin());
    storage_.memory_regions.push_back(MemoryRegion{"Headers", storage_.optional.image_base,
                                                   storage_.optional.size_of_headers, true, false, false, true, true,
                                                   std::nullopt});
    for (const auto& section : storage_.sections) {
        if (section.loaded_size == 0) {
            continue;
        }
        auto destination = storage_.mapped_image.begin() + section.virtual_address;
        if (section.file_backed_size != 0) {
            const auto source = reader_.bytes().begin() + static_cast<std::ptrdiff_t>(section.raw_offset);
            std::copy_n(source, section.file_backed_size, destination);
        }
        storage_.memory_regions.push_back(MemoryRegion{
            section.name, storage_.optional.image_base + section.virtual_address, section.loaded_size,
            (section.characteristics & section_characteristics::memory_read) != 0,
            (section.characteristics & section_characteristics::memory_write) != 0,
            (section.characteristics & section_characteristics::memory_execute) != 0, true, false, section.index});
    }
    return {};
}

/// Resolves an RVA to the raw file offset of its first byte.
std::optional<FileOffset> Parser::raw_offset_for_rva(Rva rva) const {
    if (rva < storage_.optional.size_of_headers && rva < reader_.bytes().size()) {
        return rva;
    }
    for (const auto& section : storage_.sections) {
        if (section.file_backed_size != 0 && rva >= section.virtual_address &&
            rva - section.virtual_address < section.file_backed_size) {
            return section.raw_offset + (rva - section.virtual_address);
        }
    }
    return std::nullopt;
}

/// Resolves a preferred virtual address to an RVA during parsing.
std::optional<Rva> Parser::rva_for_va(Va va) const {
    if (va < storage_.optional.image_base || va - storage_.optional.image_base > 0xffffffffULL) {
        return std::nullopt;
    }
    const auto rva = static_cast<Rva>(va - storage_.optional.image_base);
    return rva < storage_.optional.size_of_image ? std::optional<Rva>(rva) : std::nullopt;
}

/// Reads a byte from the zero-filled image during parsing.
std::optional<Byte> Parser::mapped_byte(Rva rva) const {
    if (rva >= storage_.mapped_image.size()) {
        return std::nullopt;
    }
    const bool mapped = rva < storage_.optional.size_of_headers ||
                        std::any_of(
                            storage_.sections.begin(), storage_.sections.end(),
                            [rva](const Section& section) {
                                return section.loaded_size != 0 && rva >= section.virtual_address &&
                                       rva - section.virtual_address < section.loaded_size;
                            });
    return mapped ? std::optional<Byte>(storage_.mapped_image[rva]) : std::nullopt;
}

/// Reads a 16-bit integer from the mapped image.
std::optional<std::uint16_t> Parser::mapped_u16(Rva rva) const {
    auto first = mapped_byte(rva);
    auto second = rva != std::numeric_limits<Rva>::max() ? mapped_byte(rva + 1) : std::nullopt;
    return first && second ? std::optional<std::uint16_t>(static_cast<std::uint16_t>(*first) |
                                                          (static_cast<std::uint16_t>(*second) << 8U))
                           : std::nullopt;
}

/// Reads a 32-bit integer from the mapped image.
std::optional<std::uint32_t> Parser::mapped_u32(Rva rva) const {
    std::uint32_t value = 0;
    for (std::uint32_t index = 0; index < 4; ++index) {
        if (rva > std::numeric_limits<Rva>::max() - index) {
            return std::nullopt;
        }
        auto byte = mapped_byte(rva + index);
        if (!byte) {
            return std::nullopt;
        }
        value |= static_cast<std::uint32_t>(*byte) << (index * 8U);
    }
    return value;
}

/// Reads a 64-bit integer from the mapped image.
std::optional<std::uint64_t> Parser::mapped_u64(Rva rva) const {
    std::uint64_t value = 0;
    for (std::uint32_t index = 0; index < 8; ++index) {
        if (rva > std::numeric_limits<Rva>::max() - index) {
            return std::nullopt;
        }
        auto byte = mapped_byte(rva + index);
        if (!byte) {
            return std::nullopt;
        }
        value |= static_cast<std::uint64_t>(*byte) << (index * 8U);
    }
    return value;
}

/// Reads a null-terminated string from the mapped image with a hard bound.
std::expected<std::string, ParseError> Parser::mapped_string(Rva rva, std::uint64_t maximum,
                                                             std::string_view what) const {
    std::string result;
    for (std::uint64_t index = 0; index < maximum; ++index) {
        if (rva > std::numeric_limits<Rva>::max() - index) {
            return parse_failure<std::string>(ParseErrorCode::invalid_directory, rva,
                                              std::string("Overflow while reading ") + std::string(what));
        }
        auto byte = mapped_byte(rva + static_cast<Rva>(index));
        if (!byte) {
            return parse_failure<std::string>(ParseErrorCode::invalid_directory, rva,
                                              std::string("Unmapped ") + std::string(what));
        }
        if (*byte == 0) {
            return result;
        }
        result.push_back(static_cast<char>(*byte));
    }
    return parse_failure<std::string>(ParseErrorCode::truncated, rva, std::string("Unterminated ") + std::string(what));
}

/// Returns a directory pointer by its standard slot.
const DataDirectory* Parser::directory(DirectoryIndex index) const noexcept {
    const auto numeric = static_cast<std::size_t>(index);
    return numeric < storage_.directories.size() ? &storage_.directories[numeric] : nullptr;
}

/// Parses all non-empty standard directories in optional-header order.
std::expected<void, ParseError> Parser::parse_directories() {
    const std::array<std::pair<DirectoryIndex, std::expected<void, ParseError> (Parser::*)(const DataDirectory&)>, 15>
        parsers = {{
            {DirectoryIndex::export_table, &Parser::parse_exports},
            {DirectoryIndex::import_table, &Parser::parse_imports},
            {DirectoryIndex::resource_table, &Parser::parse_resources},
            // Ghidra parses load-config before exceptions so x86 CHPE metadata can select ARM rows.
            {DirectoryIndex::load_config, &Parser::parse_load_config},
            {DirectoryIndex::exception_table, &Parser::parse_exceptions},
            {DirectoryIndex::security, &Parser::parse_security},
            {DirectoryIndex::base_relocation_table, &Parser::parse_relocations},
            {DirectoryIndex::debug, &Parser::parse_debug},
            {DirectoryIndex::tls_table, &Parser::parse_tls},
            {DirectoryIndex::bound_import, &Parser::parse_bound_imports},
            {DirectoryIndex::delay_import, &Parser::parse_delay_imports},
            {DirectoryIndex::clr_runtime_header, &Parser::parse_clr},
            {DirectoryIndex::architecture, &Parser::parse_architecture},
            {DirectoryIndex::global_pointer, &Parser::parse_global_pointer},
            {DirectoryIndex::import_address_table, &Parser::parse_iat},
        }};
    for (const auto& [index, parser] : parsers) {
        const auto* data_directory = directory(index);
        const bool global_pointer_without_payload =
            index == DirectoryIndex::global_pointer && data_directory != nullptr && data_directory->rva != 0;
        if (data_directory == nullptr || (data_directory->size == 0 && !global_pointer_without_payload) ||
            data_directory->rva == 0 || parser == nullptr) {
            continue;
        }
        if (index != DirectoryIndex::security &&
            (data_directory->rva >= storage_.optional.size_of_image ||
             data_directory->size > storage_.optional.size_of_image - data_directory->rva)) {
            ParseError error{ParseErrorCode::invalid_directory, data_directory->rva,
                             std::string(directory_name(index)) + " exceeds SizeOfImage"};
            if (options_.strict) {
                return std::unexpected(std::move(error));
            }
            record_partial(std::move(error));
            continue;
        }
        auto result = (this->*parser)(*data_directory);
        if (!result) {
            if (options_.strict) {
                return std::unexpected(result.error());
            }
            record_partial(result.error());
        }
    }
    return {};
}

/// Records a non-fatal parser diagnostic and marks the image as incomplete.
void Parser::record_partial(ParseError error) {
    storage_.parse_status = ParseStatus::partial;
    storage_.parse_diagnostics.push_back(std::move(error));
}

// Related source:
// ../../../Ghidra/Features/Base/src/main/java/ghidra/app/util/bin/format/pe/ArchitectureDataDirectory.java
/// Parses the bounded ASCII payload of IMAGE_DIRECTORY_ENTRY_ARCHITECTURE.
std::expected<void, ParseError> Parser::parse_architecture(const DataDirectory& directory_value) {
    constexpr std::uint32_t maximum_copyright_size = 1000;
    if (directory_value.size > maximum_copyright_size) {
        return parse_failure<void>(ParseErrorCode::limit_exceeded, directory_value.rva,
                                   "Architecture directory exceeds the Ghidra size limit");
    }
    const auto file_offset = raw_offset_for_rva(directory_value.rva);
    if (!file_offset || !valid_range(*file_offset, directory_value.size, reader_.bytes().size())) {
        return parse_failure<void>(ParseErrorCode::invalid_directory, directory_value.rva,
                                   "Architecture directory is not file-backed");
    }
    auto bytes = reader_.span(*file_offset, directory_value.size, "architecture directory");
    if (!bytes) {
        return std::unexpected(bytes.error());
    }
    ArchitectureDirectory architecture;
    architecture.rva = directory_value.rva;
    architecture.size = directory_value.size;
    architecture.file_offset = *file_offset;
    const auto nul = std::find(bytes->begin(), bytes->end(), Byte{0});
    architecture.copyright.assign(reinterpret_cast<const char*>(bytes->data()),
                                  static_cast<std::size_t>(std::distance(bytes->begin(), nul)));
    while (!architecture.copyright.empty() && static_cast<unsigned char>(architecture.copyright.back()) <= 0x20U) {
        architecture.copyright.pop_back();
    }
    storage_.architecture_directory = std::move(architecture);
    return {};
}

// Related source:
// ../../../Ghidra/Features/Base/src/main/java/ghidra/app/util/bin/format/pe/GlobalPointerDataDirectory.java
/// Validates and records the RVA represented by IMAGE_DIRECTORY_ENTRY_GLOBALPTR.
std::expected<void, ParseError> Parser::parse_global_pointer(const DataDirectory& directory_value) {
    auto global_pointer_va = rva_for_va(storage_.optional.image_base + directory_value.rva);
    if (!global_pointer_va) {
        return parse_failure<void>(ParseErrorCode::invalid_directory, directory_value.rva,
                                   "Global-pointer directory RVA is outside the image");
    }
    GlobalPointerDirectory global_pointer;
    global_pointer.rva = directory_value.rva;
    global_pointer.size = directory_value.size;
    global_pointer.global_pointer_va = storage_.optional.image_base + directory_value.rva;
    global_pointer.file_offset = raw_offset_for_rva(directory_value.rva);
    storage_.global_pointer_directory = std::move(global_pointer);
    return {};
}

/// Parses IMAGE_EXPORT_DIRECTORY and all function/name/ordinal tables.
std::expected<void, ParseError> Parser::parse_exports(const DataDirectory& directory_value) {
    if (directory_value.size < 40) {
        return parse_failure<void>(ParseErrorCode::invalid_export, directory_value.rva,
                                   "Export directory is shorter than IMAGE_EXPORT_DIRECTORY");
    }
    auto characteristics = mapped_u32(directory_value.rva);
    auto timestamp = mapped_u32(directory_value.rva + 4);
    auto major = mapped_u16(directory_value.rva + 8);
    auto minor = mapped_u16(directory_value.rva + 10);
    auto name_rva = mapped_u32(directory_value.rva + 12);
    auto ordinal_base = mapped_u32(directory_value.rva + 16);
    auto function_count = mapped_u32(directory_value.rva + 20);
    auto name_count = mapped_u32(directory_value.rva + 24);
    auto functions_rva = mapped_u32(directory_value.rva + 28);
    auto names_rva = mapped_u32(directory_value.rva + 32);
    auto ordinals_rva = mapped_u32(directory_value.rva + 36);
    if (!characteristics || !timestamp || !major || !minor || !name_rva || !ordinal_base || !function_count ||
        !name_count || !functions_rva || !names_rva || !ordinals_rva) {
        return parse_failure<void>(ParseErrorCode::invalid_export, directory_value.rva,
                                   "Export directory contains an unmapped field");
    }
    if (*function_count > options_.maximum_directory_entries || *name_count > options_.maximum_directory_entries) {
        return parse_failure<void>(ParseErrorCode::limit_exceeded, directory_value.rva,
                                   "Export tables exceed the configured entry limit");
    }
    if (*function_count > std::numeric_limits<std::uint32_t>::max() - *ordinal_base) {
        return parse_failure<void>(ParseErrorCode::invalid_export, directory_value.rva + 16,
                                   "Export ordinal range overflows");
    }
    ExportDirectory exports;
    exports.characteristics = *characteristics;
    exports.time_date_stamp = *timestamp;
    exports.major_version = *major;
    exports.minor_version = *minor;
    exports.name_rva = *name_rva;
    exports.ordinal_base = *ordinal_base;
    exports.number_of_functions = *function_count;
    exports.number_of_names = *name_count;
    exports.address_of_functions_rva = *functions_rva;
    exports.address_of_names_rva = *names_rva;
    exports.address_of_name_ordinals_rva = *ordinals_rva;
    auto export_name = mapped_string(*name_rva, 1U << 16, "export DLL name");
    if (!export_name) {
        return std::unexpected(export_name.error());
    }
    exports.name = *export_name;
    exports.address_of_functions_file_offset = raw_offset_for_rva(*functions_rva);
    exports.address_of_names_file_offset = raw_offset_for_rva(*names_rva);
    exports.address_of_name_ordinals_file_offset = raw_offset_for_rva(*ordinals_rva);
    std::vector<std::optional<std::string>> names(*name_count);
    std::vector<std::uint16_t> name_ordinals(*name_count);
    for (std::uint32_t index = 0; index < *name_count; ++index) {
        const auto name_delta = static_cast<std::uint64_t>(index) * 4U;
        if (name_delta > std::numeric_limits<Rva>::max() || *names_rva > std::numeric_limits<Rva>::max() - name_delta) {
            return parse_failure<void>(ParseErrorCode::invalid_export, *names_rva, "Export name table overflows");
        }
        const auto name_slot = *names_rva + static_cast<Rva>(name_delta);
        auto name_pointer = mapped_u32(name_slot);
        if (!name_pointer) {
            return parse_failure<void>(ParseErrorCode::invalid_export, *names_rva, "Export name table is unmapped");
        }
        auto name = mapped_string(*name_pointer, 1U << 16, "export name");
        const auto ordinal_delta = static_cast<std::uint64_t>(index) * 2U;
        if (ordinal_delta > std::numeric_limits<Rva>::max() ||
            *ordinals_rva > std::numeric_limits<Rva>::max() - ordinal_delta) {
            return parse_failure<void>(ParseErrorCode::invalid_export, *ordinals_rva, "Export ordinal table overflows");
        }
        auto ordinal = mapped_u16(*ordinals_rva + static_cast<Rva>(ordinal_delta));
        if (!name || !ordinal) {
            return parse_failure<void>(ParseErrorCode::invalid_export, *names_rva,
                                       "Export name or ordinal table is malformed");
        }
        if (*ordinal >= *function_count) {
            return parse_failure<void>(ParseErrorCode::invalid_export, *ordinals_rva + static_cast<Rva>(ordinal_delta),
                                       "Export name ordinal exceeds function table");
        }
        names[*ordinal] = std::move(*name);
        name_ordinals[index] = *ordinal;
    }
    storage_.exported_symbols.reserve(*function_count);
    for (std::uint32_t index = 0; index < *function_count; ++index) {
        const auto function_delta = static_cast<std::uint64_t>(index) * 4U;
        if (function_delta > std::numeric_limits<Rva>::max() ||
            *functions_rva > std::numeric_limits<Rva>::max() - function_delta) {
            return parse_failure<void>(ParseErrorCode::invalid_export, *functions_rva,
                                       "Export function table overflows");
        }
        const auto function_slot = *functions_rva + static_cast<Rva>(function_delta);
        auto function_rva = mapped_u32(function_slot);
        if (!function_rva) {
            return parse_failure<void>(ParseErrorCode::invalid_export, *functions_rva,
                                       "Export function table is unmapped");
        }
        ExportedSymbol symbol;
        symbol.ordinal = *ordinal_base + index;
        symbol.name = names[index];
        symbol.address_rva = *function_rva;
        symbol.address_va = storage_.optional.image_base + *function_rva;
        symbol.file_offset = raw_offset_for_rva(*function_rva);
        const auto function_rva_u64 = static_cast<std::uint64_t>(*function_rva);
        const auto export_end_rva = static_cast<std::uint64_t>(directory_value.rva) + directory_value.size;
        const bool forwarded = function_rva_u64 >= directory_value.rva && function_rva_u64 < export_end_rva;
        if (!forwarded && *function_rva != 0 && *function_rva >= storage_.optional.size_of_image) {
            return parse_failure<void>(ParseErrorCode::invalid_export, function_slot,
                                       "Export function RVA is outside SizeOfImage");
        }
        symbol.forwarded = forwarded;
        if (forwarded) {
            const auto remaining = export_end_rva - function_rva_u64;
            auto forwarder = mapped_string(*function_rva, remaining, "export forwarder");
            if (!forwarder) {
                return parse_failure<void>(ParseErrorCode::invalid_export, *function_rva,
                                           "Export forwarder is not terminated within the export directory");
            }
            symbol.forwarder = std::move(*forwarder);
            symbol.file_offset = raw_offset_for_rva(*function_rva);
        }
        storage_.exported_symbols.push_back(std::move(symbol));
    }
    storage_.exports = std::move(exports);
    return {};
}

/// Parses one thunk array and appends its named or ordinal imports.
std::expected<void, ParseError> Parser::parse_thunks(std::vector<ImportedSymbol>& symbols, Rva int_rva, Rva iat_rva,
                                                     std::uint32_t maximum_entries, bool thunk_values_are_va) {
    const std::uint64_t width = storage_.optional.pe32_plus ? 8 : 4;
    const std::uint64_t ordinal_flag = storage_.optional.pe32_plus ? 0x8000000000000000ULL : 0x80000000ULL;
    const std::uint64_t value_mask = storage_.optional.pe32_plus ? 0x7fffffffffffffffULL : 0x7fffffffULL;
    for (std::uint32_t index = 0; index < maximum_entries; ++index) {
        const auto int_delta = static_cast<std::uint64_t>(index) * width;
        if (int_delta > std::numeric_limits<Rva>::max() || int_rva > std::numeric_limits<Rva>::max() - int_delta ||
            iat_rva > std::numeric_limits<Rva>::max() - int_delta) {
            return parse_failure<void>(ParseErrorCode::invalid_import, int_rva,
                                       "Import thunk table overflows the RVA space");
        }
        const auto int_slot = int_rva + static_cast<Rva>(int_delta);
        const auto iat_slot = iat_rva + static_cast<Rva>(int_delta);
        const auto iat_value =
            storage_.optional.pe32_plus
                ? mapped_u64(iat_slot)
                : mapped_u32(iat_slot).transform([](std::uint32_t value) { return static_cast<std::uint64_t>(value); });
        if (!iat_value) {
            return parse_failure<void>(ParseErrorCode::invalid_import, iat_slot,
                                       "Import address table slot is unmapped");
        }
        std::optional<std::uint64_t> thunk;
        if (storage_.optional.pe32_plus) {
            thunk = mapped_u64(int_slot);
        } else if (auto value = mapped_u32(int_slot)) {
            thunk = static_cast<std::uint64_t>(*value);
        }
        if (!thunk) {
            return parse_failure<void>(ParseErrorCode::invalid_import, int_slot,
                                       "Import thunk table is unmapped or unterminated");
        }
        if (*thunk == 0) {
            return {};
        }
        ImportedSymbol symbol;
        symbol.int_slot_rva = int_slot;
        symbol.iat_slot_rva = iat_slot;
        symbol.iat_slot_va = storage_.optional.image_base + iat_slot;
        symbol.iat_slot_file_offset = raw_offset_for_rva(iat_slot);
        symbol.thunk_value = *thunk;
        symbol.imported_by_ordinal = (*thunk & ordinal_flag) != 0;
        if (symbol.imported_by_ordinal) {
            symbol.ordinal = static_cast<std::uint16_t>(*thunk & 0xffffU);
        } else {
            const auto address_of_data = *thunk & value_mask;
            std::optional<Rva> name_rva;
            if (thunk_values_are_va) {
                name_rva = rva_for_va(address_of_data);
            } else if (address_of_data <= std::numeric_limits<Rva>::max()) {
                name_rva = static_cast<Rva>(address_of_data);
            }
            if (!name_rva) {
                return parse_failure<void>(ParseErrorCode::invalid_import, int_slot,
                                           "Import-by-name pointer is not a valid image RVA");
            }
            if (*name_rva > std::numeric_limits<Rva>::max() - 2U) {
                return parse_failure<void>(ParseErrorCode::invalid_import, *name_rva, "Import-by-name RVA overflows");
            }
            auto hint = mapped_u16(*name_rva);
            auto name = mapped_string(*name_rva + 2, 1U << 16, "import name");
            if (!hint || !name) {
                return parse_failure<void>(ParseErrorCode::invalid_import, *name_rva,
                                           "Import-by-name record is malformed");
            }
            symbol.hint = *hint;
            symbol.name = std::move(*name);
            symbol.import_by_name_rva = *name_rva;
            symbol.import_by_name_file_offset = raw_offset_for_rva(*name_rva);
        }
        symbols.push_back(std::move(symbol));
    }
    return parse_failure<void>(ParseErrorCode::limit_exceeded, int_rva,
                               "Import thunk table has no terminator before the configured limit");
}

/// Parses import descriptors and the corresponding INT/IAT arrays.
std::expected<void, ParseError> Parser::parse_imports(const DataDirectory& directory_value) {
    constexpr std::uint32_t descriptor_size = 20;
    bool terminated = false;
    for (std::uint32_t offset = 0; directory_value.size - offset >= descriptor_size; offset += descriptor_size) {
        const auto rva = directory_value.rva + offset;
        auto original = mapped_u32(rva);
        auto timestamp = mapped_u32(rva + 4);
        auto forwarder = mapped_u32(rva + 8);
        auto name_rva = mapped_u32(rva + 12);
        auto first_thunk = mapped_u32(rva + 16);
        if (!original || !timestamp || !forwarder || !name_rva || !first_thunk) {
            return parse_failure<void>(ParseErrorCode::invalid_import, rva, "Import descriptor is unmapped");
        }
        if (*original == 0 && *timestamp == 0 && *forwarder == 0 && *name_rva == 0 && *first_thunk == 0) {
            terminated = true;
            break;
        }
        if (*name_rva == 0 || *first_thunk == 0) {
            return parse_failure<void>(ParseErrorCode::invalid_import, rva,
                                       "Import descriptor lacks a DLL name or IAT");
        }
        auto dll_name = mapped_string(*name_rva, 1U << 16, "import DLL name");
        if (!dll_name) {
            return std::unexpected(dll_name.error());
        }
        ImportDescriptor descriptor;
        descriptor.dll_name = std::move(*dll_name);
        descriptor.name_rva = *name_rva;
        descriptor.name_file_offset = raw_offset_for_rva(*name_rva);
        descriptor.original_first_thunk_rva = *original;
        descriptor.first_thunk_rva = *first_thunk;
        descriptor.time_date_stamp = *timestamp;
        descriptor.forwarder_chain = *forwarder;
        descriptor.bound = *timestamp != 0;
        const auto int_rva = *original != 0 ? *original : *first_thunk;
        auto thunks = parse_thunks(descriptor.symbols, int_rva, *first_thunk, options_.maximum_directory_entries);
        if (!thunks) {
            return std::unexpected(thunks.error());
        }
        storage_.imports.push_back(std::move(descriptor));
    }
    if (!terminated && options_.strict) {
        return parse_failure<void>(ParseErrorCode::invalid_import, directory_value.rva + directory_value.size,
                                   "Import descriptor array has no null terminator");
    }
    return {};
}

// Related Ghidra source:
// ../../../Ghidra/Features/Base/src/main/java/ghidra/app/util/bin/format/pe/ImportAddressTableDataDirectory.java
/// Parses every pointer-sized slot in IMAGE_DIRECTORY_ENTRY_IAT.
std::expected<void, ParseError> Parser::parse_iat(const DataDirectory& directory_value) {
    const std::uint32_t slot_size = storage_.optional.pe32_plus ? 8 : 4;
    if (directory_value.size % slot_size != 0) {
        return parse_failure<void>(ParseErrorCode::invalid_import, directory_value.rva,
                                   "Import address table size is not pointer aligned");
    }
    const auto slot_count = directory_value.size / slot_size;
    if (slot_count > options_.maximum_directory_entries) {
        return parse_failure<void>(ParseErrorCode::limit_exceeded, directory_value.rva,
                                   "Import address table exceeds the configured entry limit");
    }
    storage_.iat_entries.reserve(slot_count);
    for (std::uint32_t index = 0; index < slot_count; ++index) {
        const auto delta = static_cast<std::uint64_t>(index) * slot_size;
        if (delta > std::numeric_limits<Rva>::max() || directory_value.rva > std::numeric_limits<Rva>::max() - delta) {
            return parse_failure<void>(ParseErrorCode::invalid_import, directory_value.rva,
                                       "Import address table slot overflows the RVA space");
        }
        const auto slot_rva = directory_value.rva + static_cast<Rva>(delta);
        const auto value =
            storage_.optional.pe32_plus
                ? mapped_u64(slot_rva)
                : mapped_u32(slot_rva).transform([](std::uint32_t item) { return static_cast<std::uint64_t>(item); });
        if (!value) {
            return parse_failure<void>(ParseErrorCode::invalid_import, slot_rva,
                                       "Import address table slot is unmapped");
        }
        storage_.iat_entries.push_back(
            IatEntry{slot_rva, storage_.optional.image_base + slot_rva, raw_offset_for_rva(slot_rva), *value});
    }
    return {};
}

/// Parses base-relocation blocks and checked target addresses.
std::expected<void, ParseError> Parser::parse_relocations(const DataDirectory& directory_value) {
    std::uint32_t offset = 0;
    while (offset < directory_value.size) {
        if (directory_value.size - offset < 8) {
            return parse_failure<void>(ParseErrorCode::invalid_relocation, directory_value.rva + offset,
                                       "Relocation block header is truncated");
        }
        const auto block_rva = directory_value.rva + offset;
        auto page = mapped_u32(block_rva);
        auto block_size = mapped_u32(block_rva + 4);
        if (!page || !block_size) {
            return parse_failure<void>(ParseErrorCode::invalid_relocation, block_rva, "Relocation block is unmapped");
        }
        if (*page == 0 && *block_size == 0) {
            break;
        }
        if (*block_size < 8 || (*block_size & 1U) != 0 || *block_size > directory_value.size - offset) {
            return parse_failure<void>(ParseErrorCode::invalid_relocation, block_rva + 4,
                                       "Relocation block size is invalid");
        }
        RelocationBlock block;
        block.page_rva = *page;
        block.block_size = *block_size;
        block.file_offset = raw_offset_for_rva(block_rva);
        const auto entry_count = (*block_size - 8) / 2;
        for (std::uint32_t index = 0; index < entry_count; ++index) {
            const auto entry_delta = static_cast<std::uint64_t>(index) * 2U;
            if (entry_delta > std::numeric_limits<Rva>::max() - 8U ||
                block_rva > std::numeric_limits<Rva>::max() - 8U - entry_delta) {
                return parse_failure<void>(ParseErrorCode::invalid_relocation, block_rva,
                                           "Relocation entry address overflows");
            }
            const auto entry_rva = block_rva + 8U + static_cast<Rva>(entry_delta);
            auto raw_value = mapped_u16(entry_rva);
            if (!raw_value) {
                return parse_failure<void>(ParseErrorCode::invalid_relocation, entry_rva,
                                           "Relocation entry is unmapped");
            }
            RelocationEntry entry;
            entry.raw_value = *raw_value;
            entry.type = static_cast<std::uint16_t>(*raw_value >> 12U);
            entry.offset = static_cast<std::uint16_t>(*raw_value & 0x0fffU);
            if (*page > std::numeric_limits<Rva>::max() - entry.offset) {
                return parse_failure<void>(ParseErrorCode::invalid_relocation, block_rva,
                                           "Relocation target overflows the RVA space");
            }
            entry.target_rva = *page + entry.offset;
            if (entry.target_rva >= storage_.optional.size_of_image) {
                return parse_failure<void>(ParseErrorCode::invalid_relocation, entry.target_rva,
                                           "Relocation target is outside SizeOfImage");
            }
            entry.target_va = storage_.optional.image_base + entry.target_rva;
            entry.target_file_offset = raw_offset_for_rva(entry.target_rva);
            block.entries.push_back(entry);
        }
        storage_.relocations.push_back(std::move(block));
        offset += *block_size;
    }
    return {};
}

/// Parses IMAGE_DEBUG_DIRECTORY records and CodeView RSDS/NB10 payloads.
std::expected<void, ParseError> Parser::parse_debug(const DataDirectory& directory_value) {
    constexpr std::uint32_t entry_size = 28;
    if (directory_value.size % entry_size != 0) {
        return parse_failure<void>(ParseErrorCode::invalid_debug_data, directory_value.rva,
                                   "Debug directory size is not a multiple of IMAGE_DEBUG_DIRECTORY");
    }
    for (std::uint32_t offset = 0; offset < directory_value.size; offset += entry_size) {
        const auto rva = directory_value.rva + offset;
        auto characteristics = mapped_u32(rva);
        auto timestamp = mapped_u32(rva + 4);
        auto major = mapped_u16(rva + 8);
        auto minor = mapped_u16(rva + 10);
        auto type = mapped_u32(rva + 12);
        auto size = mapped_u32(rva + 16);
        auto data_rva = mapped_u32(rva + 20);
        auto raw_pointer = mapped_u32(rva + 24);
        if (!characteristics || !timestamp || !major || !minor || !type || !size || !data_rva || !raw_pointer) {
            return parse_failure<void>(ParseErrorCode::invalid_debug_data, rva, "Debug directory entry is unmapped");
        }
        DebugDirectoryEntry entry;
        entry.characteristics = *characteristics;
        entry.time_date_stamp = *timestamp;
        entry.major_version = *major;
        entry.minor_version = *minor;
        entry.type_raw = *type;
        entry.type = *type <= 11 ? static_cast<DebugType>(*type) : DebugType::unknown;
        entry.size_of_data = *size;
        entry.address_of_raw_data = *data_rva;
        entry.pointer_to_raw_data = *raw_pointer;
        if (*size != 0) {
            if (valid_range(*raw_pointer, *size, reader_.bytes().size())) {
                entry.data_file_offset = *raw_pointer;
            } else if (raw_offset_for_rva(*data_rva).has_value() &&
                       valid_range(*raw_offset_for_rva(*data_rva), *size, reader_.bytes().size())) {
                entry.data_file_offset = *raw_offset_for_rva(*data_rva);
            } else {
                return parse_failure<void>(ParseErrorCode::invalid_debug_data, rva,
                                           "Debug payload is outside the file");
            }
            if (entry.type == DebugType::code_view) {
                const auto payload_offset = *entry.data_file_offset;
                auto signature = reader_.span(payload_offset, std::min<std::uint32_t>(*size, 4), "CodeView signature");
                if (!signature || signature->size() < 4) {
                    return parse_failure<void>(ParseErrorCode::invalid_debug_data, payload_offset,
                                               "CodeView payload is truncated");
                }
                CodeViewInfo code_view;
                code_view.signature.assign(reinterpret_cast<const char*>(signature->data()), 4);
                if (code_view.signature == "RSDS" && *size >= 24) {
                    auto payload = reader_.span(payload_offset, *size, "RSDS payload");
                    auto age = reader_.u32(payload_offset + 20, "RSDS age");
                    if (!payload || !age) {
                        return std::unexpected(!payload ? payload.error() : age.error());
                    }
                    code_view.identifier.assign(payload->begin() + 4, payload->begin() + 20);
                    code_view.age = *age;
                    auto path = read_file_string(reader_, payload_offset + 24, *size - 24, "PDB path");
                    if (path) {
                        code_view.path = std::move(*path);
                        code_view.valid = true;
                    }
                } else if (code_view.signature == "NB10" && *size >= 16) {
                    auto timestamp_value = reader_.u32(payload_offset + 8, "NB10 timestamp");
                    auto age = reader_.u32(payload_offset + 12, "NB10 age");
                    if (!timestamp_value || !age) {
                        return std::unexpected(!timestamp_value ? timestamp_value.error() : age.error());
                    }
                    code_view.identifier.resize(8);
                    std::memcpy(code_view.identifier.data(), &*timestamp_value, 4);
                    std::memcpy(code_view.identifier.data() + 4, &*age, 4);
                    code_view.age = *age;
                    auto path = read_file_string(reader_, payload_offset + 16, *size - 16, "PDB path");
                    if (path) {
                        code_view.path = std::move(*path);
                        code_view.valid = true;
                    }
                }
                entry.code_view = std::move(code_view);
            }
        }
        storage_.debug_entries.push_back(std::move(entry));
    }
    return {};
}

/// Parses fixed-size runtime-function records from the exception directory.
std::expected<void, ParseError> Parser::parse_exceptions(const DataDirectory& directory_value) {
    const bool chpe_image = storage_.load_config.has_value() && storage_.load_config->chpe_metadata_pointer != 0;
    const bool packed_arm_records =
        storage_.coff.machine == Machine::arm || storage_.coff.machine == Machine::armnt ||
        storage_.coff.machine == Machine::thumb || storage_.coff.machine == Machine::arm64 ||
        storage_.coff.machine == Machine::arm64ec || storage_.coff.machine == Machine::arm64x || chpe_image;
    const std::uint32_t record_size = packed_arm_records ? 8 : 12;
    if (directory_value.size % record_size != 0) {
        return parse_failure<void>(ParseErrorCode::invalid_directory, directory_value.rva,
                                   "Exception directory is not aligned to runtime-function records");
    }
    const auto count = directory_value.size / record_size;
    if (count > options_.maximum_directory_entries) {
        return parse_failure<void>(ParseErrorCode::limit_exceeded, directory_value.rva,
                                   "Exception directory exceeds the configured entry limit");
    }
    storage_.exception_functions.reserve(count);
    for (std::uint32_t index = 0; index < count; ++index) {
        const auto rva = directory_value.rva + index * record_size;
        auto begin = mapped_u32(rva);
        if (!begin) {
            return parse_failure<void>(ParseErrorCode::invalid_directory, rva,
                                       "Exception runtime-function begin address is unmapped");
        }
        if (packed_arm_records) {
            *begin &= ~1U;
        }
        auto end = packed_arm_records ? std::optional<std::uint32_t>(*begin) : mapped_u32(rva + 4);
        auto unwind = packed_arm_records ? mapped_u32(rva + 4) : mapped_u32(rva + 8);
        if (!end || !unwind || (!packed_arm_records && *begin > *end) || *end > storage_.optional.size_of_image) {
            return parse_failure<void>(ParseErrorCode::invalid_directory, rva,
                                       "Exception runtime-function range is invalid");
        }
        RuntimeFunction function;
        function.begin_rva = *begin;
        function.end_rva = *end;
        function.begin_va = storage_.optional.image_base + *begin;
        function.end_va = storage_.optional.image_base + *end;
        if (packed_arm_records && (*unwind & 3U) != 0) {
            function.packed_unwind_data = *unwind;
            function.unwind_info_rva = 0;
            function.unwind_info_va = 0;
            function.unwind_info_file_offset.reset();
        } else if (packed_arm_records) {
            function.unwind_info_rva = *unwind & ~3U;
            if (function.unwind_info_rva >= storage_.optional.size_of_image) {
                return parse_failure<void>(ParseErrorCode::invalid_directory, rva + 4,
                                           "Packed unwind RVA is outside SizeOfImage");
            }
            function.unwind_info_va = storage_.optional.image_base + function.unwind_info_rva;
            function.unwind_info_file_offset = raw_offset_for_rva(function.unwind_info_rva);
        } else {
            if (*unwind >= storage_.optional.size_of_image) {
                return parse_failure<void>(ParseErrorCode::invalid_directory, rva + 8,
                                           "Unwind RVA is outside SizeOfImage");
            }
            function.unwind_info_rva = *unwind;
            function.unwind_info_va = storage_.optional.image_base + *unwind;
            function.unwind_info_file_offset = raw_offset_for_rva(*unwind);
        }
        storage_.exception_functions.push_back(function);
    }
    return {};
}

/// Parses IMAGE_TLS_DIRECTORY32/64 and its null-terminated callback pointer array.
std::expected<void, ParseError> Parser::parse_tls(const DataDirectory& directory_value) {
    const std::uint32_t minimum_size = storage_.optional.pe32_plus ? 40 : 24;
    if (directory_value.size < minimum_size) {
        return parse_failure<void>(ParseErrorCode::invalid_directory, directory_value.rva,
                                   "TLS directory is shorter than its architecture-specific header");
    }
    auto read_pointer = [&](Rva rva) -> std::optional<Va> {
        return storage_.optional.pe32_plus
                   ? mapped_u64(rva)
                   : mapped_u32(rva).transform([](std::uint32_t value) { return static_cast<Va>(value); });
    };
    auto start = read_pointer(directory_value.rva);
    auto end = read_pointer(directory_value.rva + (storage_.optional.pe32_plus ? 8 : 4));
    auto index = read_pointer(directory_value.rva + (storage_.optional.pe32_plus ? 16 : 8));
    auto callbacks = read_pointer(directory_value.rva + (storage_.optional.pe32_plus ? 24 : 12));
    auto zero_fill = mapped_u32(directory_value.rva + (storage_.optional.pe32_plus ? 32 : 16));
    auto characteristics = mapped_u32(directory_value.rva + (storage_.optional.pe32_plus ? 36 : 20));
    if (!start || !end || !index || !callbacks || !zero_fill || !characteristics) {
        return parse_failure<void>(ParseErrorCode::invalid_directory, directory_value.rva,
                                   "TLS directory field is unmapped");
    }
    TlsDirectory tls;
    tls.start_address_of_raw_data = *start;
    tls.end_address_of_raw_data = *end;
    tls.address_of_index = *index;
    tls.address_of_callbacks = *callbacks;
    tls.size_of_zero_fill = *zero_fill;
    tls.characteristics = *characteristics;
    if (*callbacks != 0) {
        auto callback_rva = rva_for_va(*callbacks);
        if (!callback_rva) {
            return parse_failure<void>(ParseErrorCode::invalid_directory, directory_value.rva,
                                       "TLS callback array address is outside the image");
        }
        for (std::uint32_t callback_index = 0; callback_index < 4096; ++callback_index) {
            const auto pointer_rva = *callback_rva + callback_index * (storage_.optional.pe32_plus ? 8U : 4U);
            if (pointer_rva < *callback_rva) {
                return parse_failure<void>(ParseErrorCode::invalid_directory, pointer_rva,
                                           "TLS callback array overflows");
            }
            auto callback = read_pointer(pointer_rva);
            if (!callback) {
                return parse_failure<void>(ParseErrorCode::invalid_directory, pointer_rva,
                                           "TLS callback array is unmapped or unterminated");
            }
            if (*callback == 0) {
                break;
            }
            tls.callback_addresses.push_back(*callback);
            if (callback_index + 1 == 4096) {
                return parse_failure<void>(ParseErrorCode::limit_exceeded, pointer_rva,
                                           "TLS callback array exceeds the configured limit");
            }
        }
    }
    storage_.tls = std::move(tls);
    return {};
}

/// Parses versioned PE32/PE32+ load-config fields without reading beyond the declared size.
std::expected<void, ParseError> Parser::parse_load_config(const DataDirectory& directory_value) {
    if (directory_value.size < 4) {
        return parse_failure<void>(ParseErrorCode::invalid_directory, directory_value.rva,
                                   "Load-config directory lacks its size field");
    }
    auto declared_size = mapped_u32(directory_value.rva);
    if (!declared_size || *declared_size < 4) {
        return parse_failure<void>(ParseErrorCode::invalid_directory, directory_value.rva,
                                   "Load-config size is invalid");
    }
    const auto available = std::min<std::uint32_t>(*declared_size, directory_value.size);
    auto u16 = [&](std::uint32_t offset) -> std::optional<std::uint16_t> {
        return offset <= available && 2 <= available - offset ? mapped_u16(directory_value.rva + offset) : std::nullopt;
    };
    auto u32 = [&](std::uint32_t offset) -> std::optional<std::uint32_t> {
        return offset <= available && 4 <= available - offset ? mapped_u32(directory_value.rva + offset) : std::nullopt;
    };
    auto pointer = [&](std::uint32_t offset) -> std::optional<Va> {
        if (offset > available || (storage_.optional.pe32_plus ? 8U : 4U) > available - offset) {
            return std::nullopt;
        }
        return storage_.optional.pe32_plus
                   ? mapped_u64(directory_value.rva + offset)
                   : mapped_u32(directory_value.rva + offset).transform([](std::uint32_t value) {
                         return static_cast<Va>(value);
                     });
    };
    LoadConfigDirectory config;
    config.size = *declared_size;
    config.available_size = available;
    config.raw_bytes.reserve(available);
    for (std::uint32_t index = 0; index < available; ++index) {
        auto byte = mapped_byte(directory_value.rva + index);
        if (!byte) {
            break;
        }
        config.raw_bytes.push_back(*byte);
    }
    if (auto value = u32(4)) {
        config.time_date_stamp = *value;
    }
    if (auto value = u16(8)) {
        config.major_version = *value;
    }
    if (auto value = u16(10)) {
        config.minor_version = *value;
    }
    if (auto value = u32(20)) {
        config.critical_section_default_timeout = *value;
    }
    const std::uint32_t security_cookie_offset = storage_.optional.pe32_plus ? 88 : 60;
    const std::uint32_t seh_table_offset = storage_.optional.pe32_plus ? 96 : 64;
    const std::uint32_t seh_count_offset = storage_.optional.pe32_plus ? 104 : 68;
    const std::uint32_t guard_check_offset = storage_.optional.pe32_plus ? 112 : 72;
    const std::uint32_t guard_dispatch_offset = storage_.optional.pe32_plus ? 120 : 76;
    const std::uint32_t guard_table_offset = storage_.optional.pe32_plus ? 128 : 80;
    const std::uint32_t guard_count_offset = storage_.optional.pe32_plus ? 136 : 84;
    const std::uint32_t guard_flags_offset = storage_.optional.pe32_plus ? 144 : 88;
    // CodeIntegrity is a 12-byte field between GuardFlags and the fields below.
    // Related source:
    // ../../../Ghidra/Features/Base/src/main/java/ghidra/app/util/bin/format/pe/LoadConfigDirectory.java
    const std::uint32_t address_taken_table_offset = storage_.optional.pe32_plus ? 160 : 104;
    const std::uint32_t address_taken_count_offset = storage_.optional.pe32_plus ? 168 : 108;
    const std::uint32_t long_jump_table_offset = storage_.optional.pe32_plus ? 176 : 112;
    const std::uint32_t long_jump_count_offset = storage_.optional.pe32_plus ? 184 : 116;
    const std::uint32_t dynamic_reloc_offset = storage_.optional.pe32_plus ? 192 : 120;
    const std::uint32_t chpe_offset = storage_.optional.pe32_plus ? 200 : 124;
    const std::uint32_t guard_rf_failure_offset = storage_.optional.pe32_plus ? 208 : 128;
    const std::uint32_t guard_rf_pointer_offset = storage_.optional.pe32_plus ? 216 : 132;
    const std::uint32_t dynamic_reloc_table_offset = storage_.optional.pe32_plus ? 224 : 136;
    const std::uint32_t dynamic_reloc_section_offset = storage_.optional.pe32_plus ? 228 : 140;
    const std::uint32_t guard_rf_verify_offset = storage_.optional.pe32_plus ? 232 : 144;
    if (auto value = pointer(security_cookie_offset)) {
        config.security_cookie = *value;
    }
    if (auto value = pointer(seh_table_offset)) {
        config.se_handler_table = *value;
    }
    if (auto value = pointer(seh_count_offset)) {
        config.se_handler_count = *value;
    }
    if (auto value = pointer(guard_check_offset)) {
        config.guard_cf_check_function_pointer = *value;
    }
    if (auto value = pointer(guard_dispatch_offset)) {
        config.guard_cf_dispatch_function_pointer = *value;
    }
    if (auto value = pointer(guard_table_offset)) {
        config.guard_cf_function_table = *value;
    }
    if (auto value = pointer(guard_count_offset)) {
        config.guard_cf_function_count = *value;
    }
    if (auto value = u32(guard_flags_offset)) {
        config.guard_flags = *value;
    }
    if (auto value = pointer(address_taken_table_offset)) {
        config.guard_address_taken_iat_entry_table = *value;
    }
    if (auto value = pointer(address_taken_count_offset)) {
        config.guard_address_taken_iat_entry_count = *value;
    }
    if (auto value = pointer(long_jump_table_offset)) {
        config.guard_long_jump_target_table = *value;
    }
    if (auto value = pointer(long_jump_count_offset)) {
        config.guard_long_jump_target_count = *value;
    }
    if (auto value = pointer(dynamic_reloc_offset)) {
        config.dynamic_value_reloc_table = *value;
    }
    if (auto value = pointer(chpe_offset)) {
        config.chpe_metadata_pointer = *value;
    }
    if (auto value = pointer(guard_rf_failure_offset)) {
        config.guard_rf_failure_routine = *value;
    }
    if (auto value = pointer(guard_rf_pointer_offset)) {
        config.guard_rf_failure_routine_function_pointer = *value;
    }
    if (auto value = u32(dynamic_reloc_table_offset)) {
        config.dynamic_value_reloc_table_offset = *value;
    }
    if (auto value = u16(dynamic_reloc_section_offset)) {
        config.dynamic_value_reloc_table_section = *value;
    }
    if (auto value = pointer(guard_rf_verify_offset)) {
        config.guard_rf_verify_stack_pointer_function_pointer = *value;
    }
    storage_.load_config = std::move(config);
    return {};
}

/// Traverses IMAGE_RESOURCE_DIRECTORY nodes and records every resource leaf.
std::expected<void, ParseError> Parser::parse_resources(const DataDirectory& directory_value) {
    if (directory_value.size < 16) {
        return parse_failure<void>(ParseErrorCode::invalid_resource, directory_value.rva,
                                   "Resource directory is shorter than its root header");
    }
    auto root_characteristics = mapped_u32(directory_value.rva);
    auto root_timestamp = mapped_u32(directory_value.rva + 4);
    auto root_major = mapped_u16(directory_value.rva + 8);
    auto root_minor = mapped_u16(directory_value.rva + 10);
    auto root_named = mapped_u16(directory_value.rva + 12);
    auto root_ids = mapped_u16(directory_value.rva + 14);
    if (!root_characteristics || !root_timestamp || !root_major || !root_minor || !root_named || !root_ids) {
        return parse_failure<void>(ParseErrorCode::invalid_resource, directory_value.rva,
                                   "Resource root header is unmapped");
    }
    ResourceDirectory resources;
    resources.characteristics = *root_characteristics;
    resources.time_date_stamp = *root_timestamp;
    resources.major_version = *root_major;
    resources.minor_version = *root_minor;
    resources.number_of_named_entries = *root_named;
    resources.number_of_id_entries = *root_ids;
    std::unordered_set<std::uint32_t> active_directories;

    auto resource_string = [&](std::uint32_t relative) -> std::expected<std::string, ParseError> {
        if (relative > directory_value.size - 2) {
            return parse_failure<std::string>(ParseErrorCode::invalid_resource, directory_value.rva + relative,
                                              "Resource name offset is outside the directory");
        }
        auto length = mapped_u16(directory_value.rva + relative);
        if (!length || *length > (directory_value.size - relative - 2) / 2) {
            return parse_failure<std::string>(ParseErrorCode::invalid_resource, directory_value.rva + relative,
                                              "Resource UTF-16 name is truncated");
        }
        std::vector<std::uint16_t> code_units;
        code_units.reserve(*length);
        for (std::uint32_t index = 0; index < *length; ++index) {
            auto value = mapped_u16(directory_value.rva + relative + 2 + index * 2U);
            if (!value) {
                return parse_failure<std::string>(ParseErrorCode::invalid_resource, directory_value.rva + relative,
                                                  "Resource UTF-16 name is unmapped");
            }
            code_units.push_back(*value);
        }
        return utf16_to_utf8(code_units);
    };
    auto resource_identifier = [&](std::uint32_t raw) -> std::expected<ResourceIdentifier, ParseError> {
        ResourceIdentifier identifier;
        if ((raw & 0x80000000U) != 0) {
            auto name = resource_string(raw & 0x7fffffffU);
            if (!name) {
                return std::unexpected(name.error());
            }
            identifier.named = true;
            identifier.name = std::move(*name);
        } else {
            identifier.id = raw;
        }
        return identifier;
    };
    using Path = std::vector<ResourceIdentifier>;
    std::function<std::expected<void, ParseError>(std::uint32_t, const Path&, std::uint32_t)> walk;
    walk = [&](std::uint32_t relative, const Path& path, std::uint32_t depth) -> std::expected<void, ParseError> {
        if (depth > options_.maximum_resource_depth) {
            return parse_failure<void>(ParseErrorCode::limit_exceeded, directory_value.rva + relative,
                                       "Resource directory depth exceeds the configured limit");
        }
        if (relative > directory_value.size - 16 || !active_directories.insert(relative).second) {
            return parse_failure<void>(ParseErrorCode::invalid_resource, directory_value.rva + relative,
                                       "Resource directory contains an invalid or cyclic node");
        }
        auto named = mapped_u16(directory_value.rva + relative + 12);
        auto ids = mapped_u16(directory_value.rva + relative + 14);
        if (!named || !ids || static_cast<std::uint32_t>(*named) + *ids > (directory_value.size - relative - 16) / 8) {
            active_directories.erase(relative);
            return parse_failure<void>(ParseErrorCode::invalid_resource, directory_value.rva + relative,
                                       "Resource directory entry array is truncated");
        }
        const auto entry_count = static_cast<std::uint32_t>(*named) + *ids;
        for (std::uint32_t index = 0; index < entry_count; ++index) {
            const auto entry_relative = relative + 16 + index * 8U;
            auto raw_name = mapped_u32(directory_value.rva + entry_relative);
            auto raw_data = mapped_u32(directory_value.rva + entry_relative + 4);
            if (!raw_name || !raw_data) {
                active_directories.erase(relative);
                return parse_failure<void>(ParseErrorCode::invalid_resource, directory_value.rva + entry_relative,
                                           "Resource directory entry is unmapped");
            }
            auto identifier = resource_identifier(*raw_name);
            if (!identifier) {
                active_directories.erase(relative);
                return std::unexpected(identifier.error());
            }
            Path next_path = path;
            next_path.push_back(std::move(*identifier));
            if ((*raw_data & 0x80000000U) != 0) {
                auto child = walk(*raw_data & 0x7fffffffU, next_path, depth + 1);
                if (!child) {
                    active_directories.erase(relative);
                    return std::unexpected(child.error());
                }
                continue;
            }
            const auto data_relative = *raw_data;
            if (data_relative > directory_value.size - 16) {
                active_directories.erase(relative);
                return parse_failure<void>(ParseErrorCode::invalid_resource, directory_value.rva + data_relative,
                                           "Resource data entry is outside the directory");
            }
            auto data_rva = mapped_u32(directory_value.rva + data_relative);
            auto data_size = mapped_u32(directory_value.rva + data_relative + 4);
            auto code_page = mapped_u32(directory_value.rva + data_relative + 8);
            auto reserved = mapped_u32(directory_value.rva + data_relative + 12);
            if (!data_rva || !data_size || !code_page || !reserved || next_path.empty()) {
                active_directories.erase(relative);
                return parse_failure<void>(ParseErrorCode::invalid_resource, directory_value.rva + data_relative,
                                           "Resource data entry is malformed");
            }
            ResourceLeaf leaf;
            leaf.type = next_path.size() > 0 ? next_path[0] : ResourceIdentifier{};
            leaf.name = next_path.size() > 1 ? next_path[1] : ResourceIdentifier{};
            leaf.language = next_path.size() > 2 ? next_path[2] : ResourceIdentifier{};
            leaf.data_rva = *data_rva;
            leaf.size = *data_size;
            leaf.code_page = *code_page;
            leaf.reserved = *reserved;
            if (leaf.size != 0) {
                if (leaf.data_rva > storage_.optional.size_of_image ||
                    leaf.size > storage_.optional.size_of_image - leaf.data_rva) {
                    active_directories.erase(relative);
                    return parse_failure<void>(ParseErrorCode::invalid_resource, leaf.data_rva,
                                               "Resource payload exceeds SizeOfImage");
                }
                leaf.file_offset = raw_offset_for_rva(leaf.data_rva);
                if (!leaf.file_offset || !valid_range(*leaf.file_offset, leaf.size, reader_.bytes().size())) {
                    active_directories.erase(relative);
                    return parse_failure<void>(ParseErrorCode::invalid_resource, leaf.data_rva,
                                               "Resource payload is not file-backed");
                }
            }
            resources.leaves.push_back(std::move(leaf));
        }
        active_directories.erase(relative);
        return {};
    };
    auto result = walk(0, {}, 0);
    if (!result) {
        return std::unexpected(result.error());
    }
    storage_.resources = std::move(resources);
    return {};
}

/// Parses WIN_CERTIFICATE records from the file-offset security directory.
std::expected<void, ParseError> Parser::parse_security(const DataDirectory& directory_value) {
    std::uint32_t offset = 0;
    while (offset < directory_value.size) {
        const FileOffset certificate_offset = static_cast<FileOffset>(directory_value.rva) + offset;
        if (directory_value.size - offset < 8 || !valid_range(certificate_offset, 8, reader_.bytes().size())) {
            return parse_failure<void>(ParseErrorCode::invalid_certificate, certificate_offset,
                                       "Certificate header is truncated");
        }
        auto length = reader_.u32(certificate_offset, "certificate length");
        auto revision = reader_.u16(certificate_offset + 4, "certificate revision");
        auto type = reader_.u16(certificate_offset + 6, "certificate type");
        if (!length || !revision || !type || *length < 8 || *length > directory_value.size - offset ||
            !valid_range(certificate_offset, *length, reader_.bytes().size())) {
            return parse_failure<void>(ParseErrorCode::invalid_certificate, certificate_offset,
                                       "Certificate length is invalid");
        }
        SecurityCertificate certificate;
        certificate.length = *length;
        certificate.revision = *revision;
        certificate.type = *type;
        certificate.file_offset = certificate_offset;
        auto bytes = reader_.span(certificate.file_offset + 8, *length - 8, "certificate bytes");
        if (!bytes) {
            return std::unexpected(bytes.error());
        }
        certificate.certificate_bytes.assign(bytes->begin(), bytes->end());
        storage_.certificates.push_back(std::move(certificate));
        const auto aligned_length = (static_cast<std::uint64_t>(*length) + 7U) & ~7ULL;
        if (aligned_length > directory_value.size - offset) {
            if (offset + *length == directory_value.size) {
                break;
            }
            return parse_failure<void>(ParseErrorCode::invalid_certificate, certificate_offset,
                                       "Certificate alignment exceeds the directory");
        }
        offset += static_cast<std::uint32_t>(aligned_length);
    }
    return {};
}

/// Parses IMAGE_BOUND_IMPORT_DESCRIPTOR records and their forwarder references.
std::expected<void, ParseError> Parser::parse_bound_imports(const DataDirectory& directory_value) {
    std::uint32_t offset = 0;
    while (offset + 8 <= directory_value.size) {
        auto timestamp = mapped_u32(directory_value.rva + offset);
        auto name_offset = mapped_u16(directory_value.rva + offset + 4);
        auto forwarder_count = mapped_u16(directory_value.rva + offset + 6);
        if (!timestamp || !name_offset || !forwarder_count) {
            return parse_failure<void>(ParseErrorCode::invalid_directory, directory_value.rva + offset,
                                       "Bound-import descriptor is unmapped");
        }
        if (*timestamp == 0 && *name_offset == 0 && *forwarder_count == 0) {
            return {};
        }
        if (*name_offset >= directory_value.size) {
            return parse_failure<void>(ParseErrorCode::invalid_directory, directory_value.rva + offset,
                                       "Bound-import module name is outside the directory");
        }
        auto module_name = mapped_string(directory_value.rva + *name_offset, directory_value.size - *name_offset,
                                         "bound-import module name");
        if (!module_name) {
            return std::unexpected(module_name.error());
        }
        BoundImport descriptor;
        descriptor.time_date_stamp = *timestamp;
        descriptor.module_name_offset = *name_offset;
        descriptor.number_of_forwarder_refs = *forwarder_count;
        descriptor.module_name = std::move(*module_name);
        const auto refs_offset = offset + 8;
        if (*forwarder_count > (directory_value.size - refs_offset) / 8) {
            return parse_failure<void>(ParseErrorCode::invalid_directory, directory_value.rva + refs_offset,
                                       "Bound-import forwarder references are truncated");
        }
        for (std::uint32_t ref_index = 0; ref_index < *forwarder_count; ++ref_index) {
            const auto ref = refs_offset + ref_index * 8U;
            auto ref_name_offset = mapped_u16(directory_value.rva + ref + 4);
            if (!ref_name_offset || *ref_name_offset >= directory_value.size) {
                return parse_failure<void>(ParseErrorCode::invalid_directory, directory_value.rva + ref,
                                           "Bound-import forwarder name is invalid");
            }
            auto ref_name = mapped_string(directory_value.rva + *ref_name_offset,
                                          directory_value.size - *ref_name_offset, "bound-import forwarder name");
            if (!ref_name) {
                return std::unexpected(ref_name.error());
            }
            descriptor.forwarder_modules.push_back(std::move(*ref_name));
        }
        storage_.bound_imports.push_back(std::move(descriptor));
        offset = refs_offset + *forwarder_count * 8U;
    }
    return {};
}

/// Parses delay-import descriptors in RVA or absolute-VA mode.
std::expected<void, ParseError> Parser::parse_delay_imports(const DataDirectory& directory_value) {
    constexpr std::uint32_t descriptor_size = 32;
    bool terminated = false;
    auto resolve = [&](std::uint32_t value, bool uses_rva) -> std::optional<Rva> {
        return uses_rva ? (value < storage_.optional.size_of_image ? std::optional<Rva>(value) : std::nullopt)
                        : rva_for_va(value);
    };
    for (std::uint32_t offset = 0; directory_value.size - offset >= descriptor_size; offset += descriptor_size) {
        const auto rva = directory_value.rva + offset;
        auto attributes = mapped_u32(rva);
        auto name = mapped_u32(rva + 4);
        auto module_handle = mapped_u32(rva + 8);
        auto iat = mapped_u32(rva + 12);
        auto int_table = mapped_u32(rva + 16);
        auto bound_iat = mapped_u32(rva + 20);
        auto unload_iat = mapped_u32(rva + 24);
        auto timestamp = mapped_u32(rva + 28);
        if (!attributes || !name || !module_handle || !iat || !int_table || !bound_iat || !unload_iat || !timestamp) {
            return parse_failure<void>(ParseErrorCode::invalid_directory, rva, "Delay-import descriptor is unmapped");
        }
        if (*attributes == 0 && *name == 0 && *module_handle == 0 && *iat == 0 && *int_table == 0 && *bound_iat == 0 &&
            *unload_iat == 0 && *timestamp == 0) {
            terminated = true;
            break;
        }
        const bool uses_rva = (*attributes & 1U) != 0;
        auto name_rva = resolve(*name, uses_rva);
        auto iat_rva = resolve(*iat, uses_rva);
        auto int_rva = resolve(*int_table, uses_rva);
        const auto resolve_optional = [&](std::uint32_t value) -> std::optional<Rva> {
            return value == 0 ? std::optional<Rva>(0) : resolve(value, uses_rva);
        };
        auto module_handle_rva = resolve_optional(*module_handle);
        auto bound_iat_rva = resolve_optional(*bound_iat);
        auto unload_iat_rva = resolve_optional(*unload_iat);
        if (!name_rva || !iat_rva || !int_rva || !module_handle_rva || !bound_iat_rva || !unload_iat_rva) {
            return parse_failure<void>(ParseErrorCode::invalid_directory, rva,
                                       "Delay-import descriptor contains an invalid RVA/VA");
        }
        auto dll_name = mapped_string(*name_rva, 1U << 16, "delay-import DLL name");
        if (!dll_name) {
            return std::unexpected(dll_name.error());
        }
        DelayImportDescriptor descriptor;
        descriptor.attributes = *attributes;
        descriptor.dll_name = std::move(*dll_name);
        descriptor.name_rva = *name_rva;
        descriptor.module_handle_rva = *module_handle_rva;
        descriptor.import_address_table_rva = *iat_rva;
        descriptor.import_name_table_rva = *int_rva;
        descriptor.bound_import_address_table_rva = *bound_iat_rva;
        descriptor.unload_information_table_rva = *unload_iat_rva;
        descriptor.time_date_stamp = *timestamp;
        descriptor.uses_rva = uses_rva;
        auto thunks =
            parse_thunks(descriptor.symbols, *int_rva, *iat_rva, options_.maximum_directory_entries, !uses_rva);
        if (!thunks) {
            return std::unexpected(thunks.error());
        }
        storage_.delay_imports.push_back(std::move(descriptor));
    }
    if (!terminated && options_.strict) {
        return parse_failure<void>(ParseErrorCode::invalid_directory, directory_value.rva + directory_value.size,
                                   "Delay-import descriptor array has no null terminator");
    }
    return {};
}

/// Parses the fixed IMAGE_COR20_HEADER used to identify managed PE files.
std::expected<void, ParseError> Parser::parse_clr(const DataDirectory& directory_value) {
    // Related source: ../../../Ghidra/Features/Base/src/main/java/ghidra/app/util/bin/format/pe/ImageCor20Header.java
    // The Java model exposes the fixed 72-byte IMAGE_COR20_HEADER, while the cb field is the
    // authoritative bound for newer or malformed headers. Never decode fields past either bound.
    if (directory_value.size < 4) {
        return parse_failure<void>(ParseErrorCode::invalid_directory, directory_value.rva,
                                   "CLR directory lacks its cb field");
    }
    auto size = mapped_u32(directory_value.rva);
    if (!size || *size < 72 || directory_value.size < 72) {
        return parse_failure<void>(ParseErrorCode::invalid_directory, directory_value.rva,
                                   "CLR cb does not declare the complete IMAGE_COR20_HEADER");
    }
    const auto available = std::min(*size, directory_value.size);
    auto bounded_u16 = [&](std::uint32_t offset) -> std::optional<std::uint16_t> {
        if (offset > available || 2 > available - offset ||
            directory_value.rva > std::numeric_limits<Rva>::max() - offset) {
            return std::nullopt;
        }
        return mapped_u16(directory_value.rva + offset);
    };
    auto bounded_u32 = [&](std::uint32_t offset) -> std::optional<std::uint32_t> {
        if (offset > available || 4 > available - offset ||
            directory_value.rva > std::numeric_limits<Rva>::max() - offset) {
            return std::nullopt;
        }
        return mapped_u32(directory_value.rva + offset);
    };
    auto major = bounded_u16(4);
    auto minor = bounded_u16(6);
    auto metadata_rva = bounded_u32(8);
    auto metadata_size = bounded_u32(12);
    auto flags = bounded_u32(16);
    auto entry_point = bounded_u32(20);
    auto resources_rva = bounded_u32(24);
    auto resources_size = bounded_u32(28);
    auto strong_name_rva = bounded_u32(32);
    auto strong_name_size = bounded_u32(36);
    auto code_manager_rva = bounded_u32(40);
    auto code_manager_size = bounded_u32(44);
    auto vtable_rva = bounded_u32(48);
    auto vtable_size = bounded_u32(52);
    auto export_jumps_rva = bounded_u32(56);
    auto export_jumps_size = bounded_u32(60);
    auto native_header_rva = bounded_u32(64);
    auto native_header_size = bounded_u32(68);
    if (!major || !minor || !metadata_rva || !metadata_size || !flags || !entry_point || !resources_rva ||
        !resources_size || !strong_name_rva || !strong_name_size || !code_manager_rva || !code_manager_size ||
        !vtable_rva || !vtable_size || !export_jumps_rva || !export_jumps_size || !native_header_rva ||
        !native_header_size) {
        return parse_failure<void>(ParseErrorCode::invalid_directory, directory_value.rva, "CLR header is unmapped");
    }
    ClrHeader clr;
    clr.size = *size;
    clr.major_runtime_version = *major;
    clr.minor_runtime_version = *minor;
    clr.metadata_rva = *metadata_rva;
    clr.metadata_size = *metadata_size;
    clr.flags = *flags;
    clr.entry_point_is_native = (*flags & 0x10U) != 0;
    clr.entry_point_token_or_rva = *entry_point;
    clr.resources_rva = *resources_rva;
    clr.resources_size = *resources_size;
    clr.strong_name_signature_rva = *strong_name_rva;
    clr.strong_name_signature_size = *strong_name_size;
    clr.code_manager_table_rva = *code_manager_rva;
    clr.code_manager_table_size = *code_manager_size;
    clr.vtable_fixups_rva = *vtable_rva;
    clr.vtable_fixups_size = *vtable_size;
    clr.export_address_table_jumps_rva = *export_jumps_rva;
    clr.export_address_table_jumps_size = *export_jumps_size;
    clr.managed_native_header_rva = *native_header_rva;
    clr.managed_native_header_size = *native_header_size;
    storage_.clr_header = std::move(clr);
    return {};
}

/// Parses the optional COFF symbol array and its trailing string table.
std::expected<void, ParseError> Parser::parse_coff_symbols() {
    if (storage_.coff.pointer_to_symbol_table == 0 || storage_.coff.number_of_symbols == 0) {
        return {};
    }
    constexpr std::uint64_t symbol_size = 18;
    if (storage_.coff.number_of_symbols > options_.maximum_directory_entries ||
        multiply_overflow(storage_.coff.number_of_symbols, symbol_size) ||
        !valid_range(storage_.coff.pointer_to_symbol_table,
                     static_cast<std::uint64_t>(storage_.coff.number_of_symbols) * symbol_size,
                     reader_.bytes().size())) {
        return parse_failure<void>(ParseErrorCode::invalid_directory, storage_.coff.pointer_to_symbol_table,
                                   "COFF symbol table is outside the input or exceeds the configured limit");
    }
    const FileOffset string_table_offset = storage_.coff.pointer_to_symbol_table +
                                           static_cast<std::uint64_t>(storage_.coff.number_of_symbols) * symbol_size;
    auto string_table_size = reader_.u32(string_table_offset, "COFF string-table size");
    if (!string_table_size || *string_table_size < 4 ||
        !valid_range(string_table_offset, *string_table_size, reader_.bytes().size())) {
        return parse_failure<void>(ParseErrorCode::invalid_directory, string_table_offset,
                                   "COFF string table is truncated");
    }
    auto read_symbol_name = [&](FileOffset offset) -> std::expected<std::string, ParseError> {
        auto name_bytes = reader_.span(offset, 8, "COFF symbol name");
        if (!name_bytes) {
            return std::unexpected(name_bytes.error());
        }
        const auto zero = std::find(name_bytes->begin(), name_bytes->begin() + 4, Byte{0});
        if (zero != name_bytes->begin()) {
            const auto end = std::find(name_bytes->begin(), name_bytes->end(), Byte{0});
            return std::string(reinterpret_cast<const char*>(name_bytes->data()),
                               static_cast<std::size_t>(std::distance(name_bytes->begin(), end)));
        }
        auto string_offset = reader_.u32(offset + 4, "COFF long-name offset");
        if (!string_offset || *string_offset < 4 || *string_offset >= *string_table_size) {
            return parse_failure<std::string>(ParseErrorCode::invalid_directory, offset,
                                              "COFF symbol string offset is invalid");
        }
        return read_file_string(reader_, string_table_offset + *string_offset - 4, *string_table_size - *string_offset,
                                "COFF symbol name");
    };
    for (std::uint32_t index = 0; index < storage_.coff.number_of_symbols;) {
        const FileOffset symbol_offset =
            storage_.coff.pointer_to_symbol_table + static_cast<std::uint64_t>(index) * symbol_size;
        auto name = read_symbol_name(symbol_offset);
        auto value = reader_.u32(symbol_offset + 8, "COFF symbol value");
        auto section_number = reader_.u16(symbol_offset + 12, "COFF symbol section");
        auto type = reader_.u16(symbol_offset + 14, "COFF symbol type");
        auto storage_class = reader_.u8(symbol_offset + 16, "COFF symbol storage class");
        auto auxiliary_count = reader_.u8(symbol_offset + 17, "COFF symbol auxiliary count");
        if (!name || !value || !section_number || !type || !storage_class || !auxiliary_count) {
            return parse_failure<void>(ParseErrorCode::invalid_directory, symbol_offset,
                                       "COFF symbol record is malformed");
        }
        CoffSymbol symbol;
        symbol.name = std::move(*name);
        symbol.value = *value;
        symbol.section_number = static_cast<std::int16_t>(*section_number);
        symbol.type = *type;
        symbol.storage_class = *storage_class;
        symbol.auxiliary_count = *auxiliary_count;
        symbol.file_offset = symbol_offset;
        storage_.coff_symbols.push_back(std::move(symbol));
        if (*auxiliary_count > storage_.coff.number_of_symbols - index - 1) {
            return parse_failure<void>(ParseErrorCode::invalid_directory, symbol_offset + 17,
                                       "COFF auxiliary-symbol count exceeds the table");
        }
        index += 1 + *auxiliary_count;
    }
    for (auto& section : storage_.sections) {
        if (section.name.size() > 1 && section.name.front() == '/') {
            try {
                const auto string_offset = static_cast<std::uint32_t>(std::stoul(section.name.substr(1)));
                if (string_offset >= 4 && string_offset < *string_table_size) {
                    auto name = read_file_string(reader_, string_table_offset + string_offset - 4,
                                                 *string_table_size - string_offset, "COFF section name");
                    if (name) {
                        section.name = std::move(*name);
                    }
                }
            } catch (const std::exception&) {
                return parse_failure<void>(ParseErrorCode::invalid_directory, section.raw_offset,
                                           "COFF section name offset is not numeric");
            }
        }
    }
    return {};
}

/// Runs the complete PE parse pipeline and returns an owning image.
std::expected<LoadedPeImage, ParseError> Parser::run() {
    try {
        storage_.file.assign(reader_.bytes().begin(), reader_.bytes().end());
    } catch (const std::bad_alloc&) {
        return parse_failure<LoadedPeImage>(ParseErrorCode::limit_exceeded, 0, "Unable to retain the PE input bytes");
    }
    auto headers = parse_headers();
    if (!headers) {
        return std::unexpected(headers.error());
    }
    auto symbols = parse_coff_symbols();
    if (!symbols) {
        if (options_.strict) {
            return std::unexpected(symbols.error());
        }
        record_partial(symbols.error());
    }
    auto mapping = map_image();
    if (!mapping) {
        return std::unexpected(mapping.error());
    }
    if (options_.parse_directories) {
        auto directories = parse_directories();
        if (!directories) {
            return std::unexpected(directories.error());
        }
    }
    return LoadedPeImage(std::make_unique<LoadedPeImage::Storage>(std::move(storage_)));
}

} // namespace

/// Converts an RVA backed by file bytes to its raw file offset.
std::expected<FileOffset, AddressError> LoadedPeImage::rva_to_file_offset(Rva rva) const {
    if (rva >= storage_->optional.size_of_image) {
        return std::unexpected(AddressError{AddressErrorCode::outside_image, rva, 1, "RVA is outside SizeOfImage"});
    }
    if (rva < storage_->optional.size_of_headers && rva < storage_->file.size()) {
        return static_cast<FileOffset>(rva);
    }
    for (const auto& section : storage_->sections) {
        if (section.file_backed_size != 0 && rva >= section.virtual_address &&
            rva - section.virtual_address < section.loaded_size) {
            const auto relative = rva - section.virtual_address;
            if (relative >= section.file_backed_size) {
                return std::unexpected(
                    AddressError{AddressErrorCode::not_file_backed, rva, 1, "RVA is in a virtual-only section tail"});
            }
            return section.raw_offset + relative;
        }
    }
    return std::unexpected(
        AddressError{AddressErrorCode::unmapped, rva, 1, "RVA is not covered by PE headers or a section"});
}

/// Converts a raw file offset to an RVA when the offset is mapped into the image.
std::expected<Rva, AddressError> LoadedPeImage::file_offset_to_rva(FileOffset offset) const {
    if (offset < storage_->optional.size_of_headers && offset < storage_->file.size()) {
        return static_cast<Rva>(offset);
    }
    for (const auto& section : storage_->sections) {
        if (section.file_backed_size != 0 && offset >= section.raw_offset &&
            offset - section.raw_offset < section.file_backed_size) {
            const auto relative = offset - section.raw_offset;
            if (relative > std::numeric_limits<Rva>::max() - section.virtual_address) {
                return std::unexpected(
                    AddressError{AddressErrorCode::overflow, offset, 1, "File offset translation overflows an RVA"});
            }
            return section.virtual_address + static_cast<Rva>(relative);
        }
    }
    return std::unexpected(AddressError{AddressErrorCode::unmapped, offset, 1,
                                        "File offset is not covered by PE headers or section raw data"});
}

/// Converts an in-image RVA to a preferred-image virtual address.
std::expected<Va, AddressError> LoadedPeImage::rva_to_va(Rva rva) const {
    if (rva >= storage_->optional.size_of_image ||
        storage_->optional.image_base > std::numeric_limits<Va>::max() - rva) {
        return std::unexpected(
            AddressError{AddressErrorCode::outside_image, rva, 1, "RVA is outside the loaded image"});
    }
    return storage_->optional.image_base + rva;
}

/// Converts a preferred-image virtual address to an in-image RVA.
std::expected<Rva, AddressError> LoadedPeImage::va_to_rva(Va va) const {
    if (va < storage_->optional.image_base) {
        return std::unexpected(
            AddressError{AddressErrorCode::outside_image, va, 1, "Virtual address precedes the image base"});
    }
    const auto delta = va - storage_->optional.image_base;
    if (delta >= storage_->optional.size_of_image || delta > std::numeric_limits<Rva>::max()) {
        return std::unexpected(
            AddressError{AddressErrorCode::outside_image, va, 1, "Virtual address is outside SizeOfImage"});
    }
    return static_cast<Rva>(delta);
}

/// Reads a contiguous range from one mapped PE memory region.
std::expected<std::vector<Byte>, MemoryError> LoadedPeImage::read_memory(Va address, std::uint64_t size) const {
    if (size > std::numeric_limits<std::size_t>::max()) {
        return std::unexpected(
            MemoryError{MemoryErrorCode::overflow, address, size, "Requested memory range is too large"});
    }
    auto rva = va_to_rva(address);
    if (!rva || size > storage_->optional.size_of_image - *rva) {
        return std::unexpected(MemoryError{MemoryErrorCode::outside_image, address, size,
                                           "Requested memory range is outside SizeOfImage"});
    }
    for (const auto& region : storage_->memory_regions) {
        const auto delta = address >= region.start ? address - region.start : 0;
        if (address >= region.start && delta <= region.size && size <= region.size - delta) {
            const auto image_offset = static_cast<std::size_t>(*rva);
            return std::vector<Byte>(storage_->mapped_image.begin() + image_offset,
                                     storage_->mapped_image.begin() + image_offset + static_cast<std::size_t>(size));
        }
    }
    return std::unexpected(
        MemoryError{MemoryErrorCode::unmapped, address, size, "Requested range crosses an unmapped image gap"});
}

/// Reads one byte from a mapped PE memory region.
std::expected<Byte, MemoryError> LoadedPeImage::read_byte(Va address) const {
    auto bytes = read_memory(address, 1);
    if (!bytes) {
        return std::unexpected(bytes.error());
    }
    return bytes->front();
}

/// Reads the exact bytes described by a parsed resource data entry.
std::expected<std::vector<Byte>, MemoryError> LoadedPeImage::read_resource_payload(const ResourceLeaf& leaf) const {
    if (leaf.size == 0) {
        return std::vector<Byte>{};
    }
    auto address = rva_to_va(leaf.data_rva);
    if (!address) {
        return std::unexpected(MemoryError{MemoryErrorCode::outside_image, leaf.data_rva, leaf.size,
                                           "Resource payload RVA is outside the loaded image"});
    }
    return read_memory(*address, leaf.size);
}

/// Reads the exact bytes described by one leaf in the parsed resource tree.
std::expected<std::vector<Byte>, MemoryError> LoadedPeImage::read_resource_payload(std::size_t leaf_index) const {
    if (!storage_->resources || leaf_index >= storage_->resources->leaves.size()) {
        return std::unexpected(MemoryError{MemoryErrorCode::outside_image, 0, 0,
                                           "Resource leaf index is outside the parsed resource tree"});
    }
    return read_resource_payload(storage_->resources->leaves[leaf_index]);
}

/// Parses a byte span into an owned loaded PE image.
std::expected<LoadedPeImage, ParseError> PeLoader::load(std::span<const Byte> bytes, const LoadOptions& options) {
    try {
        return Parser(bytes, options).run();
    } catch (const std::bad_alloc&) {
        return parse_failure<LoadedPeImage>(ParseErrorCode::limit_exceeded, 0, "PE parsing exceeded available memory");
    }
}

/// Reads a PE file from disk and passes its bytes through the checked parser.
std::expected<LoadedPeImage, ParseError> PeLoader::load_file(const std::filesystem::path& path,
                                                             const LoadOptions& options) {
    std::ifstream input(path, std::ios::binary | std::ios::ate);
    if (!input) {
        return parse_failure<LoadedPeImage>(ParseErrorCode::io_failure, 0, "Unable to open PE file: " + path.string());
    }
    const auto end = input.tellg();
    if (end < 0) {
        return parse_failure<LoadedPeImage>(ParseErrorCode::io_failure, 0,
                                            "Unable to determine PE file size: " + path.string());
    }
    const auto size = static_cast<std::uint64_t>(end);
    if (size > std::numeric_limits<std::size_t>::max()) {
        return parse_failure<LoadedPeImage>(ParseErrorCode::limit_exceeded, 0, "PE file is too large for this process");
    }
    std::vector<Byte> bytes;
    try {
        bytes.resize(static_cast<std::size_t>(size));
    } catch (const std::bad_alloc&) {
        return parse_failure<LoadedPeImage>(ParseErrorCode::limit_exceeded, 0,
                                            "PE file is too large to retain in memory");
    }
    input.seekg(0);
    if (!bytes.empty()) {
        input.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
        if (!input) {
            return parse_failure<LoadedPeImage>(ParseErrorCode::io_failure, 0,
                                                "Unable to read PE file: " + path.string());
        }
    }
    return load(bytes, options);
}
} // namespace pe
