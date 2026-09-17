export module recode.core.variable;

import std;
import recode.core.data_type;
import recode.core.identifiers;
import recode.core.storage_location;

export namespace recode::core {

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

} // namespace recode::core
