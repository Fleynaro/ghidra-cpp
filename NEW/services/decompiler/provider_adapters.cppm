export module recode.service.decompiler.provider_adapters;

export import recode.core.contracts.decompiler;

export namespace recode::services::decompiler {

/// Names the provider bundle passed to each isolated native decompiler task.
using ProviderAdapters = recode::core::contracts::ProviderContext;

} // namespace recode::services::decompiler
