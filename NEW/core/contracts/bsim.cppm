export module recode.core.contracts.bsim;

import std;
import recode.core.diagnostics;
import recode.core.function;
import recode.core.normalized_function;

export namespace recode::core::contracts {

/// Carries one sparse weighted feature entry through an implementation-neutral contract.
struct SimilarityVectorEntry {
    std::uint32_t hash{};
    std::uint16_t term_frequency{};
    std::uint16_t idf{};
    double coefficient{};
};

/// Represents a normalized sparse similarity vector and its aggregate statistics.
struct SimilarityVector {
    std::vector<SimilarityVectorEntry> entries;
    double length{};
    std::uint64_t hash_count{};
    std::uint64_t unique_hash{};
};

/// Contains the raw feature hashes emitted by a function feature extractor.
/// The sequence is sorted but intentionally retains duplicate hashes.
struct FunctionSimilarityFeatures {
    std::optional<FunctionKey> function;
    std::vector<std::uint32_t> hashes;
    std::vector<std::uint64_t> direct_call_addresses;
    bool has_unimplemented{};
    bool has_bad_data{};
    std::uint32_t settings{};
    std::uint64_t overall_hash{};
};

/// Carries the intermediate counters produced while comparing two sparse vectors.
struct SimilarityComparison {
    double cosine{};
    double dot_product{};
    std::uint64_t first_hash_count{};
    std::uint64_t second_hash_count{};
    std::uint64_t intersect_count{};
    std::uint64_t minimum_count{};
    std::uint64_t maximum_count{};
    std::uint64_t flipped_count{};
    std::uint64_t difference_count{};
    double significance{};
};

/// Holds the complete value result for one function.
struct FunctionSimilarityResult {
    FunctionSimilarityFeatures features;
    SimilarityVector vector;
};

/// Configures native Ghidra signature settings and vector resource selection.
struct SimilarityOptions {
    std::uint32_t settings{0x49U};
    std::int32_t max_iterations{3};
    std::int32_t max_block_iterations{1};
    std::uint32_t max_varnodes{};
    std::string language_id{"x86:LE:64:default"};
    std::filesystem::path resource_directory;
};

/// Defines an implementation-independent function-similarity service contract.
class IFunctionSimilarityService {
public:
    /// Releases a similarity service through its contract.
    virtual ~IFunctionSimilarityService() = default;

    /// Generates sorted, duplicate-preserving function feature hashes.
    [[nodiscard]] virtual Result<FunctionSimilarityFeatures>
    generate_signature(const NormalizedFunction& function) const = 0;

    /// Generates the weighted sparse vector for one normalized function.
    [[nodiscard]] virtual Result<SimilarityVector>
    generate_vector(const NormalizedFunction& function) const = 0;

    /// Generates both the raw signature and weighted vector in one pass.
    [[nodiscard]] virtual Result<FunctionSimilarityResult>
    analyze(const NormalizedFunction& function) const = 0;

    /// Compares two vectors using cosine and Ghidra’s significance calculation.
    [[nodiscard]] virtual Result<SimilarityComparison>
    compare(const SimilarityVector& first, const SimilarityVector& second) const = 0;
};

} // namespace recode::core::contracts
