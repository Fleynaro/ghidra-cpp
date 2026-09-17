export module recode.core.contracts.resource_manager;

import std;
import recode.core.binary;
import recode.core.diagnostics;

export namespace recode::core::contracts {

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

} // namespace recode::core::contracts
