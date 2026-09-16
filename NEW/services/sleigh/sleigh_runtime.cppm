// The standard library is imported through the C++23 module interface so this
// public API does not depend on textual standard-header inclusion.
export module sleigh_runtime;
import std;
export import ghidra.core;

export namespace sleigh_runtime {

/// Re-exports the canonical decoder value vocabulary without defining runtime-local DTOs.
using OperandKind = ghidra::core::OperandKind;
using Operand = ghidra::core::DecodedOperand;
using Varnode = ghidra::core::StorageLocation;
using PcodeOpcode = ghidra::core::PcodeOpcode;
using PcodeOp = ghidra::core::PcodeOp;
using FlowKind = ghidra::core::FlowKind;
using FlowInfo = ghidra::core::DecodedFlowInfo;
using ProcessorContext = ghidra::core::ProcessorContext;
/// Preserves the pair-based context fixture spelling without defining a runtime DTO.
using ContextValue = std::pair<std::string, std::uint64_t>;
using Instruction = ghidra::core::DecodedInstruction;
using DecodeError = ghidra::core::DecodeError;

/// Owns a compiled SLA runtime and decodes bounded instruction windows.
class Decoder final {
public:
    /// Loads and validates the specified binary SLA file.
    ///
    /// Throws `std::runtime_error` when the file cannot be opened or has an
    /// unsupported SLA format version.
    explicit Decoder(std::filesystem::path sla_path);

    /// Releases all compiled SLA state and owned runtime resources.
    ~Decoder();

    /// Prevents copying an owning compiled runtime.
    Decoder(const Decoder&) = delete;

    /// Prevents copying an owning compiled runtime.
    Decoder& operator=(const Decoder&) = delete;

    /// Transfers compiled runtime ownership without reloading the SLA file.
    Decoder(Decoder&&) noexcept;

    /// Releases the current runtime and transfers compiled runtime ownership.
    Decoder& operator=(Decoder&&) noexcept;

    /// Decodes bytes at `address`, resolves operands, and materializes all p-code.
    ///
    /// At most the architectural Sleigh instruction buffer (16 bytes) is
    /// accepted. Extra bytes are rejected instead of being silently truncated.
    [[nodiscard]] std::expected<Instruction, DecodeError>
    decode(std::uint64_t address, std::span<const std::uint8_t> bytes, const ProcessorContext& context = {});

private:
    class Implementation;
    std::unique_ptr<Implementation> implementation_;
};

/// Returns the source-tree directory containing the module's compiled SLA specifications.
///
/// The directory is supplied by the module build and is shared by library users and
/// command-line frontends. Callers may still provide an explicit relative or absolute path.
[[nodiscard]] std::filesystem::path default_specification_directory();

/// Resolves a specification name while preserving explicitly qualified paths.
///
/// A path containing only a filename is resolved below `default_specification_directory()`;
/// paths with a parent component are returned unchanged. The function does not require the
/// resulting file to exist and therefore can be used for diagnostics before loading.
[[nodiscard]] std::filesystem::path resolve_sla_path(std::filesystem::path sla_path);

/// Returns the canonical textual name of a materialized p-code opcode.
[[nodiscard]] std::string_view opcode_name(PcodeOpcode opcode);

} // namespace sleigh_runtime
