export module recode.core.contracts.project_query;

import std;
import recode.core.address;
import recode.core.data_object;
import recode.core.function;
import recode.core.identifiers;
import recode.core.instruction;
import recode.core.memory_region;
import recode.core.reference;
import recode.core.symbol;

export namespace recode::core::contracts {

/// Provides a revision-stamped read-only view over the current projection.
class IProjectQuery {
public:
    /// Releases a query provider through its contract.
    virtual ~IProjectQuery() = default;

    /// Returns the revision represented by this query view.
    [[nodiscard]] virtual Revision current_revision() const = 0;

    /// Returns one instruction at an exact address.
    [[nodiscard]] virtual std::optional<Instruction> instruction_at(Address address) const = 0;

    /// Returns one function at an entry address.
    [[nodiscard]] virtual std::optional<FunctionSnapshot> function_at(Address address) const = 0;

    /// Returns the function containing an address, if known.
    [[nodiscard]] virtual std::optional<FunctionSnapshot> function_containing(Address address) const = 0;

    /// Returns all current functions in deterministic entry order.
    [[nodiscard]] virtual std::vector<FunctionSnapshot> functions() const = 0;

    /// Returns all current instructions in deterministic address order.
    [[nodiscard]] virtual std::vector<Instruction> instructions() const = 0;

    /// Returns all current data objects.
    [[nodiscard]] virtual std::vector<DataObject> data_objects() const = 0;

    /// Returns references originating at an address.
    [[nodiscard]] virtual std::vector<Reference> references_from(Address address) const = 0;

    /// Returns current symbols.
    [[nodiscard]] virtual std::vector<Symbol> symbols() const = 0;

    /// Returns current mapped memory regions.
    [[nodiscard]] virtual std::vector<MemoryRegion> memory_regions() const = 0;
};

} // namespace recode::core::contracts
