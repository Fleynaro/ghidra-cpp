export module ghidra.core.variable;

import std;
import ghidra.core.data_type;
import ghidra.core.identifiers;
import ghidra.core.storage_location;

export namespace ghidra::core {

/// Groups one or more physical storage pieces for an ABI value.
struct VariableStorage {
    std::vector<StorageLocation> pieces;
};

/// Describes one source/local/parameter variable without mutable manager state.
struct VariableDescription {
    EntityId id;
    std::string name;
    DataTypeId type;
    VariableStorage storage;
    std::string source;
    bool isolated{};
};

} // namespace ghidra::core
