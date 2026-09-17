export module recode.core.reference;

import std;
import recode.core.address;
import recode.core.flow;
import recode.core.identifiers;

export namespace recode::core {

/// Classifies the semantic relationship represented by a reference.
enum class ReferenceKind : std::uint8_t {
    fallthrough,
    conditional_jump,
    unconditional_jump,
    conditional_call,
    unconditional_call,
    computed_jump,
    computed_call,
    data,
    scalar,
    stack,
    external,
};

/// Stores one stable source-target relation and its provenance.
struct Reference {
    EntityId id;
    Address source;
    std::optional<Address> target;
    std::optional<EntityId> target_entity;
    std::optional<std::uint32_t> operand_index;
    ReferenceKind kind{ReferenceKind::data};
    bool primary{};
    bool source_reference{};
    std::optional<FlowOverride> flow_override;
    std::optional<std::int64_t> stack_offset;
    bool external{};
    bool entry_point{};
    std::string provenance;
};

} // namespace recode::core
