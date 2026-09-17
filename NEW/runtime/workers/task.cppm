export module recode.runtime.workers.task;

import recode.core.contracts.operation;

export namespace recode::runtime::workers {

/// Re-exports the stable task value used by runtime worker implementations.
template <class T> using Task = recode::core::contracts::Task<T>;

} // namespace recode::runtime::workers
