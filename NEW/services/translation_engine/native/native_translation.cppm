export module ghidra.service.translation_engine.native;

import std;
import ghidra.core;

export namespace ghidra::services::translation_engine {

/// Describes the shared native translation boundary used by Sleigh/decompiler adapters.
struct NativeTranslationEngine {
    std::string language_id;
    std::string compiler_spec_id;
    std::vector<core::AddressSpaceDescriptor> spaces;

    /// Reports whether the shared boundary knows a named address space.
    [[nodiscard]] bool has_space(std::string_view name) const noexcept {
        return std::ranges::any_of(spaces, [&](const auto& space) { return space.id.name() == name; });
    }
};

} // namespace ghidra::services::translation_engine
