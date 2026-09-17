export module recode.runtime.projections.coordinator;

import std;
import recode.core;
import recode.core.contracts.event_bus;
import recode.core.contracts.event_store;
import recode.core.contracts.projection;

export namespace recode::runtime::projections {

namespace core = recode::core;

/// Coordinates append, projection apply, checkpoint persistence, and publication.
class ProjectionCoordinator final {
public:
    /// Takes ownership of the project-scoped infrastructure interfaces.
    ProjectionCoordinator(std::shared_ptr<core::contracts::IEventStore> store,
                          std::shared_ptr<core::contracts::IProjection> projection,
                          std::shared_ptr<core::contracts::IProjectionStore> projection_store,
                          std::shared_ptr<core::contracts::IEventBus> bus)
        : store_(std::move(store)), projection_(std::move(projection)), projection_store_(std::move(projection_store)),
          bus_(std::move(bus)) {}

    /// Commits drafts atomically through the one project writer lane.
    [[nodiscard]] core::Result<core::contracts::AppendResult>
    commit(const core::ProjectId& project, std::span<const core::events::EventDraft> drafts,
           std::optional<core::Revision> expected = std::nullopt) {
        std::scoped_lock lock(mutex_);
        auto current = store_->last_revision(project);
        if (!current)
            return std::unexpected(current.error());
        if (expected && expected->value != current->value)
            return std::unexpected(core::Error::make(core::DiagnosticCode::conflict,
                                                     "Project revision changed before commit",
                                                     "Recapture a current immutable project snapshot."));
        auto append = store_->append(project, drafts);
        if (!append)
            return std::unexpected(append.error());
        if (const auto applied = projection_->apply_batch(append->batch.events); !applied)
            return std::unexpected(applied.error());
        for (const auto& event : append->batch.events) {
            if (const auto persisted = projection_store_->persist(event); !persisted)
                return std::unexpected(persisted.error());
        }
        if (const auto flushed = projection_store_->flush(); !flushed)
            return std::unexpected(flushed.error());
        bus_->publish(append->batch.events);
        return append;
    }

    /// Rebuilds the in-memory and durable projection from event history.
    [[nodiscard]] core::Result<void> rebuild(const core::ProjectId& project) {
        std::scoped_lock lock(mutex_);
        auto stream = store_->read(project, core::Revision{1});
        if (!stream)
            return std::unexpected(stream.error());
        if (const auto result = projection_->rebuild(stream->events); !result)
            return std::unexpected(result.error());
        for (const auto& event : stream->events)
            if (const auto persisted = projection_store_->persist(event); !persisted)
                return std::unexpected(persisted.error());
        return projection_store_->flush();
    }

private:
    std::mutex mutex_;
    std::shared_ptr<core::contracts::IEventStore> store_;
    std::shared_ptr<core::contracts::IProjection> projection_;
    std::shared_ptr<core::contracts::IProjectionStore> projection_store_;
    std::shared_ptr<core::contracts::IEventBus> bus_;
};

} // namespace recode::runtime::projections
