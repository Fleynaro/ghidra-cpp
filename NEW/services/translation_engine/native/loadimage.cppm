export module ghidra.service.translation_engine.native.loadimage;

export import ghidra.core.contracts.memory_provider;

export namespace ghidra::services::translation_engine::native {

/// Keeps native load-image access behind the core memory-provider contract.
using LoadImageProvider = ghidra::core::contracts::IMemoryProvider;

} // namespace ghidra::services::translation_engine::native
