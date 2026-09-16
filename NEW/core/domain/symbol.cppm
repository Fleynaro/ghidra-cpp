export module ghidra.core.symbol;

import std;
import ghidra.core.address;
import ghidra.core.identifiers;

export namespace ghidra::core {

/// Identifies the origin and trust level of a symbol.
enum class SymbolSource : std::uint8_t { default_name, import, pdb, analysis, user, fid };

/// Classifies a projected symbol.
enum class SymbolKind : std::uint8_t { function, data, label, external, namespace_scope, unknown };

/// Stores an immutable symbol snapshot with source-priority metadata.
struct Symbol {
    EntityId id;
    std::optional<Address> address;
    std::string external_identity;
    std::string name;
    std::string namespace_name;
    SymbolKind kind{SymbolKind::unknown};
    bool primary{};
    SymbolSource source{SymbolSource::analysis};
    std::int32_t source_priority{};
    std::optional<EntityId> supersedes_symbol_id;
};

} // namespace ghidra::core
