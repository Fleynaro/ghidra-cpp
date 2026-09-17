export module recode.core.memory_region;

import std;
import recode.core.address_range;
import recode.core.identifiers;

export namespace recode::core {

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

} // namespace recode::core
