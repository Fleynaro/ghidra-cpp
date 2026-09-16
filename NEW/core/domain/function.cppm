export module ghidra.core.function;

import std;
import ghidra.core.address;
import ghidra.core.address_range;
import ghidra.core.basic_block;
import ghidra.core.function_signature;
import ghidra.core.identifiers;
import ghidra.core.variable;

export namespace ghidra::core {

/// Identifies a function by project entity and entry address.
struct FunctionKey {
    EntityId entity;
    Address entry;
    friend bool operator==(const FunctionKey&, const FunctionKey&) = default;
};

/// Stores the revision-stamped immutable state of one function entity.
struct FunctionSnapshot {
    FunctionKey key;
    std::string name;
    std::string namespace_name;
    AddressRangeSet body;
    std::vector<std::uint64_t> instruction_starts;
    std::vector<BasicBlock> blocks;
    std::optional<FunctionKey> thunk_target;
    bool external{};
    bool no_return{};
    std::optional<VariableStorage> stack_frame;
    std::optional<FunctionSignature> signature;
    std::vector<VariableDescription> variables;
    std::string source;
    std::string analysis_status;
};

} // namespace ghidra::core
