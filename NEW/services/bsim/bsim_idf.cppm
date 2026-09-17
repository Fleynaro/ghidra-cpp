export module recode.service.bsim.idf;

import std;

// Ported from:
// Ghidra/Framework/Generic/src/main/java/generic/lsh/vector/IDFLookup.java
// Original class: IDFLookup.  The open-addressed table and its empty-slot
// sentinel are retained so lookup behavior does not depend on std::unordered_map.

export namespace recode::services::bsim {

/// Stores the normalized document-frequency count for a 32-bit feature hash.
///
/// The table owns all entries.  A default-constructed lookup is the Java
/// implementation's null-table state; calling set with zero pairs deliberately
/// creates the same two-slot, non-empty table as IDFLookup.set(new int[0]).
class IDFLookup final {
public:
    /// Marks an unoccupied slot exactly as IDFLookup.initializeTable does.
    static constexpr std::uint32_t empty_count = 0xffffffffU;

    /// Represents one physical table slot, including the Java empty sentinel.
    struct IDFEntry {
        std::uint32_t hash{};
        std::uint32_t count{empty_count};
    };

    /// Constructs the uninitialized lookup used by a fresh Java IDFLookup.
    IDFLookup() noexcept = default;

    /// Returns whether this object still has the Java null-table state.
    [[nodiscard]] bool empty() const noexcept {
        return table_.empty();
    }

    /// Looks up a hash and returns zero for an absent hash or an empty table.
    [[nodiscard]] std::uint32_t get_count(std::uint32_t hash) const noexcept {
        if (table_.empty())
            return 0;

        std::size_t position = static_cast<std::size_t>(hash & mask_);
        for (;;) {
            const auto& entry = table_[position];
            if (entry.count == empty_count)
                return 0;
            if (entry.hash == hash)
                return entry.count;
            position = (position + 1U) & mask_;
        }
    }

    /// Returns Java's getCapacity value, which is the mask, not table length.
    [[nodiscard]] std::uint32_t get_capacity() const noexcept {
        return mask_;
    }

    /// Returns a physical hash slot without translating or probing it.
    [[nodiscard]] std::uint32_t get_raw_hash(std::size_t position) const {
        return table_.at(position).hash;
    }

    /// Returns a physical count slot, including empty_count for empty slots.
    [[nodiscard]] std::uint32_t get_raw_count(std::size_t position) const {
        return table_.at(position).count;
    }

    /// Collapses occupied slots into Java's hash/count alternating array.
    ///
    /// A fresh lookup has no Java table and therefore cannot be collapsed;
    /// std::logic_error makes that invalid state explicit instead of silently
    /// returning a different representation.
    [[nodiscard]] std::vector<std::uint32_t> to_array() const {
        if (table_.empty())
            throw std::logic_error("IDFLookup has not been initialized");

        std::vector<std::uint32_t> result;
        result.reserve(table_.size());
        for (const auto& entry : table_) {
            if (entry.count == empty_count)
                continue;
            result.push_back(entry.hash);
            result.push_back(entry.count);
        }
        return result;
    }

    /// Initializes the table from alternating hash/count values.
    ///
    /// The input order and duplicate insertion behavior follow
    /// IDFLookup.set(int[]), whose pairs are hash then count.
    void set(std::span<const std::uint32_t> hash_count_pairs) {
        if ((hash_count_pairs.size() & 1U) != 0U)
            throw std::invalid_argument("IDFLookup requires hash/count pairs");

        size_ = static_cast<std::uint32_t>(hash_count_pairs.size() / 2U);
        initialize_table();
        for (std::size_t index = 0; index < hash_count_pairs.size(); index += 2U)
            insert_hash(hash_count_pairs[index], hash_count_pairs[index + 1U]);
    }

    /// Initializes the table from an owning alternating hash/count vector.
    void set(const std::vector<std::uint32_t>& hash_count_pairs) {
        set(std::span<const std::uint32_t>(hash_count_pairs));
    }

    /// Initializes entries while preserving XML's declared table size.
    ///
    /// IDFLookup.restoreXml sizes its table from the `size` attribute rather
    /// than from the number of child hashes, so resource loading uses this
    /// overload to retain that distinction.
    void set_pairs(std::span<const std::uint32_t> hash_count_pairs, std::uint32_t declared_size) {
        if ((hash_count_pairs.size() & 1U) != 0U)
            throw std::invalid_argument("IDFLookup requires hash/count pairs");

        size_ = declared_size;
        initialize_table();
        for (std::size_t index = 0; index < hash_count_pairs.size(); index += 2U)
            insert_hash(hash_count_pairs[index], hash_count_pairs[index + 1U]);
    }

    /// Initializes the table from explicit hash/count records.
    void set(std::span<const IDFEntry> entries) {
        size_ = static_cast<std::uint32_t>(entries.size());
        initialize_table();
        for (const auto& entry : entries)
            insert_hash(entry.hash, entry.count);
    }

    /// Java-style spelling retained for callers translating the original API.
    [[nodiscard]] std::uint32_t getCount(std::uint32_t hash) const noexcept {
        return get_count(hash);
    }

    /// Java-style spelling retained for callers translating the original API.
    [[nodiscard]] std::uint32_t getCapacity() const noexcept {
        return get_capacity();
    }

    /// Java-style spelling retained for callers translating the original API.
    [[nodiscard]] std::uint32_t getRawHash(std::size_t position) const {
        return get_raw_hash(position);
    }

    /// Java-style spelling retained for callers translating the original API.
    [[nodiscard]] std::uint32_t getRawCount(std::size_t position) const {
        return get_raw_count(position);
    }

    /// Java-style spelling retained for callers translating the original API.
    [[nodiscard]] std::vector<std::uint32_t> toArray() const {
        return to_array();
    }

private:
    /// Allocates the two-times-power-of-two table used by IDFLookup.
    void initialize_table() {
        std::uint64_t mask = 1;
        while (mask < size_)
            mask <<= 1U;
        mask <<= 1U;
        if (mask > std::numeric_limits<std::uint32_t>::max() + 1ULL)
            throw std::length_error("IDFLookup table is too large");

        mask_ = static_cast<std::uint32_t>(mask - 1U);
        table_.assign(static_cast<std::size_t>(mask), IDFEntry{});
        for (auto& entry : table_)
            entry.count = empty_count;
    }

    /// Inserts one pair with linear probing, preserving duplicate semantics.
    void insert_hash(std::uint32_t hash, std::uint32_t count) {
        if (table_.empty())
            throw std::logic_error("IDFLookup table is not initialized");

        std::size_t position = static_cast<std::size_t>(hash & mask_);
        for (;;) {
            auto& entry = table_[position];
            if (entry.count == empty_count) {
                entry.hash = hash;
                entry.count = count;
                return;
            }
            position = (position + 1U) & mask_;
        }
    }

    std::uint32_t size_{};
    std::uint32_t mask_{};
    std::vector<IDFEntry> table_;
};

} // namespace recode::services::bsim
