export module ghidra.bindings.cpp.results;

export import ghidra.runtime.project.state;
export import ghidra.core.decompilation;

export namespace ghidra::bindings::cpp {

/// Names the stable result values returned by native project operations.
using LoadResult = ghidra::runtime::project::LoadSummary;
/// Names the stable analysis result value.
using AnalysisResult = ghidra::runtime::project::AnalysisSummary;
/// Names the stable decompilation result value.
using DecompilationResult = ghidra::core::Decompilation;

} // namespace ghidra::bindings::cpp
