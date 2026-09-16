// `register` is a C++ keyword, so the source path uses the architecture catalog name
// while the compiled module uses an equivalent legal module identifier.
export module ghidra.core.register_descriptor;

export import ghidra.core.storage_location;

export namespace ghidra::core {

/// Provides the catalog name for register descriptors while retaining one storage representation.
using Register = RegisterDescriptor;

} // namespace ghidra::core
