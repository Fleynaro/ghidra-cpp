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

    /// Constructs an empty canonical operation.
    PcodeOp() = default;

    /// Constructs an operation from the canonical strongly typed opcode.
    PcodeOp(PcodeOpcode operation_opcode, std::optional<StorageLocation> operation_output,
            std::vector<StorageLocation> operation_inputs,
            std::optional<AddressSpaceId> operation_memory_space = std::nullopt,
            std::size_t operation_sequence_index = 0,
            std::optional<std::size_t> operation_source_operand = std::nullopt)
        : opcode(operation_opcode), output(std::move(operation_output)), inputs(std::move(operation_inputs)),
          memory_space(std::move(operation_memory_space)), sequence_index(operation_sequence_index),
          source_operand(operation_source_operand) {}

    /// Constructs an operation from the numeric opcode form used by native provider fixtures.
    PcodeOp(std::uint32_t operation_opcode, std::optional<StorageLocation> operation_output,
            std::vector<StorageLocation> operation_inputs,
            std::optional<AddressSpaceId> operation_memory_space = std::nullopt,
            std::size_t operation_sequence_index = 0,
            std::optional<std::size_t> operation_source_operand = std::nullopt)
        : PcodeOp(static_cast<PcodeOpcode>(operation_opcode), std::move(operation_output), std::move(operation_inputs),
                  std::move(operation_memory_space), operation_sequence_index, operation_source_operand) {}
};

/// Groups ordered p-code operations belonging to one instruction.
struct PcodeSequence {
    Address instruction;
    std::vector<PcodeOp> operations;
};

} // namespace ghidra::core
