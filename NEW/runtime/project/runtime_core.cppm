export module recode.runtime.project.runtime_core;

import std;
import recode.core;
import recode.runtime.event_bus;
import recode.runtime.project.config;
import recode.runtime.project.manager;
import recode.runtime.project.session;
import recode.runtime.workers.pool;

export namespace recode::runtime::project {

/// Configures the process-wide native runtime infrastructure.
struct RuntimeConfig {
    workers::WorkerPoolConfig worker_pool;
};

/// Owns the shared worker pool, event bus, and project manager lifecycle.
class RuntimeCore final {
public:
    /// Starts the runtime infrastructure.
    explicit RuntimeCore(RuntimeConfig config = {})
        : workers_(std::make_shared<workers::WorkerPool>(config.worker_pool)),
          bus_(std::make_shared<event_bus::EventBus>()), manager_(workers_, bus_) {}

    /// Closes projects and then shuts down all worker threads.
    ~RuntimeCore() {
        shutdown();
    }

    /// Prevents copying thread-owning runtime state.
    RuntimeCore(const RuntimeCore&) = delete;

    /// Prevents copying thread-owning runtime state.
    RuntimeCore& operator=(const RuntimeCore&) = delete;

    /// Opens one project through the shared manager.
    [[nodiscard]] core::Result<std::shared_ptr<ProjectSession>> open(ProjectConfig config) {
        return manager_.open(std::move(config));
    }

    /// Returns the process-shared project manager.
    [[nodiscard]] ProjectManager& projects() noexcept {
        return manager_;
    }

    /// Returns the process-shared worker pool for advanced native services.
    [[nodiscard]] std::shared_ptr<workers::WorkerPool> workers() const noexcept {
        return workers_;
    }

    /// Closes projects and shuts down the worker pool once.
    void shutdown() noexcept {
        if (shutdown_)
            return;
        shutdown_ = true;
        manager_.close_all();
        workers_->shutdown();
    }

private:
    std::shared_ptr<workers::WorkerPool> workers_;
    std::shared_ptr<event_bus::EventBus> bus_;
    ProjectManager manager_;
    bool shutdown_{};
};

} // namespace recode::runtime::project
