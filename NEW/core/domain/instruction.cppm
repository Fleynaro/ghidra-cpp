export module ghidra.core.instruction;

import std;
import ghidra.core.address;
import ghidra.core.bytes;
import ghidra.core.flow;
import ghidra.core.identifiers;
import ghidra.core.operand;
import ghidra.core.pcode;

export namespace ghidra::core {

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

} // namespace ghidra::core
