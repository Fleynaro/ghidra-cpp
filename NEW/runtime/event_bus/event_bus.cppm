export module ghidra.runtime.event_bus;

import std;
import ghidra.core;
import ghidra.core.contracts.event_bus;
import ghidra.core.events.event;

export namespace ghidra::runtime::event_bus {

namespace core = ghidra::core;

/// Publishes committed envelopes to ordered in-process subscribers.
class EventBus final : public core::contracts::IEventBus {
public:
    /// Adds a project-scoped callback and returns its subscription identity.
    [[nodiscard]] core::contracts::SubscriptionId
    subscribe(const core::ProjectId& project,
              std::function<void(const core::events::EventEnvelope&)> callback) override {
        std::scoped_lock lock(mutex_);
        const auto id = next_id_++;
        subscribers_.emplace(id, Subscriber{project, std::move(callback)});
        return id;
    }

    /// Removes a callback without changing committed history.
    void unsubscribe(core::contracts::SubscriptionId subscription) override {
        std::scoped_lock lock(mutex_);
        subscribers_.erase(subscription);
    }

    /// Delivers events in the supplied per-project sequence order.
    void publish(std::span<const core::events::EventEnvelope> events) override {
        for (const auto& event : events) {
            std::vector<std::function<void(const core::events::EventEnvelope&)>> callbacks;
            {
                std::scoped_lock lock(mutex_);
                for (const auto& [unused, subscriber] : subscribers_)
                    if (subscriber.project == event.project)
                        callbacks.push_back(subscriber.callback);
            }
            for (auto& callback : callbacks)
                callback(event);
        }
    }

private:
    /// Groups a callback with the project whose event stream it observes.
    struct Subscriber {
        core::ProjectId project;
        std::function<void(const core::events::EventEnvelope&)> callback;
    };

    std::mutex mutex_;
    std::map<core::contracts::SubscriptionId, Subscriber> subscribers_;
    core::contracts::SubscriptionId next_id_{1};
};

} // namespace ghidra::runtime::event_bus
