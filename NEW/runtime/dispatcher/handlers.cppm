export module ghidra.runtime.dispatcher.handlers;

import std;
import ghidra.core;
import ghidra.runtime.project.session;

export namespace ghidra::runtime::dispatcher {

namespace core = ghidra::core;

/// Provides a named command-handler boundary for project sessions.
class CommandHandlers final {
public:
    /// Executes one command through the supplied session commit lane.
    [[nodiscard]] core::Result<core::contracts::CommandResponse>
    execute(project::ProjectSession& session, const core::contracts::CommandRequest& request) const {
        return session.handle(request);
    }
};

} // namespace ghidra::runtime::dispatcher
