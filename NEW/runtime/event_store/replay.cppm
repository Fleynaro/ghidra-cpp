export module recode.runtime.event_store.replay;

import std;
import recode.core;
import recode.core.contracts.event_store;
import recode.core.contracts.projection;

export namespace recode::runtime::event_store {

namespace core = recode::core;

/// Rebuilds a projection directly from committed event history.
[[nodiscard]] inline core::Result<void>
replay(core::contracts::IEventStore& store, core::contracts::IProjection& projection, const core::ProjectId& project) {
    auto stream = store.read(project, core::Revision{1});
    if (!stream)
        return std::unexpected(stream.error());
    return projection.rebuild(stream->events);
}

} // namespace recode::runtime::event_store
