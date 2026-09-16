export module ghidra.runtime.api.project;

import std;
import ghidra.core;
import ghidra.runtime.project.config;
import ghidra.runtime.project.runtime_core;
import ghidra.runtime.project.session;
import ghidra.runtime.project.state;

export namespace ghidra::runtime::api {

/// Exposes a narrow native C++ project API without leaking runtime/service implementation types.
class ProjectFacade final {
public:
    /// Opens a project through a caller-owned runtime core.
    static core::Result<std::shared_ptr<ProjectFacade>> open(std::shared_ptr<project::RuntimeCore> runtime,
                                                             project::ProjectConfig config) {
        auto session = runtime->open(std::move(config));
        if (!session)
            return std::unexpected(session.error());
        return std::shared_ptr<ProjectFacade>(new ProjectFacade(std::move(runtime), std::move(*session)));
    }

    /// Loads the configured primary executable through the event/projection commit path.
    [[nodiscard]] core::Result<project::LoadSummary> load() {
        return session_->load_primary_binary();
    }

    /// Runs the project analysis lifecycle and returns committed revision evidence.
    [[nodiscard]] core::Result<project::AnalysisSummary> analyze() {
        return session_->analyze();
    }

    /// Returns a current lifecycle snapshot.
    [[nodiscard]] project::ProjectState state() const {
        return session_->state();
    }

    /// Returns the read-only current query view.
    [[nodiscard]] std::shared_ptr<const core::contracts::IProjectQuery> query() const {
        return session_->query();
    }

    /// Queues native decompilation for one current function snapshot.
    [[nodiscard]] core::contracts::Task<core::Result<core::Decompilation>> decompile(core::FunctionKey function) {
        return session_->decompile(std::move(function));
    }

    /// Executes one typed command through the project commit lane.
    [[nodiscard]] core::Result<core::contracts::CommandResponse>
    execute(const core::contracts::CommandRequest& request) {
        return session_->handle(request);
    }

    /// Closes this project and releases its manager slot.
    void close() noexcept {
        if (session_) {
            const auto project = session_->id();
            runtime_->projects().close(project);
            session_.reset();
        }
    }

    /// Returns the stable project identity.
    [[nodiscard]] const core::ProjectId& id() const noexcept {
        return session_->id();
    }

private:
    /// Stores only native facade ownership and the opaque project session.
    ProjectFacade(std::shared_ptr<project::RuntimeCore> runtime, std::shared_ptr<project::ProjectSession> session)
        : runtime_(std::move(runtime)), session_(std::move(session)) {}

    std::shared_ptr<project::RuntimeCore> runtime_;
    std::shared_ptr<project::ProjectSession> session_;
};

} // namespace ghidra::runtime::api
