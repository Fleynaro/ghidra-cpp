export module ghidra.runtime.project.state;

import std;
import ghidra.core;

export namespace ghidra::runtime::project {

/// Represents the externally visible project lifecycle state.
enum class ProjectStatus : std::uint8_t {
    unopened,
    opening,
    open,
    loading,
    ready,
    analyzing,
    closing,
    closed,
    recovery_required,
    failed,
};

/// Stores lifecycle status and revision without borrowing runtime objects.
struct ProjectState {
    ProjectStatus status{ProjectStatus::unopened};
    core::Revision revision;
    std::string diagnostic;
};

/// Describes the result of materializing a primary input.
struct LoadSummary {
    core::Revision revision;
    std::size_t memory_regions{};
    std::size_t decoded_instructions{};
    std::optional<core::FunctionKey> entry_function;
    std::vector<core::Diagnostic> diagnostics;
};

/// Describes the result of an analysis scheduler run.
struct AnalysisSummary {
    core::AnalysisRunId run;
    core::Revision read_revision;
    core::Revision committed_revision;
    std::vector<std::string> analyzers;
    std::vector<core::Diagnostic> diagnostics;
};

} // namespace ghidra::runtime::project
