export module recode.core.events.memory;

import std;
import recode.core.events.event;
import recode.core.identifiers;
import recode.core.memory_region;

export namespace recode::core::events {

/// Creates a loader-owned mapped-memory state event.
[[nodiscard]] inline EventDraft memory_state_changed(const ProjectId& project, const MemoryRegion& region,
                                                     const CorrelationId& correlation) {
    return EventDraft{project,
                      "memory",
                      region.id.value(),
                      "MemoryStateChanged",
                      1,
                      correlation,
                      std::nullopt,
                      "pe_loader",
                      "memory-" + region.id.value(),
                      encode_fields({{"id", region.id.value()},
                                     {"space", region.range.start.space.name()},
                                     {"start", std::to_string(region.range.start.offset)},
                                     {"end", std::to_string(region.range.end.offset)},
                                     {"name", region.name},
                                     {"r", region.permissions.readable ? "1" : "0"},
                                     {"w", region.permissions.writable ? "1" : "0"},
                                     {"x", region.permissions.executable ? "1" : "0"}})};
}

} // namespace recode::core::events
