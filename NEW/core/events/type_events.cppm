export module recode.core.events.type;

import std;
import recode.core.data_type;
import recode.core.events.event;
import recode.core.identifiers;

export namespace recode::core::events {

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

} // namespace recode::core::events
