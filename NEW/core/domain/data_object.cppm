export module ghidra.core.data_object;

import std;
import ghidra.core.address_range;
import ghidra.core.data_type;
import ghidra.core.identifiers;

export namespace ghidra::core {

/// Stores a typed data listing object independently from raw image bytes.
struct DataObject {
    EntityId id;
    AddressRange range;
    DataTypeId type;
    std::string display;
    std::string value;
    bool read_only{};
    bool aligned{};
    bool string_value{};
    std::string provenance;
};

} // namespace ghidra::core
