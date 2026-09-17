export module recode.runtime.analysis.coalescer;

import std;
import recode.core;

export namespace recode::runtime::analysis {

namespace core = recode::core;

/// Coalesces duplicate event triggers while retaining all event payloads for replay-safe analyzers.
class TriggerCoalescer final {
public:
    /// Adds committed events to the current dirty batch in sequence order.
    void add(std::span<const core::events::EventEnvelope> events) {
        for (const auto& event : events) {
            if (seen_.insert(event.event_id).second)
                events_.push_back(event);
        }
    }

    /// Returns the current coalesced event batch.
    [[nodiscard]] core::events::EventBatch batch() const {
        return core::events::EventBatch{events_};
    }

    /// Clears dirty events only after a successful scheduler commit.
    void clear() {
        events_.clear();
        seen_.clear();
    }

private:
    std::vector<core::events::EventEnvelope> events_;
    std::set<core::EventId> seen_;
};

} // namespace recode::runtime::analysis
