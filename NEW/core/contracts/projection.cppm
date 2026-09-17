export module recode.core.contracts.projection;

import std;
import recode.core.events.event;
import recode.core.identifiers;
import recode.core.diagnostics;

export namespace recode::core::contracts {

/// Applies committed state events to a replaceable current query model.
class IProjection {
public:
    /// Releases a projection through its contract.
    virtual ~IProjection() = default;

    /// Applies one event idempotently and advances the checkpoint.
    [[nodiscard]] virtual Result<void> apply(const events::EventEnvelope& event) = 0;

    /// Applies a committed batch in global-sequence order.
    [[nodiscard]] virtual Result<void> apply_batch(std::span<const events::EventEnvelope> events) = 0;

    /// Returns the last event sequence applied to the projection.
    [[nodiscard]] virtual Revision checkpoint() const = 0;

    /// Rebuilds the projection from an empty state and supplied history.
    [[nodiscard]] virtual Result<void> rebuild(std::span<const events::EventEnvelope> events) = 0;
};

/// Provides optional durable materialization for a projection implementation.
class IProjectionStore {
public:
    /// Releases a projection store through its contract.
    virtual ~IProjectionStore() = default;

    /// Opens or initializes the current projection.
    [[nodiscard]] virtual Result<void> open(const ProjectId& project) = 0;

    /// Persists one applied event and its checkpoint.
    [[nodiscard]] virtual Result<void> persist(const events::EventEnvelope& event) = 0;

    /// Returns the durable projection checkpoint.
    [[nodiscard]] virtual Result<Revision> checkpoint(const ProjectId& project) const = 0;

    /// Flushes current projection writes.
    [[nodiscard]] virtual Result<void> flush() = 0;
};

} // namespace recode::core::contracts
