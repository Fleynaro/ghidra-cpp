export module ghidra.core.events.code;

import std;
import ghidra.core.events.event;
import ghidra.core.identifiers;
import ghidra.core.instruction;

export namespace ghidra::core::events {

/// Creates a compact instruction state event suitable for replay.
[[nodiscard]] inline EventDraft listing_state_changed(const ProjectId& project, const Instruction& instruction,
                                                      const CorrelationId& correlation) {
    return EventDraft{project,
                      "instruction",
                      instruction.key.entity.value(),
                      "ListingStateChanged",
                      1,
                      correlation,
                      std::nullopt,
                      instruction.provenance.empty() ? "sleigh" : instruction.provenance,
                      "instruction-" + instruction.key.entity.value(),
                      encode_fields({{"id", instruction.key.entity.value()},
                                     {"space", instruction.key.address.space.name()},
                                     {"address", std::to_string(instruction.key.address.offset)},
                                     {"length", std::to_string(instruction.length)},
                                     {"mnemonic", instruction.mnemonic},
                                     {"assembly", instruction.assembly}})};
}

} // namespace ghidra::core::events
