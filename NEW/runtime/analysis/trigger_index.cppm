export module recode.runtime.analysis.trigger_index;

import std;
import recode.core;

export namespace recode::runtime::analysis {

namespace core = recode::core;

/// Indexes analyzer IDs by event type for bounded trigger selection.
class TriggerIndex final {
public:
    /// Adds one descriptor's trigger set to the index.
    void add(const core::contracts::AnalyzerDescriptor& descriptor) {
        for (const auto& event_type : descriptor.triggers)
            analyzers_[event_type].insert(descriptor.stable_id);
    }

    /// Returns analyzer IDs triggered by any event in the batch.
    [[nodiscard]] std::set<std::string> triggered(std::span<const core::events::EventEnvelope> events) const {
        std::set<std::string> result;
        for (const auto& event : events) {
            const auto iterator = analyzers_.find(event.event_type);
            if (iterator != analyzers_.end())
                result.insert(iterator->second.begin(), iterator->second.end());
        }
        return result;
    }

private:
    std::map<std::string, std::set<std::string>> analyzers_;
};

} // namespace recode::runtime::analysis
