export module ghidra.core.storage_location;

import std;
import ghidra.core.address_space;

export namespace ghidra::core {

/// Identifies a fixed-size storage location used by p-code and ABI descriptions.
struct StorageLocation {
    AddressSpaceId space;
    std::uint64_t offset{};
    std::uint32_t size{};

    /// Constructs an empty location for incremental decoder materialization.
    StorageLocation() = default;

    /// Constructs a storage location from a stable address-space name.
    StorageLocation(std::string space_name, std::uint64_t storage_offset, std::uint32_t storage_size)
        : space(std::move(space_name)), offset(storage_offset), size(storage_size) {}

    /// Constructs a storage location from an existing domain address-space identifier.
    StorageLocation(AddressSpaceId storage_space, std::uint64_t storage_offset, std::uint32_t storage_size)
        : space(std::move(storage_space)), offset(storage_offset), size(storage_size) {}

    /// Compares the complete storage identity.
    friend bool operator==(const StorageLocation&, const StorageLocation&) = default;
};

/// Describes a processor register and its optional parent/bit range.
struct RegisterDescriptor {
    std::string name;
    StorageLocation storage;
    std::string display_name;
    std::optional<std::string> parent;
    std::uint32_t bit_offset{};
    std::uint32_t bit_size{};
};

} // namespace ghidra::core
