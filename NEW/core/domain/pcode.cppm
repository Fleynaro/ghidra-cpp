export module ghidra.core.pcode;

import std;
import ghidra.core.address;
import ghidra.core.pcode_opcode;
import ghidra.core.storage_location;

export namespace ghidra::core {

/// Stores one canonical p-code operation independent of native engine ownership.
struct PcodeOp {
    PcodeOpcode opcode{PcodeOpcode::unknown};
    std::optional<StorageLocation> output;
    std::vector<StorageLocation> inputs;
    std::optional<AddressSpaceId> memory_space;
    std::size_t sequence_index{};
    std::optional<std::size_t> source_operand;
};

/// Groups ordered p-code operations belonging to one instruction.
struct PcodeSequence {
    Address instruction;
    std::vector<PcodeOp> operations;
};

} // namespace ghidra::core
