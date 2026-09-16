export module ghidra.core.function_signature;

import std;
import ghidra.core.data_type;
import ghidra.core.storage_location;

export namespace ghidra::core {

/// Describes one parameter in a recovered or user-assigned signature.
struct FunctionParameter {
    std::string name;
    DataTypeId type;
    std::vector<StorageLocation> storage;
    std::uint32_t ordinal{};
    bool indirect{};
};

/// Names an ABI calling convention and its stable resource identity.
struct CallingConvention {
    std::string name;
    std::string compiler_spec_id;
};

/// Stores a complete immutable function signature proposal or snapshot.
struct FunctionSignature {
    std::vector<FunctionParameter> parameters;
    DataTypeId return_type;
    std::vector<StorageLocation> return_storage;
    CallingConvention calling_convention;
    bool variadic{};
    bool no_return{};
    std::string source;
};

} // namespace ghidra::core
