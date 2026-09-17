export module recode.core.data_type;

import std;
import recode.core.identifiers;

export namespace recode::core {

/// Classifies a serializable data-type descriptor.
enum class DataTypeKind : std::uint8_t {
    void_type,
    boolean,
    signed_integer,
    unsigned_integer,
    floating_point,
    pointer,
    array,
    structure,
    union_type,
    typedef_type,
    enumeration,
    unknown,
};

/// Describes one field in a structure or union descriptor.
struct DataTypeField {
    std::string name;
    std::string type_id;
    std::uint32_t offset{};
    std::uint32_t bit_size{};
};

/// Stores an immutable, serializable data-type graph node.
struct DataTypeDescriptor {
    std::string id;
    std::string path;
    std::string name;
    DataTypeKind kind{DataTypeKind::unknown};
    std::uint32_t size{};
    std::uint32_t alignment{};
    bool signed_value{};
    std::string element_type;
    std::uint32_t element_count{};
    std::vector<DataTypeField> fields;
    std::vector<std::pair<std::string, std::int64_t>> enum_values;
    std::optional<std::string> source_archive;
    std::string declaration;
};

/// Provides a strong type name for assignments and signatures.
using DataTypeId = StrongIdentifier<struct DataTypeIdTag>;

} // namespace recode::core
