export module recode.core.contracts.function_id_database;

import std;
import recode.core.function_id;
import recode.core.diagnostics;

export namespace recode::core::contracts {

/// Supplies immutable Function ID candidates from one external database.
class IFunctionIdDatabase {
public:
    /// Releases a database provider through its contract.
    virtual ~IFunctionIdDatabase() = default;

    /// Returns the database's logical identity.
    [[nodiscard]] virtual std::string identity() const = 0;

    /// Queries candidates for one relation-aware hash family.
    [[nodiscard]] virtual Result<std::vector<FunctionIdCandidate>> query(const FunctionHashFamily& hashes) const = 0;
};

} // namespace recode::core::contracts
