export module recode.service.bsim.cosine_vector;

import std;
import recode.core.contracts.bsim;
import recode.service.bsim.hash_entry;
import recode.service.bsim.idf;
import recode.service.bsim.vector_compare;
import recode.service.bsim.weight_factory;

// Ported from:
// Ghidra/Framework/Generic/src/main/java/generic/lsh/vector/LSHCosineVector.java
// Original class: LSHCosineVector.  The optional accumulator below is ported
// from LSHCosineVectorAccum.java and retains unsigned ordering and lazy finalization.

export namespace recode::services::bsim {

namespace detail {

/// Computes one reflected CRC-32 table entry used by SimpleCRC32.hashOneByte.
[[nodiscard]] constexpr std::uint32_t crc_table_entry(std::uint32_t value) noexcept {
    for (int bit = 0; bit < 8; ++bit)
        value = (value & 1U) != 0U ? (value >> 1U) ^ 0xedb88320U : value >> 1U;
    return value;
}

/// Performs the exact byte-at-a-time CRC update used by SimpleCRC32.java.
[[nodiscard]] constexpr std::uint32_t hash_one_byte(std::uint32_t hash, std::uint32_t value) noexcept {
    return crc_table_entry((hash ^ value) & 0xffU) ^ (hash >> 8U);
}

/// Compares hashes as unsigned values, independent of host integer ordering.
[[nodiscard]] constexpr bool unsigned_less(std::uint32_t left, std::uint32_t right) noexcept {
    return left < right;
}

} // namespace detail

/// Owns a sorted sparse list of weighted hash entries and its cosine metadata.
class CosineVector {
public:
    /// Constructs the empty vector used by the Java template and zero-vector factory.
    CosineVector() noexcept = default;

    /// Builds a weighted vector from an already unsigned-sorted, duplicate-preserving hash list.
    ///
    /// Ghidra requires sorted input and does not sort or reject it.  This port
    /// therefore coalesces only adjacent equal values, preserving that contract.
    CosineVector(std::span<const std::uint32_t> features, const WeightFactory& weights, const IDFLookup& lookup) {
        install_features(features, weights, lookup);
        calculate_length();
    }

    /// Reconstructs a cosine vector from the independent sparse contract value.
    explicit CosineVector(const recode::core::contracts::SimilarityVector& value) {
        entries_.reserve(value.entries.size());
        for (const auto& entry : value.entries)
            entries_.emplace_back(entry.hash, entry.term_frequency, entry.idf, entry.coefficient);
        calculate_length();
    }

    /// Replaces entries without sorting them, then recalculates length and multiplicity.
    void set_hash_entries(std::span<const HashEntry> entries) {
        entries_.assign(entries.begin(), entries.end());
        calculate_length();
    }

    /// Returns the number of unique sparse entries.
    [[nodiscard]] std::size_t num_entries() const noexcept {
        return entries_.size();
    }

    /// Returns one sparse entry with bounds checking.
    [[nodiscard]] const HashEntry& get_entry(std::size_t index) const {
        return entries_.at(index);
    }

    /// Returns all sparse entries in their original unsigned-sorted order.
    [[nodiscard]] const std::vector<HashEntry>& get_entries() const noexcept {
        return entries_;
    }

    /// Returns the Euclidean coefficient length.
    [[nodiscard]] double get_length() const noexcept {
        return length_;
    }

    /// Returns total TF multiplicity, not the number of unique entries.
    [[nodiscard]] std::uint64_t get_hash_count() const noexcept {
        return hash_count_;
    }

    /// Computes the two-register unique hash using Ghidra's seven CRC updates.
    [[nodiscard]] std::uint64_t calc_unique_hash() const noexcept {
        std::uint32_t register_one = 0x12cf93abU;
        std::uint32_t register_two = 0xee39b2d6U;
        for (const auto& entry : entries_) {
            const auto current_tf = static_cast<std::uint32_t>(entry.get_tf());
            const auto current_hash = entry.get_hash();
            const auto old_register_one = register_one;
            register_one = detail::hash_one_byte(register_one, current_tf);
            register_one = detail::hash_one_byte(register_one, current_hash);
            register_one = detail::hash_one_byte(register_one, register_two >> 24U);
            register_two = detail::hash_one_byte(register_two, old_register_one >> 24U);
            register_two = detail::hash_one_byte(register_two, current_hash >> 8U);
            register_two = detail::hash_one_byte(register_two, current_hash >> 16U);
            register_two = detail::hash_one_byte(register_two, current_hash >> 24U);
        }
        return (static_cast<std::uint64_t>(register_one) << 32U) | register_two;
    }

