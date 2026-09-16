export module ghidra.core.memory_region;

import std;
import ghidra.core.address_range;
import ghidra.core.identifiers;

export namespace ghidra::core {

/// Represents effective permissions of a mapped memory region.
struct MemoryPermissions {
    bool readable{};
    bool writable{};
    bool executable{};
};

/// Describes one generic mapped region without importing PE-specific section types.
struct MemoryRegion {
    EntityId id;
    AddressRange range;
    std::string name;
    MemoryPermissions permissions;
    bool initialized{};
    bool headers{};
    bool file_backed{};
    std::optional<std::pair<std::uint64_t, std::uint64_t>> artifact_range;
    std::string provenance;
};

} // namespace ghidra::core
