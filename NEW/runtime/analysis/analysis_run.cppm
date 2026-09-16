export module ghidra.runtime.analysis.run;

import std;
import ghidra.core;

export namespace ghidra::runtime::analysis {

/// Stores durable lifecycle values for one analysis run.
struct AnalysisRunState {
    core::AnalysisRunId id;
    core::ProjectId project;
    core::Revision read_revision;
    core::Revision committed_revision;
    std::string status;
    std::vector<std::string> analyzers;
};

} // namespace ghidra::runtime::analysis
