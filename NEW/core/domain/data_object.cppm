export module recode.core.data_object;

import std;
import recode.core.address_range;
import recode.core.data_type;
import recode.core.identifiers;

export namespace recode::core {

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

} // namespace recode::core
