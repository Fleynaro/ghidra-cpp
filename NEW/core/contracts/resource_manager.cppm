export module ghidra.core.contracts.resource_manager;

import std;
import ghidra.core.binary;
import ghidra.core.diagnostics;

export namespace ghidra::core::contracts {

/// Loads and leases immutable analysis resources by content identity.
class IResourceManager {
public:
    /// Releases a resource manager through its contract.
    virtual ~IResourceManager() = default;

    /// Verifies that an exact resource identity is available.
    [[nodiscard]] virtual Result<void> require(const ResourceIdentity& identity) const = 0;

    /// Returns the deterministic identity of the configured resource set.
    [[nodiscard]] virtual ResourceSetIdentity resource_set() const = 0;
};

} // namespace ghidra::core::contracts
