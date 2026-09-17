export module recode.core.contracts.architecture_provider;

import std;
import recode.core.address_factory;
import recode.core.architecture;
import recode.core.diagnostics;
import recode.core.storage_location;

export namespace recode::core::contracts {

/// Supplies immutable language, address-space, and register metadata.
class IArchitectureProvider {
public:
    /// Releases an architecture provider through its contract.
    virtual ~IArchitectureProvider() = default;

    /// Returns the immutable architecture description.
    [[nodiscard]] virtual const ArchitectureDescription& architecture() const = 0;

    /// Returns the project's immutable address factory.
    [[nodiscard]] virtual const AddressFactory& address_factory() const = 0;

    /// Resolves a register by its display/name identity.
    [[nodiscard]] virtual std::optional<RegisterDescriptor> register_named(std::string_view name) const = 0;
};

} // namespace recode::core::contracts
