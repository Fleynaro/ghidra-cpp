export module ghidra.runtime.workers.task;

import ghidra.core.contracts.operation;

export namespace ghidra::runtime::workers {

/// Re-exports the stable task value used by runtime worker implementations.
template <class T> using Task = ghidra::core::contracts::Task<T>;

} // namespace ghidra::runtime::workers
