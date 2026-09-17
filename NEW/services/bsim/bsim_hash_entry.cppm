export module recode.service.bsim.hash_entry;

import std;
import recode.core.contracts.bsim;
import recode.service.bsim.weight_factory;

// Ported from:
// Ghidra/Framework/Generic/src/main/java/generic/lsh/vector/HashEntry.java
// Original class: HashEntry.  TF is kept internally as count minus one and
// equality intentionally ignores IDF and coefficient, just as Java does.

export namespace recode::services::bsim {

/// Represents one unique sparse feature and its weighted coefficient.
class HashEntry final {
public:
    /// Constructs Java's zero-valued restore placeholder.
    HashEntry() noexcept = default;

    /// Constructs an entry with an explicit coefficient and IDF slot one.
    ///
    /// The upper TF clamp is deliberately `tcnt > 63`, not `tcnt >= 63`:
    /// Java stores count-minus-one in a six-bit logical slot.
    HashEntry(std::uint32_t hash, std::int32_t term_count, double coefficient) noexcept
        : hash_(hash), internal_tf_(clamp_tf(term_count)), idf_(1), coefficient_(coefficient) {}

    /// Computes an entry coefficient from clamped TF and IDF table slots.
    HashEntry(std::uint32_t hash, std::int32_t term_count, std::int32_t document_count, const WeightFactory& weights)
        : hash_(hash), internal_tf_(clamp_tf(term_count)), idf_(clamp_idf(document_count)),
          coefficient_(weights.get_coeff(as_index(idf_), as_index(internal_tf_))) {}

    /// Reconstructs an entry from contract fields without recalculating its coefficient.
    HashEntry(std::uint32_t hash, std::int32_t term_count, std::int32_t document_count, double coefficient) noexcept
        : hash_(hash), internal_tf_(clamp_tf(term_count)), idf_(clamp_idf(document_count)), coefficient_(coefficient) {}

    /// Returns the unsigned 32-bit feature hash.
    [[nodiscard]] std::uint32_t get_hash() const noexcept {
        return hash_;
    }

    /// Returns public TF, which is the internal count-minus-one plus one.
    [[nodiscard]] std::int32_t get_tf() const noexcept {
        return static_cast<std::int32_t>(internal_tf_) + 1;
    }

    /// Returns the clamped IDF lookup slot.
    [[nodiscard]] std::int32_t get_idf() const noexcept {
        return idf_;
    }

    /// Returns the weighted coefficient used by cosine comparison.
    [[nodiscard]] double get_coeff() const noexcept {
        return coefficient_;
    }

    /// Returns the stored count-minus-one TF slot for audit and serialization.
    [[nodiscard]] std::int32_t internal_tf() const noexcept {
        return internal_tf_;
    }

    /// Converts this implementation entry to the independent contract value.
    [[nodiscard]] recode::core::contracts::SimilarityVectorEntry to_contract_entry() const noexcept {
        return recode::core::contracts::SimilarityVectorEntry{hash_, static_cast<std::uint16_t>(get_tf()),
                                                              static_cast<std::uint16_t>(idf_), coefficient_};
    }

    /// Implements Java HashEntry.equals: only hash and internal TF participate.
    friend bool operator==(const HashEntry& left, const HashEntry& right) noexcept {
        return left.hash_ == right.hash_ && left.internal_tf_ == right.internal_tf_;
    }

    /// Implements the bit-preserving Java 31-based hashCode calculation.
    [[nodiscard]] std::uint32_t hash_code() const noexcept {
        std::uint32_t result = 1U;
        result = result * 31U + hash_;
        result = result * 31U + static_cast<std::uint32_t>(internal_tf_);
        return result;
    }

    /// Java-style spelling retained for source-to-source comparisons.
    [[nodiscard]] std::uint32_t getHash() const noexcept {
        return get_hash();
    }

    /// Java-style spelling retained for source-to-source comparisons.
    [[nodiscard]] std::int32_t getTF() const noexcept {
        return get_tf();
    }

    /// Java-style spelling retained for source-to-source comparisons.
    [[nodiscard]] std::int32_t getIDF() const noexcept {
        return get_idf();
    }

    /// Java-style spelling retained for source-to-source comparisons.
    [[nodiscard]] double getCoeff() const noexcept {
        return get_coeff();
    }

private:
    /// Applies HashEntry's upper-only term-frequency clamp.
    [[nodiscard]] static std::int32_t clamp_tf(std::int32_t term_count) noexcept {
        return term_count > 63 ? 63 : term_count - 1;
    }

    /// Applies HashEntry's upper-only inverse-document-frequency clamp.
    [[nodiscard]] static std::int32_t clamp_idf(std::int32_t document_count) noexcept {
        return document_count > 511 ? 511 : document_count;
    }

    /// Converts valid non-negative slots to table indexes without changing bits.
    [[nodiscard]] static std::size_t as_index(std::int32_t value) noexcept {
        return static_cast<std::size_t>(value);
    }

    std::uint32_t hash_{};
    std::int32_t internal_tf_{};
    std::int32_t idf_{};
    double coefficient_{};
};

} // namespace recode::services::bsim
