export module ghidra.core.events.type;

import std;
import ghidra.core.data_type;
import ghidra.core.events.event;
import ghidra.core.identifiers;

export namespace ghidra::core::events {

/// Creates a serializable data-type descriptor event.
[[nodiscard]] inline EventDraft type_state_changed(const ProjectId& project, const DataTypeDescriptor& type,
                                                   const CorrelationId& correlation) {
    return EventDraft{project,
                      "type",
                      type.id,
                      "TypeStateChanged",
                      1,
                      correlation,
                      std::nullopt,
                      "type",
                      "type-" + type.id,
                      encode_fields({{"id", type.id}, {"name", type.name}, {"size", std::to_string(type.size)}})};
}

} // namespace ghidra::core::events
