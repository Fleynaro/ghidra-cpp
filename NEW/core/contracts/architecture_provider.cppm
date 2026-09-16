export module ghidra.core.contracts.architecture_provider;

import std;
import ghidra.core.address_factory;
import ghidra.core.architecture;
import ghidra.core.diagnostics;
import ghidra.core.storage_location;

export namespace ghidra::core::contracts {

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

} // namespace ghidra::core::contracts
