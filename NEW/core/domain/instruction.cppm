export module recode.core.instruction;

import std;
import recode.core.address;
import recode.core.bytes;
import recode.core.decoded_instruction;
import recode.core.flow;
import recode.core.identifiers;
import recode.core.operand;
import recode.core.pcode;

export namespace recode::core {

/// Identifies an instruction entity and its current address.
struct InstructionKey {
    EntityId entity;
    Address address;
    friend bool operator==(const InstructionKey&, const InstructionKey&) = default;
};

/// Stores a complete immutable decoded instruction snapshot.
struct Instruction {
    InstructionKey key;
    std::size_t length{};
    Bytes bytes;
    std::string mnemonic;
    std::string assembly;
    std::vector<InstructionOperand> operands;
    std::vector<std::uint8_t> instruction_mask;
    std::string architecture;
    FlowInfo flow;
    PcodeSequence pcode;
    std::string provenance;
};

/// Promotes a transient decoder snapshot into the space-aware listing instruction model.
///
/// This is the single domain-owned promotion boundary used by service/runtime
/// ingestion. It assigns stable identity, preserves byte/p-code metadata, and
/// resolves direct constant flow targets against the supplied code space while
/// retaining indirect storage targets symbolically.
[[nodiscard]] inline Instruction materialize_decoded_instruction(const DecodedInstruction& source, Address address) {
    Instruction result;
    result.key =
        InstructionKey{EntityId{make_identifier("instruction-" + address.space.name(), address.offset)}, address};
    result.length = source.length;
    result.bytes = Bytes{source.bytes};
    result.mnemonic = source.mnemonic;
    result.assembly = source.assembly;
    result.instruction_mask = source.instruction_mask;
    result.architecture = source.is_x86 ? "x86" : "unknown";
    result.provenance = "sleigh";
    result.pcode.instruction = address;
    for (const auto& operand : source.operands) {
        InstructionOperand converted;
        converted.text = operand.text;
        converted.kind = operand.kind;
        converted.value_mask = operand.value_mask;
        converted.objects = operand.objects;
        if (operand.value) {
            converted.scalar =
                Scalar{*operand.value,
                       operand.value_mask.empty() ? 64U : static_cast<std::uint32_t>(operand.value_mask.size() * 8U),
                       false, operand.kind == OperandKind::address,
                       std::ranges::any_of(operand.objects, [](const auto& object) { return object.relocated; })};
            if (operand.kind == OperandKind::address)
                converted.address = Address{address.space, *operand.value};
        }
        result.operands.push_back(std::move(converted));
    }
    result.flow.kind = source.flow.kind;
    result.flow.has_fallthrough = source.flow.has_fallthrough;
    result.flow.terminal = source.flow.terminal;
    if (source.flow.target) {
        const bool indirect =
            source.flow.kind == FlowKind::indirect_branch || source.flow.kind == FlowKind::indirect_call;
        if (indirect)
            result.flow.indirect_target = *source.flow.target;
        else {
            const auto target_space =
                source.flow.target->space.name() == "const" ? address.space : source.flow.target->space;
            result.flow.target = Address{target_space, source.flow.target->offset};
        }
    }
    for (std::size_t index = 0; index < source.pcode.size(); ++index) {
        PcodeOp operation = source.pcode[index];
        operation.sequence_index = index;
        result.pcode.operations.push_back(std::move(operation));
    }
    return result;
}

} // namespace recode::core
