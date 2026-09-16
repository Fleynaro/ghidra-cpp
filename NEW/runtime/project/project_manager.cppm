export module ghidra.runtime.project.manager;

import std;
import ghidra.core;
import ghidra.runtime.event_bus;
import ghidra.runtime.project.config;
import ghidra.runtime.project.session;
import ghidra.runtime.workers.pool;

export namespace ghidra::runtime::project {

/// Owns project sessions while sharing one runtime worker pool and event bus.
class ProjectManager final {
public:
    /// Constructs a manager over runtime-owned infrastructure.
    ProjectManager(std::shared_ptr<workers::WorkerPool> workers, std::shared_ptr<event_bus::EventBus> bus)
        : workers_(std::move(workers)), bus_(std::move(bus)) {}

    /// Opens or returns the existing project identified by `config.id`.
    [[nodiscard]] core::Result<std::shared_ptr<ProjectSession>> open(ProjectConfig config) {
        std::scoped_lock lock(mutex_);
        if (const auto existing = projects_.find(config.id); existing != projects_.end())
            return existing->second;
        auto session = ProjectSession::open(std::move(config), workers_, bus_);
        if (!session)
            return std::unexpected(session.error());
        projects_.emplace((*session)->id(), *session);
        return *session;
    }

    /// Returns one open project session without changing ownership.
    [[nodiscard]] std::shared_ptr<ProjectSession> find(const core::ProjectId& id) const {
        std::scoped_lock lock(mutex_);
        const auto iterator = projects_.find(id);
        return iterator == projects_.end() ? nullptr : iterator->second;
    }

    /// Closes and removes one project session.
    void close(const core::ProjectId& id) noexcept {
        std::shared_ptr<ProjectSession> session;
        {
            std::scoped_lock lock(mutex_);
            const auto iterator = projects_.find(id);
            if (iterator == projects_.end())
                return;
            session = std::move(iterator->second);
            projects_.erase(iterator);
        }
        session->close();
    }

    /// Closes all sessions before runtime worker shutdown.
    void close_all() noexcept {
        std::vector<std::shared_ptr<ProjectSession>> sessions;
        {
            std::scoped_lock lock(mutex_);
            for (auto& [unused, session] : projects_)
                sessions.push_back(std::move(session));
            projects_.clear();
        }
        for (auto& session : sessions)
            session->close();
    }

private:
    std::shared_ptr<workers::WorkerPool> workers_;
    std::shared_ptr<event_bus::EventBus> bus_;
    mutable std::mutex mutex_;
    std::map<core::ProjectId, std::shared_ptr<ProjectSession>> projects_;
};

} // namespace ghidra::runtime::project
