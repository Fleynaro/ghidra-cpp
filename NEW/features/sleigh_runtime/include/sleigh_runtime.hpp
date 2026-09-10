#ifndef NEW_SLEIGH_RUNTIME_HPP
#define NEW_SLEIGH_RUNTIME_HPP

#if !defined(SLEIGH_RUNTIME_MODULE_INTERFACE) && !defined(SLEIGH_RUNTIME_MODULE_IMPLEMENTATION)
#include <cstddef>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#endif

#ifdef SLEIGH_RUNTIME_MODULE_INTERFACE
export namespace sleigh_runtime {
#else
namespace sleigh_runtime {
#endif

/// Identifies the semantic class of a decoded assembly operand.
enum class OperandKind : std::uint8_t {
    unknown,
    register_value,
    immediate,
    memory,
    address,
};

/// Describes one operand as printed by the compiled Sleigh constructor.
struct Operand {
    std::string text;
    OperandKind kind = OperandKind::unknown;
    std::optional<std::uint64_t> value;
};

/// Identifies a concrete p-code storage location.
struct Varnode {
    std::string space;
    std::uint64_t offset = 0;
    std::uint32_t size = 0;

    /// Compares the complete storage identity of two varnodes.
    friend bool operator==(const Varnode&, const Varnode&) = default;
};

/// The p-code operation identifier used by the Sleigh specification.
enum class PcodeOpcode : std::uint8_t {
    copy = 1,
    load = 2,
    store = 3,
    branch = 4,
    cbranch = 5,
    branch_ind = 6,
    call = 7,
    call_ind = 8,
    call_other = 9,
    return_op = 10,
    int_equal = 11,
    int_not_equal = 12,
    int_sless = 13,
    int_sless_equal = 14,
    int_less = 15,
    int_less_equal = 16,
    int_zext = 17,
    int_sext = 18,
    int_add = 19,
    int_sub = 20,
    int_carry = 21,
    int_scarry = 22,
    int_sborrow = 23,
    int_two_comp = 24,
    int_negate = 25,
    int_xor = 26,
    int_and = 27,
    int_or = 28,
    int_left = 29,
    int_right = 30,
    int_sright = 31,
    int_mult = 32,
    int_div = 33,
    int_sdiv = 34,
    int_rem = 35,
    int_srem = 36,
    bool_negate = 37,
    bool_xor = 38,
    bool_and = 39,
    bool_or = 40,
    float_equal = 41,
    float_not_equal = 42,
    float_less = 43,
    float_less_equal = 44,
    float_nan = 46,
    float_add = 47,
    float_div = 48,
    float_mult = 49,
    float_sub = 50,
    float_neg = 51,
    float_abs = 52,
    float_sqrt = 53,
    float_int_to_float = 54,
    float_float_to_float = 55,
    float_trunc = 56,
    float_ceil = 57,
    float_floor = 58,
    float_round = 59,
    multiequal = 60,
    indirect = 61,
    piece = 62,
    subpiece = 63,
    cast = 64,
    ptradd = 65,
    ptrsub = 66,
    segment_op = 67,
    cpool_ref = 68,
    new_op = 69,
    insert = 70,
    popcount = 72,
    lzcount = 73,
    spull = 74,
};

/// Describes control-flow semantics emitted for the instruction.
enum class FlowKind : std::uint8_t {
    none,
    branch,
    conditional_branch,
    call,
    indirect_branch,
    indirect_call,
    return_op,
};

/// Represents one materialized p-code operation.
struct PcodeOp {
    PcodeOpcode opcode = PcodeOpcode::copy;
    std::optional<Varnode> output;
    std::vector<Varnode> inputs;
};

/// Represents one control-flow effect discovered in the materialized p-code.
struct FlowInfo {
    FlowKind kind = FlowKind::none;
    std::optional<Varnode> target;
};

/// Supplies low-level processor context values by their Sleigh field names.
struct ProcessorContext {
    std::vector<std::pair<std::string, std::uint64_t>> values;
};

/// The result of decoding one machine instruction.
struct Instruction {
    std::uint64_t address = 0;
    std::size_t length = 0;
    std::string mnemonic;
    std::string assembly;
    std::vector<Operand> operands;
    FlowInfo flow;
    std::vector<PcodeOp> pcode;
};

/// Describes a decode failure without exposing the legacy Ghidra exception types.
struct DecodeError {
    std::string message;
};

/// Runtime decoder for a binary, compiled Sleigh specification.
///
/// The decoder owns the compiled runtime state and accepts only the binary `.sla`
/// format. It has no dependency on ProgramDB, Listing, the decompiler, or a GUI.
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
    /// At most the architectural Sleigh instruction buffer (16 bytes) is needed;
    /// extra bytes are rejected so an accidental oversized instruction cannot be
    /// silently truncated.
    [[nodiscard]] std::expected<Instruction, DecodeError>
    decode(std::uint64_t address, std::span<const std::uint8_t> bytes, const ProcessorContext& context = {});

private:
    class Implementation;
    std::unique_ptr<Implementation> implementation_;
};

/// Returns the canonical textual name of a materialized p-code opcode.
[[nodiscard]] std::string_view opcode_name(PcodeOpcode opcode);

} // namespace sleigh_runtime

#endif
