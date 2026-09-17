export module recode.core.events.analysis;

import std;
import recode.core.events.event;
import recode.core.identifiers;

export namespace recode::core::events {

/// Creates an analysis-run lifecycle event; progress ticks remain transient.
[[nodiscard]] inline EventDraft analysis_run_state_changed(const ProjectId& project, const AnalysisRunId& run,
                                                           std::string status, const CorrelationId& correlation) {
    return EventDraft{project,
                      "analysis_run",
                      run.value(),
                      "AnalysisRunStateChanged",
                      1,
                      correlation,
                      std::nullopt,
                      "runtime.analysis",
                      "analysis-run-" + run.value() + "-" + status,
                      encode_fields({{"run", run.value()}, {"status", std::move(status)}})};
}

} // namespace recode::core::events
