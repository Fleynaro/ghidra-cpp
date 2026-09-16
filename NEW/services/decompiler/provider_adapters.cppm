export module ghidra.service.decompiler.provider_adapters;

export import ghidra.core.contracts.decompiler;

export namespace ghidra::services::decompiler {

/// Names the provider bundle passed to each isolated native decompiler task.
using ProviderAdapters = ghidra::core::contracts::ProviderContext;

} // namespace ghidra::services::decompiler
