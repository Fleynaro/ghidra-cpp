export module recode.service.translation_engine.native.types;

export import recode.core.scalar;
export import recode.core.storage_location;

export namespace recode::services::translation_engine::native {

/// Canonical shared storage value replacing duplicated VarnodeData copies.
using StorageLocation = recode::core::StorageLocation;
/// Canonical scalar interpretation value.
using Scalar = recode::core::Scalar;

} // namespace recode::services::translation_engine::native
