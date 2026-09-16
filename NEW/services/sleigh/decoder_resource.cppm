export module ghidra.service.sleigh.decoder_resource;

import std;
import ghidra.core;

export namespace ghidra::services::sleigh {

namespace core = ghidra::core;

/// Validates and identifies one compiled SLA before a decoder is created.
class DecoderResource final {
public:
    /// Opens only the resource metadata and leaves native decoder ownership to `SleighService`.
    static core::Result<DecoderResource> open(std::filesystem::path path) {
        std::error_code error;
        if (!std::filesystem::is_regular_file(path, error) || error)
            return std::unexpected(core::Error::make(core::DiagnosticCode::resource_unavailable,
                                                     "Compiled SLA resource does not exist: " + path.string()));
        return DecoderResource(std::move(path), std::filesystem::file_size(path, error));
    }

    /// Returns the validated SLA path.
    [[nodiscard]] const std::filesystem::path& path() const noexcept {
        return path_;
    }

    /// Returns the immutable resource byte size.
    [[nodiscard]] std::uint64_t size() const noexcept {
        return size_;
    }

private:
    /// Stores validated resource metadata.
    DecoderResource(std::filesystem::path path, std::uint64_t size) : path_(std::move(path)), size_(size) {}

    std::filesystem::path path_;
    std::uint64_t size_{};
};

} // namespace ghidra::services::sleigh
