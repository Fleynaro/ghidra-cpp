export module ghidra.service.translation_engine.native.marshal;

import std;

export namespace ghidra::services::translation_engine::native {

/// Provides endian-neutral unsigned integer marshaling for native adapters.
class Marshal final {
public:
    /// Appends a little-endian unsigned value to an output byte string.
    static void append_u64(std::string& output, std::uint64_t value) {
        for (std::size_t index = 0; index < sizeof(value); ++index)
            output.push_back(static_cast<char>((value >> (index * 8)) & 0xffU));
    }
};

} // namespace ghidra::services::translation_engine::native
