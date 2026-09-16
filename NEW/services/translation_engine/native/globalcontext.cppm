export module ghidra.service.translation_engine.native.globalcontext;

export import ghidra.core.architecture;

export namespace ghidra::services::translation_engine::native {

/// Canonical architecture metadata consumed by native adapters.
using GlobalArchitectureContext = ghidra::core::ArchitectureDescription;

} // namespace ghidra::services::translation_engine::native
