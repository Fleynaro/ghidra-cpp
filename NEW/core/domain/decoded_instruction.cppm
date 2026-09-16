export module ghidra.core.decoded_instruction;

import std;
import ghidra.core.flow;
import ghidra.core.operand;
import ghidra.core.pcode;
import ghidra.core.storage_location;

export namespace ghidra::core {

// Original behavior references: Ghidra/Features/Decompiler/src/decompile/cpp/sleigh.hh
// (`Sleigh::oneInstruction`, `AssemblyEmit`, and `PcodeEmit`) and
// Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/model/lang/SleighInstructionPrototype.java.

/// Stores the low-level operand facts produced by a processor decoder.
///
/// This value is intentionally separate from `InstructionOperand`: the latter
/// is a listing snapshot with resolved scalar/address/register views, while a
/// decoder must also preserve the printed value and exact operand mask before
/// listing resolution has taken place.
struct DecodedOperand {
    /// Provides the historical nested name for decoder hash objects.
    using HashObject = OperandObject;

    std::string text;
    OperandKind kind{OperandKind::unknown};
    std::optional<std::uint64_t> value;
    std::vector<std::uint8_t> value_mask;
    std::vector<OperandObject> objects;
};

/// Describes a low-level control-flow effect before it is resolved into a listing reference.
///
/// Decoder targets are storage locations because indirect branches and calls
/// may still be represented by a register or constant varnode. The Sleigh
/// service resolves concrete address targets into the listing-level `FlowInfo`.
struct DecodedFlowInfo {
    FlowKind kind{FlowKind::none};
    std::optional<StorageLocation> target;
    bool has_fallthrough{true};
    bool terminal{};
};

/// Represents one decoder failure without exposing native runtime exceptions.
struct DecodeError {
    std::string message;
};

/// Captures one instruction as a portable decoder snapshot shared by Sleigh and decompiler adapters.
///
/// The address remains numeric at this boundary because standalone decoder
/// clients and native frontend providers operate on one selected code space.
/// The service boundary promotes it to the space-aware `core::Instruction`.
struct DecodedInstruction {
    std::uint64_t address{};
    std::size_t length{};
    std::vector<std::uint8_t> bytes;
    std::string mnemonic;
    std::string assembly;
    std::vector<DecodedOperand> operands;
    std::vector<std::uint8_t> instruction_mask;
    bool is_x86{};
    DecodedFlowInfo flow;
    std::vector<PcodeOp> pcode;

    /// Constructs an empty decoder snapshot for incremental materialization.
    DecodedInstruction() = default;

    /// Constructs the compact provider form used by scripted decompiler inputs.
    DecodedInstruction(std::uint64_t instruction_address, std::size_t instruction_length,
                       std::string instruction_mnemonic, std::string instruction_assembly,
                       std::vector<PcodeOp> operations)
        : address(instruction_address), length(instruction_length), mnemonic(std::move(instruction_mnemonic)),
          assembly(std::move(instruction_assembly)), pcode(std::move(operations)) {}
};

} // namespace ghidra::core
