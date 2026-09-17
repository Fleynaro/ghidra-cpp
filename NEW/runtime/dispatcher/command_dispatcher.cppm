export module recode.runtime.dispatcher.command;

import std;
import recode.core;
import recode.runtime.project.session;
import recode.runtime.workers.pool;

export namespace recode::runtime::dispatcher {

/// Separates inline validation from runtime-owned queued command execution.
class CommandDispatcher final {
public:
    /// Uses the shared worker pool for queued command handling.
    explicit CommandDispatcher(std::shared_ptr<workers::WorkerPool> workers) : workers_(std::move(workers)) {}

    /// Executes inline or returns a task according to command routing metadata.
    [[nodiscard]] core::Result<core::contracts::CommandResponse>
    dispatch(project::ProjectSession& session, const core::contracts::CommandRequest& request) const {
        if (request.mode == core::contracts::ExecutionMode::queued)
            return std::unexpected(
                core::Error::make(core::DiagnosticCode::invalid_argument, "Queued commands require dispatch_async"));
        return session.handle(request);
    }

    /// Queues one command on the shared runtime pool and preserves its response type.
    [[nodiscard]] core::contracts::Task<core::Result<core::contracts::CommandResponse>>
    dispatch_async(std::shared_ptr<project::ProjectSession> session, core::contracts::CommandRequest request) const {
        auto submitted = workers_->submit(
            request.project, request.priority,
            [session = std::move(session), request = std::move(request)](core::contracts::CancellationToken token) {
                if (token.stop_requested())
                    return core::Result<core::contracts::CommandResponse>{std::unexpected(
                        core::Error::make(core::DiagnosticCode::cancelled, "Command was cancelled before dispatch"))};
                return session->handle(request);
            });
        if (!submitted)
            throw std::runtime_error(submitted.error().message);
        return std::move(*submitted);
    }

private:
    std::shared_ptr<workers::WorkerPool> workers_;
};

} // namespace recode::runtime::dispatcher
