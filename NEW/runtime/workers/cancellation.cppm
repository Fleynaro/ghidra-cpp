export module ghidra.runtime.workers.cancellation;

export import ghidra.core.contracts.operation;

export namespace ghidra::runtime::workers {

/// Provides the runtime name for the core cooperative cancellation token.
using CancellationToken = ghidra::core::contracts::CancellationToken;

} // namespace ghidra::runtime::workers
