export module recode.bindings.cpp.results;

export import recode.runtime.project.state;
export import recode.core.decompilation;

export namespace recode::bindings::cpp {

/// Names the stable result values returned by native project operations.
using LoadResult = recode::runtime::project::LoadSummary;
/// Names the stable analysis result value.
using AnalysisResult = recode::runtime::project::AnalysisSummary;
/// Names the stable decompilation result value.
using DecompilationResult = recode::core::Decompilation;

} // namespace recode::bindings::cpp
