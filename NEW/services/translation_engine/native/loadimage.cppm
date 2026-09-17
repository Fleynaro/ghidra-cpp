export module recode.service.translation_engine.native.loadimage;

export import recode.core.contracts.memory_provider;

export namespace recode::services::translation_engine::native {

/// Keeps native load-image access behind the core memory-provider contract.
using LoadImageProvider = recode::core::contracts::IMemoryProvider;

} // namespace recode::services::translation_engine::native
