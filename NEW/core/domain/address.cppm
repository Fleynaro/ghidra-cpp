export module recode.core.address;

import std;
import recode.core.address_space;
import recode.core.diagnostics;

export namespace recode::core {

/// Represents an immutable address in a named address space.
struct Address {
    AddressSpaceId space;
    std::uint64_t offset{};

    /// Compares addresses by space and then offset.
    friend auto operator<=>(const Address&, const Address&) = default;

    /// Adds an offset while reporting unsigned overflow as a domain error.
    [[nodiscard]] Result<Address> add(std::uint64_t amount) const {
        if (amount > std::numeric_limits<std::uint64_t>::max() - offset)
            return std::unexpected(Error::make(DiagnosticCode::invalid_argument, "Address addition overflows"));
        return Address{space, offset + amount};
    }

    /// Subtracts an offset while reporting unsigned underflow as a domain error.
    [[nodiscard]] Result<Address> subtract(std::uint64_t amount) const {
        if (amount > offset)
            return std::unexpected(Error::make(DiagnosticCode::invalid_argument, "Address subtraction underflows"));
        return Address{space, offset - amount};
    }
};

/// Formats an address without assuming a single global address space.
[[nodiscard]] inline std::string format_address(const Address& address) {
    return address.space.name() + ":0x" + [&] {
        std::ostringstream output;
        output << std::hex << address.offset;
        return output.str();
    }();
}

} // namespace recode::core
