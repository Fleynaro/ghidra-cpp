export module recode.core.flow;

import std;
import recode.core.address;
import recode.core.storage_location;

export namespace recode::core {

/// Classifies decoded control-flow behavior.
enum class FlowKind : std::uint8_t {
    none,
    branch,
    conditional_branch,
    conditional_call,
    call,
    indirect_branch,
    indirect_call,
    return_op,
};

/// Describes the decoded flow effect of one instruction.
struct FlowInfo {
    FlowKind kind{FlowKind::none};
    std::optional<Address> target;
    std::optional<StorageLocation> indirect_target;
    bool has_fallthrough{true};
    bool terminal{};
};

/// Represents a user or analysis correction separate from decoded flow.
struct FlowOverride {
    Address address;
    std::string kind;
    std::optional<Address> target;
};

} // namespace recode::core
