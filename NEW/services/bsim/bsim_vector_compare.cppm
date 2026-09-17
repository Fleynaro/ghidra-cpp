export module recode.service.bsim.vector_compare;

import std;

// Ported from:
// Ghidra/Framework/Generic/src/main/java/generic/lsh/vector/VectorCompare.java
// Original class: VectorCompare.  Field names retain the original meanings;
// counts use a wider unsigned type at the contract boundary.

export namespace recode::services::bsim {

/// Holds intermediate sparse-vector overlap results before significance fill-out.
struct VectorCompare {
    double dotproduct{};
    std::uint64_t acount{};
    std::uint64_t bcount{};
    std::uint64_t intersectcount{};
    std::uint64_t min{};
    std::uint64_t max{};
    std::uint64_t numflip{};
    std::uint64_t diff{};

    /// Fills minimum, maximum, flipped, and difference counts exactly as Java.
    void fill_out() noexcept {
        if (acount < bcount) {
            min = acount;
            max = bcount;
        } else {
            min = bcount;
            max = acount;
        }
        diff = max - min;
        numflip = min - intersectcount;
    }

    /// Java-style spelling retained for source-to-source comparisons.
    void fillOut() noexcept {
        fill_out();
    }

    /// Returns the raw dot product using the contract's descriptive spelling.
    [[nodiscard]] double dot_product() const noexcept {
        return dotproduct;
    }

    /// Returns the first vector's multiplicity count.
    [[nodiscard]] std::uint64_t first_count() const noexcept {
        return acount;
    }

    /// Returns the second vector's multiplicity count.
    [[nodiscard]] std::uint64_t second_count() const noexcept {
        return bcount;
    }
};

} // namespace recode::services::bsim
