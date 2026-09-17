export module recode.core.function;

import std;
import recode.core.address;
import recode.core.address_range;
import recode.core.basic_block;
import recode.core.function_signature;
import recode.core.identifiers;
import recode.core.variable;

export namespace recode::core {

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

} // namespace recode::core
