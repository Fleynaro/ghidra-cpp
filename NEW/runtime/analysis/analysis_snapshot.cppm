export module ghidra.runtime.analysis.snapshot;

export import ghidra.core.contracts.analyzer;

export namespace ghidra::runtime::analysis {

/// Names the runtime-owned snapshot builder result without duplicating core state.
using AnalysisSnapshot = ghidra::core::contracts::AnalysisSnapshot;

} // namespace ghidra::runtime::analysis
