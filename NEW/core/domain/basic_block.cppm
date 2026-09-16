export module ghidra.core.basic_block;

import std;
import ghidra.core.address_range;
import ghidra.core.identifiers;

export namespace ghidra::core {

/// Classifies the role of a basic block in a control-flow graph.
enum class BasicBlockKind : std::uint8_t { normal, entry, exit, thunk, external, unknown };

/// Stores one immutable basic/simple block view.
struct BasicBlock {
    EntityId id;
    AddressRangeSet ranges;
    std::vector<std::uint64_t> instruction_starts;
    std::vector<EntityId> successors;
    std::vector<EntityId> predecessors;
    BasicBlockKind kind{BasicBlockKind::normal};
};

} // namespace ghidra::core
