export module recode.core.events.code;

import std;
import recode.core.events.event;
import recode.core.identifiers;
import recode.core.instruction;
import recode.core.operand;

export namespace recode::core::events {

/// Encodes binary instruction data into the delimiter-safe hexadecimal event representation.
[[nodiscard]] inline std::string encode_hex(const std::vector<std::uint8_t>& bytes) {
    std::string result;
    result.reserve(bytes.size() * 2U);
    constexpr std::array<char, 16> digits{'0', '1', '2', '3', '4', '5', '6', '7',
                                          '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
    for (const auto byte : bytes) {
        result.push_back(digits[byte >> 4U]);
        result.push_back(digits[byte & 0x0fU]);
    }
    return result;
}

/// Encodes operand object facts required for deterministic Function ID hashing.
[[nodiscard]] inline std::string encode_operands(const std::vector<InstructionOperand>& operands) {
    std::string result;
    for (std::size_t operand_index = 0; operand_index < operands.size(); ++operand_index) {
        if (operand_index != 0)
            result.push_back(';');
        const auto& operand = operands[operand_index];
        result += std::to_string(std::to_underlying(operand.kind));
        result.push_back(':');
        result += operand.scalar ? std::to_string(operand.scalar->value) : "_";
        result.push_back(':');
        for (std::size_t object_index = 0; object_index < operand.objects.size(); ++object_index) {
            if (object_index != 0)
                result.push_back(',');
            const auto& object = operand.objects[object_index];
            result += std::to_string(std::to_underlying(object.kind));
            result.push_back(':');
            result += std::to_string(object.value);
            result += object.whole_scalar ? ":1" : ":0";
            result += object.address_scalar ? ":1" : ":0";
            result += object.relocated ? ":1" : ":0";
        }
    }
    return result;
}

/// Creates a compact instruction state event suitable for replay.
[[nodiscard]] inline EventDraft listing_state_changed(const ProjectId& project, const Instruction& instruction,
                                                      const CorrelationId& correlation) {
    return EventDraft{
        project,
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
                       {"assembly", instruction.assembly},
                       {"bytes", encode_hex(instruction.bytes.values())},
                       {"instruction_mask", encode_hex(instruction.instruction_mask)},
                       {"operands", encode_operands(instruction.operands)},
                       {"flow_kind", std::to_string(std::to_underlying(instruction.flow.kind))},
                       {"flow_fallthrough", instruction.flow.has_fallthrough ? "1" : "0"},
                       {"flow_terminal", instruction.flow.terminal ? "1" : "0"},
                       {"flow_target", instruction.flow.target && instruction.flow.kind != FlowKind::return_op
                                           ? std::to_string(instruction.flow.target->offset)
                                           : ""},
                       {"pcode_count", std::to_string(instruction.pcode.operations.size())}})};
}

} // namespace recode::core::events
