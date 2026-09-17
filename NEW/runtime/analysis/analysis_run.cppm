export module recode.runtime.analysis.run;

import std;
import recode.core;

export namespace recode::runtime::analysis {

/// Stores durable lifecycle values for one analysis run.
struct AnalysisRunState {
    core::AnalysisRunId id;
    core::ProjectId project;
    core::Revision read_revision;
    core::Revision committed_revision;
    std::string status;
    std::vector<std::string> analyzers;
};

} // namespace recode::runtime::analysis
