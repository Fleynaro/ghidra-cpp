// `register` is a C++ keyword, so the source path uses the architecture catalog name
// while the compiled module uses an equivalent legal module identifier.
export module recode.core.register_descriptor;

export import recode.core.storage_location;

export namespace recode::core {

/// Provides the catalog name for register descriptors while retaining one storage representation.
using Register = RegisterDescriptor;

} // namespace recode::core
