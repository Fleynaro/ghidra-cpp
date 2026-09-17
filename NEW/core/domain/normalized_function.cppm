export module recode.core.normalized_function;

import std;
import recode.core.function;
import recode.core.pcode_opcode;
import recode.core.storage_location;

export namespace recode::core {

/// Describes the stable SSA identity and semantic flags of one normalized value.
/// The identity is independent of native Varnode ownership and remains valid after
/// the decompiler releases its internal graph.
struct NormalizedVarnode {
    std::uint32_t id{};
    StorageLocation storage;
    std::uint32_t size{};
    bool constant{};
    std::uint64_t constant_value{};
    bool input{};
    bool persistent{};
    bool annotation{};
    bool written{};
    std::optional<std::uint32_t> defining_operation;
};

/// Describes one normalized SSA operation and its data-flow edges.
/// Input order is significant unless `commutative` is true, and the optional
/// marker flag controls the native BSim indirect-noise graph.
struct NormalizedOperation {
    std::uint32_t id{};
    PcodeOpcode opcode{PcodeOpcode::unknown};
    std::optional<std::uint32_t> output;
    std::vector<std::uint32_t> inputs;
    std::uint32_t block_id{};
    bool marker{};
    bool commutative{};
};

/// Describes one normalized control-flow block with stable predecessor order.
struct NormalizedBasicBlock {
    std::uint32_t id{};
    std::vector<std::uint32_t> operation_ids;
    std::vector<std::uint32_t> predecessors;
    std::vector<std::uint32_t> successors;
};

/// Owns the value-only function graph consumed by similarity algorithms.
/// This is the boundary object produced by an analysis/decompiler adapter; it
/// contains no database handles, native decompiler pointers, or Ghidra classes.
struct NormalizedFunction {
    std::optional<FunctionKey> identity;
    std::string name;
    std::vector<NormalizedVarnode> varnodes;
    std::vector<NormalizedOperation> operations;
    std::vector<NormalizedBasicBlock> blocks;
    std::vector<std::uint64_t> direct_call_addresses;
    bool has_unimplemented{};
    bool has_bad_data{};
};

} // namespace recode::core
