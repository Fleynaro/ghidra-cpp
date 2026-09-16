export module ghidra.core.contracts.memory_provider;

import std;
import ghidra.core.address;
import ghidra.core.address_range;
import ghidra.core.bytes;
import ghidra.core.memory_region;
import ghidra.core.diagnostics;

export namespace ghidra::core::contracts {

/// Supplies immutable bytes and mapped-region metadata to services.
class IMemoryProvider {
public:
    /// Releases a provider through its contract.
    virtual ~IMemoryProvider() = default;

    /// Reads exactly `size` bytes at an address or returns a checked error.
    [[nodiscard]] virtual Result<Bytes> read(Address address, std::size_t size) const = 0;

    /// Finds the region containing an address, if one is mapped.
    [[nodiscard]] virtual std::optional<MemoryRegion> region_at(Address address) const = 0;

    /// Returns immutable mapped-region snapshots.
    [[nodiscard]] virtual std::vector<MemoryRegion> regions() const = 0;

    /// Returns ranges whose reads may have externally observable side effects.
    /// Providers without volatile memory may return an empty set.
    [[nodiscard]] virtual std::vector<AddressRange> volatile_ranges() const {
        return {};
    }
};

} // namespace ghidra::core::contracts
