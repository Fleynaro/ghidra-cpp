export module ghidra.core.address_range;

import std;
import ghidra.core.address;

export namespace ghidra::core {

/// Represents one inclusive range in a single address space.
struct AddressRange {
    Address start;
    Address end;

    /// Reports whether an address belongs to this inclusive range.
    [[nodiscard]] bool contains(const Address& address) const noexcept {
        return address.space == start.space && start.offset <= address.offset && address.offset <= end.offset;
    }
};

/// Stores normalized, non-overlapping inclusive address ranges.
class AddressRangeSet final {
public:
    /// Adds a range and merges overlap or adjacency in the same space.
    void add(AddressRange range) {
        if (range.start.space != range.end.space || range.start.offset > range.end.offset)
            return;
        std::vector<AddressRange> result;
        bool inserted = false;
        for (const auto& current : ranges_) {
            const bool same_space = current.start.space == range.start.space;
            const bool current_before =
                same_space && current.end.offset < range.start.offset && range.start.offset - current.end.offset > 1;
            const bool current_after =
                same_space && range.end.offset < current.start.offset && current.start.offset - range.end.offset > 1;
            if (current_before) {
                result.push_back(current);
                continue;
            }
            if (current_after) {
                if (!inserted) {
                    result.push_back(range);
                    inserted = true;
                }
                result.push_back(current);
                continue;
            }
            if (!same_space) {
                result.push_back(current);
                continue;
            }
            range.start.offset = std::min(range.start.offset, current.start.offset);
            range.end.offset = std::max(range.end.offset, current.end.offset);
        }
        if (!inserted)
            result.push_back(range);
        std::ranges::sort(result, {}, [](const AddressRange& value) { return value.start; });
        ranges_ = std::move(result);
    }

    /// Reports whether the set contains an address.
    [[nodiscard]] bool contains(const Address& address) const noexcept {
        return std::ranges::any_of(ranges_, [&](const AddressRange& range) { return range.contains(address); });
    }

    /// Returns the normalized ranges in address order.
    [[nodiscard]] const std::vector<AddressRange>& ranges() const noexcept {
        return ranges_;
    }

    /// Reports whether the set has no ranges.
    [[nodiscard]] bool empty() const noexcept {
        return ranges_.empty();
    }

private:
    std::vector<AddressRange> ranges_;
};

} // namespace ghidra::core
