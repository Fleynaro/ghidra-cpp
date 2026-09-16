export module ghidra.core.scalar;

import std;
import ghidra.core.address;

export namespace ghidra::core {

/// Preserves integer width and semantic interpretation for decoded operands.
struct Scalar {
    std::uint64_t value{};
    std::uint32_t bit_width{};
    bool signed_value{};
    bool address_value{};
    bool relocated{};

    /// Compares scalar value and interpretation metadata.
    friend bool operator==(const Scalar&, const Scalar&) = default;
};

} // namespace ghidra::core
