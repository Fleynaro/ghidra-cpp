export module ghidra.core.storage_location;

import std;
import ghidra.core.address_space;

export namespace ghidra::core {

/// Identifies a fixed-size storage location used by p-code and ABI descriptions.
struct StorageLocation {
    AddressSpaceId space;
    std::uint64_t offset{};
    std::uint32_t size{};

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
