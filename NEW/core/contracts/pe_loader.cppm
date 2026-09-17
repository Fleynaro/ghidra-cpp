export module recode.core.contracts.pe_loader;

import std;
import recode.core.architecture;
import recode.core.binary;
import recode.core.memory_region;
import recode.core.relocation;
import recode.core.symbol;
import recode.core.diagnostics;

export namespace recode::core::contracts {

/// Selects strictness and parsing breadth for a PE load.
struct LoadOptions {
    bool strict{true};
    bool parse_directories{true};
    std::uint64_t maximum_image_size{1ULL << 30};
};

/// Contains serializable PE-specific diagnostics without exposing parser objects.
struct PeLoadDetails {
    std::string machine;
    std::uint64_t image_base{};
    std::uint32_t entry_point_rva{};
    std::uint32_t image_size{};
    std::vector<std::string> imports;
    std::vector<std::string> exports;
    std::vector<std::string> directory_diagnostics;
};

/// Classifies whether a PE result contains all requested directories.
enum class PeParseStatus : std::uint8_t { complete, partial };

/// Carries generic image facts and the preserved PE directory summary.
struct PeLoadResult {
    BinaryArtifact artifact;
    ArchitectureDescription architecture;
    std::vector<MemoryRegion> memory_regions;
    std::vector<Symbol> imported_symbols;
    std::vector<Symbol> exported_symbols;
    std::vector<Relocation> relocations;
    PeLoadDetails details;
    PeParseStatus status{PeParseStatus::complete};
};

/// Parses and validates a primary binary artifact.
class IPELoader {
public:
    /// Releases a loader through its contract.
    virtual ~IPELoader() = default;

    /// Loads one artifact without mutating project persistence.
    [[nodiscard]] virtual Result<PeLoadResult> load(const BinaryArtifact& artifact, const LoadOptions& options) = 0;
};

} // namespace recode::core::contracts
