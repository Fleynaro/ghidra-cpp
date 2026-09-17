export module recode.runtime.dispatcher.handlers;

import std;
import recode.core;
import recode.runtime.project.session;

export namespace recode::runtime::dispatcher {

namespace core = recode::core;

/// Provides a named command-handler boundary for project sessions.
class CommandHandlers final {
public:
    /// Executes one command through the supplied session commit lane.
    [[nodiscard]] core::Result<core::contracts::CommandResponse>
    execute(project::ProjectSession& session, const core::contracts::CommandRequest& request) const {
        return session.handle(request);
    }
};

} // namespace recode::runtime::dispatcher
