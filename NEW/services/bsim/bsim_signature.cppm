export module recode.service.bsim.signature;

import std;
import recode.core.contracts.bsim;

// Ported from Ghidra/Features/Decompiler/src/decompile/cpp/signature.hh and
// signature.cc: Signature, hash_mixin, and crc_update.

export namespace recode::services::bsim {

/// Stores one Ghidra signature feature while retaining the native 32-bit hash width.
class SignatureFeature final {
public:
    /// Constructs a feature from the native unsigned hash value.
    explicit SignatureFeature(std::uint32_t value) : hash_(value) {}

    /// Returns the feature hash as an unsigned 32-bit value.
    [[nodiscard]] std::uint32_t hash() const noexcept {
        return hash_;
    }

private:
    std::uint32_t hash_{};
};

/// Updates a reflected CRC-32 register with one byte using Ghidra's polynomial.
[[nodiscard]] inline std::uint32_t crc_update(std::uint32_t register_value, std::uint32_t value) noexcept {
    register_value ^= value & 0xffU;
    for (int bit = 0; bit < 8; ++bit)
        register_value = (register_value >> 1U) ^ ((register_value & 1U) != 0U ? 0xedb88320U : 0U);
    return register_value;
}

/// Mixes two 64-bit hash words through the eight CRC rounds used by signature.cc.
[[nodiscard]] inline std::uint64_t hash_mixin(std::uint64_t first, std::uint64_t second) noexcept {
    std::uint32_t high = static_cast<std::uint32_t>(first >> 32U);
    std::uint32_t low = static_cast<std::uint32_t>(first);
    for (int round = 0; round < 8; ++round) {
        const std::uint32_t old_high = high;
        const std::uint32_t old_low = static_cast<std::uint32_t>(second);
        second >>= 8U;
        high = crc_update(high, old_low);
        low = crc_update(low, old_high);
    }
    return (static_cast<std::uint64_t>(high) << 32U) | low;
}

/// Tests the native modifier/check-bit encoding accepted by GraphSigManager.
[[nodiscard]] inline bool valid_signature_settings(std::uint32_t value) noexcept {
    if (value == 0)
        return false;
    constexpr std::uint32_t modifiers = 0x1U | 0x2U | 0x10U | 0x20U | 0x40U;
    constexpr std::uint32_t allowed = (modifiers << 2U) | 1U;
    return (value & ~allowed) == 0;
}

} // namespace recode::services::bsim
