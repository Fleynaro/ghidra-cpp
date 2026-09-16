export module ghidra.service.translation_engine.native.types;

export import ghidra.core.scalar;
export import ghidra.core.storage_location;

export namespace ghidra::services::translation_engine::native {

/// Canonical shared storage value replacing duplicated VarnodeData copies.
using StorageLocation = ghidra::core::StorageLocation;
/// Canonical scalar interpretation value.
using Scalar = ghidra::core::Scalar;

} // namespace ghidra::services::translation_engine::native
