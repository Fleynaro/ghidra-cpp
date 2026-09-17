export module recode.core.scalar;

import std;
import recode.core.address;

export namespace recode::core {

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

} // namespace recode::core