    /// Returns the Java hashCode bits, which are the low 32 bits of unique hash.
    [[nodiscard]] std::uint32_t hash_code() const noexcept {
        return static_cast<std::uint32_t>(calc_unique_hash());
    }

    /// Compares two vectors by unsigned merge and returns normalized cosine.
    ///
    /// For equal hashes, the lower TF coefficient contributes its square.  A
    /// tie intentionally chooses the second vector, matching Java's `else`.
    double compare(const CosineVector& other, VectorCompare& data) const noexcept {
        std::size_t first = 0;
        std::size_t second = 0;
        double result = 0.0;
        std::uint64_t intersection = 0;

        if (!entries_.empty() && !other.entries_.empty()) {
            while (first < entries_.size() && second < other.entries_.size()) {
                const auto first_hash = entries_[first].get_hash();
                const auto second_hash = other.entries_[second].get_hash();
                if (first_hash == second_hash) {
                    const auto first_tf = entries_[first].get_tf();
                    const auto second_tf = other.entries_[second].get_tf();
                    if (first_tf < second_tf) {
                        const double coefficient = entries_[first].get_coeff();
                        result += coefficient * coefficient;
                        intersection += static_cast<std::uint64_t>(first_tf);
                    } else {
                        const double coefficient = other.entries_[second].get_coeff();
                        result += coefficient * coefficient;
                        intersection += static_cast<std::uint64_t>(second_tf);
                    }
                    ++first;
                    ++second;
                } else if (detail::unsigned_less(first_hash, second_hash)) {
                    ++first;
                } else {
                    ++second;
                }
            }
            data.dotproduct = result;
            result /= (length_ * other.length_);
        } else {
            data.dotproduct = result;
        }

        data.intersectcount = intersection;
        data.acount = hash_count_;
        data.bcount = other.hash_count_;
        return result;
    }

    /// Computes only multiplicity intersection and leaves dotproduct untouched.
    void compare_counts(const CosineVector& other, VectorCompare& data) const noexcept {
        std::size_t first = 0;
        std::size_t second = 0;
        std::uint64_t intersection = 0;

        while (first < entries_.size() && second < other.entries_.size()) {
            const auto first_hash = entries_[first].get_hash();
            const auto second_hash = other.entries_[second].get_hash();
            if (first_hash == second_hash) {
                intersection +=
                    static_cast<std::uint64_t>(std::min(entries_[first].get_tf(), other.entries_[second].get_tf()));
                ++first;
                ++second;
            } else if (detail::unsigned_less(first_hash, second_hash)) {
                ++first;
            } else {
                ++second;
            }
        }
        data.intersectcount = intersection;
        data.acount = hash_count_;
        data.bcount = other.hash_count_;
    }

    /// Materializes this vector through the implementation-independent contract.
    [[nodiscard]] recode::core::contracts::SimilarityVector to_similarity_vector() const {
        recode::core::contracts::SimilarityVector result;
        result.entries.reserve(entries_.size());
        for (const auto& entry : entries_)
            result.entries.push_back(entry.to_contract_entry());
        result.length = length_;
        result.hash_count = hash_count_;
        result.unique_hash = calc_unique_hash();
        return result;
    }

    /// Java-style spelling retained for source-to-source comparisons.
    [[nodiscard]] std::uint64_t calcUniqueHash() const noexcept {
        return calc_unique_hash();
    }

    /// Java-style spelling retained for source-to-source comparisons.
    [[nodiscard]] std::size_t numEntries() const noexcept {
        return num_entries();
    }

    /// Java-style spelling retained for source-to-source comparisons.
    [[nodiscard]] const HashEntry& getEntry(std::size_t index) const {
        return get_entry(index);
    }

    /// Java-style spelling retained for source-to-source comparisons.
    [[nodiscard]] const std::vector<HashEntry>& getEntries() const noexcept {
        return get_entries();
    }

