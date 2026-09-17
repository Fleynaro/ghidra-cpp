export module recode.core.instruction_reference;

import std;
import recode.core.address;
import recode.core.flow;

export namespace recode::core {

/// Retains operand-level evidence from which a projected reference is derived.
struct InstructionReference {
    Address source;
    std::optional<Address> target;
    std::optional<std::uint32_t> operand_index;
    std::string reference_class;
    FlowKind original_flow{FlowKind::none};
    bool fallthrough{};
    std::string source_classification;
};

} // namespace recode::core
