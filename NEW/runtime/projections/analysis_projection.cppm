export module recode.runtime.projections.analysis;

import std;
import recode.core;
import recode.core.events.event;

export namespace recode::runtime::projections {

namespace core = recode::core;

/// Tracks analysis-run lifecycle records separately from software-model rows.
class AnalysisProjection final {
public:
    /// Records one committed analysis lifecycle event.
    void apply(const core::events::EventEnvelope& event) {
        if (event.event_type == "AnalysisRunStateChanged")
            runs_[event.aggregate_id] = core::events::decode_fields(event.payload)["status"];
    }

    /// Returns the last known status of an analysis run.
    [[nodiscard]] std::optional<std::string> status(std::string_view run) const {
        const auto iterator = runs_.find(std::string(run));
        return iterator == runs_.end() ? std::nullopt : std::optional{iterator->second};
    }

private:
    std::map<std::string, std::string> runs_;
};

} // namespace recode::runtime::projections
