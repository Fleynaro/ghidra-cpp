export module recode.core.contracts.event_store;

import std;
import recode.core.events.event;
import recode.core.identifiers;
import recode.core.diagnostics;

export namespace recode::core::contracts {

/// Reports identities and committed envelopes from one append operation.
struct AppendResult {
    Revision previous_revision;
    Revision committed_revision;
    events::EventBatch batch;
};

/// Owns an ordered read stream of committed project events.
struct EventStream {
    std::vector<events::EventEnvelope> events;
};

/// Persists append-only project history and performs torn-tail recovery.
class IEventStore {
public:
    /// Releases an event store through its contract.
    virtual ~IEventStore() = default;

    /// Appends uncommitted drafts and assigns event identities/sequences.
    [[nodiscard]] virtual Result<AppendResult> append(const ProjectId& project,
                                                      std::span<const events::EventDraft> drafts) = 0;

    /// Reads events from a revision, inclusive.
    [[nodiscard]] virtual Result<EventStream> read(const ProjectId& project, Revision from) const = 0;

    /// Returns the last committed project revision.
    [[nodiscard]] virtual Result<Revision> last_revision(const ProjectId& project) const = 0;

    /// Flushes durable bytes according to the store policy.
    [[nodiscard]] virtual Result<void> flush() = 0;

    /// Closes files and releases store resources.
    virtual void close() noexcept = 0;
};

} // namespace recode::core::contracts
