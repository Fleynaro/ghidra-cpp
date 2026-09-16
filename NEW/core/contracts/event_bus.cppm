export module ghidra.core.contracts.event_bus;

import std;
import ghidra.core.events.event;
import ghidra.core.identifiers;

export namespace ghidra::core::contracts {

/// Identifies an in-process subscription.
using SubscriptionId = std::uint64_t;

/// Publishes committed events to ordered non-projection subscribers.
class IEventBus {
public:
    /// Releases an event bus through its contract.
    virtual ~IEventBus() = default;

    /// Subscribes a callback for one project and returns its handle.
    [[nodiscard]] virtual SubscriptionId subscribe(const ProjectId& project,
                                                   std::function<void(const events::EventEnvelope&)> callback) = 0;

    /// Removes a subscription without affecting event history.
    virtual void unsubscribe(SubscriptionId subscription) = 0;

    /// Publishes already-committed events in sequence order.
    virtual void publish(std::span<const events::EventEnvelope> events) = 0;
};

} // namespace ghidra::core::contracts
