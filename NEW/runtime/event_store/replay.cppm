export module ghidra.runtime.event_store.replay;

import std;
import ghidra.core;
import ghidra.core.contracts.event_store;
import ghidra.core.contracts.projection;

export namespace ghidra::runtime::event_store {

namespace core = ghidra::core;

/// Rebuilds a projection directly from committed event history.
[[nodiscard]] inline core::Result<void>
replay(core::contracts::IEventStore& store, core::contracts::IProjection& projection, const core::ProjectId& project) {
    auto stream = store.read(project, core::Revision{1});
    if (!stream)
        return std::unexpected(stream.error());
    return projection.rebuild(stream->events);
}

} // namespace ghidra::runtime::event_store