    /// Java-style spelling retained for source-to-source comparisons.
    [[nodiscard]] double getLength() const noexcept {
        return get_length();
    }

    /// Java-style spelling retained for source-to-source comparisons.
    double compareCounts(const CosineVector& other, VectorCompare& data) const noexcept {
        compare_counts(other, data);
        return data.dotproduct;
    }

    /// Compares sparse entry identity, matching Java's array-based equals.
    friend bool operator==(const CosineVector& left, const CosineVector& right) noexcept {
        return left.entries_ == right.entries_;
    }

protected:
    /// Installs a direct sparse entry list and recalculates aggregate values.
    void install_direct_entries(std::vector<HashEntry> entries) {
        entries_ = std::move(entries);
        calculate_length();
    }

private:
    /// Groups adjacent equal feature hashes and computes each coefficient.
    void install_features(std::span<const std::uint32_t> features, const WeightFactory& weights,
                          const IDFLookup& lookup) {
        if (features.empty())
            return;

        entries_.reserve(features.size());
        std::size_t begin = 0;
        while (begin < features.size()) {
            const auto hash = features[begin];
            std::size_t end = begin + 1U;
            while (end < features.size() && features[end] == hash)
                ++end;
            const auto term_count = static_cast<std::int32_t>(end - begin);
            const auto document_count = lookup.empty() ? 0U : lookup.get_count(hash);
            entries_.emplace_back(hash, term_count, static_cast<std::int32_t>(document_count), weights);
            begin = end;
        }
    }

    /// Recomputes coefficient length and multiplicity after entry changes.
    void calculate_length() noexcept {
        double squared_length = 0.0;
        hash_count_ = 0;
        for (const auto& entry : entries_) {
            const double coefficient = entry.get_coeff();
            squared_length += coefficient * coefficient;
            hash_count_ += static_cast<std::uint64_t>(entry.get_tf());
        }
        length_ = std::sqrt(squared_length);
    }

    std::vector<HashEntry> entries_;
    double length_{};
    std::uint64_t hash_count_{};
};

/// Accumulates one explicit coefficient per unsigned hash until first use.
class CosineVectorAccumulator : public CosineVector {
public:
    /// Constructs an empty, mutable accumulation tree.
    CosineVectorAccumulator() = default;

    /// Inserts a hash and weight; duplicate hashes retain the first weight.
    void add_hash(std::uint32_t hash, double weight) {
        if (finalized_)
            throw std::runtime_error("already finalized");
        accumulated_.emplace(hash, weight);
    }

    /// Finalizes the unsigned map into explicit-weight HashEntry values once.
    void do_finalize() {
        if (finalized_)
            return;
        std::vector<HashEntry> entries;
        entries.reserve(accumulated_.size());
        for (const auto& [hash, weight] : accumulated_)
            entries.emplace_back(hash, 1, weight);
        install_direct_entries(std::move(entries));
        accumulated_.clear();
        finalized_ = true;
    }

    /// Returns length after forcing lazy finalization.
    [[nodiscard]] double get_length() {
        do_finalize();
        return CosineVector::get_length();
    }

    /// Finalizes both accumulator operands before comparing them.
    double compare(CosineVectorAccumulator& other, VectorCompare& data) {
        do_finalize();
        other.do_finalize();
        return CosineVector::compare(other, data);
    }

    /// Finalizes this accumulator before comparing it with an ordinary vector.
    double compare(const CosineVector& other, VectorCompare& data) {
        do_finalize();
        return CosineVector::compare(other, data);
    }

    /// Returns the mutable tree size, preserving Java's post-finalization failure.
    [[nodiscard]] std::size_t num_entries() const {
        if (finalized_)
            throw std::logic_error("accumulator has already been finalized");
        return accumulated_.size();
    }

    /// Java-style spelling retained for source-to-source comparisons.
    void addHash(std::uint32_t hash, double weight) {
        add_hash(hash, weight);
    }

    /// Java-style spelling retained for source-to-source comparisons.
    void doFinalize() {
        do_finalize();
    }

    /// Java-style spelling retained for source-to-source comparisons.
    [[nodiscard]] std::size_t numEntries() const {
        return num_entries();
    }

private:
    std::map<std::uint32_t, double> accumulated_;
    bool finalized_{};
};

} // namespace recode::services::bsim
