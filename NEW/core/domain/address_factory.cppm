export module ghidra.core.address_factory;

import std;
import ghidra.core.address;
import ghidra.core.address_space;
import ghidra.core.diagnostics;

export namespace ghidra::core {

/// Provides immutable lookup and parse/format rules for one project's spaces.
class AddressFactory final {
public:
    /// Constructs a factory from a stable descriptor set.
    explicit AddressFactory(std::vector<AddressSpaceDescriptor> descriptors = {})
        : descriptors_(std::move(descriptors)) {}

    /// Finds a descriptor by stable space name.
    [[nodiscard]] std::optional<AddressSpaceDescriptor> descriptor(std::string_view name) const {
        const auto iterator =
            std::ranges::find_if(descriptors_, [&](const auto& value) { return value.id.name() == name; });
        return iterator == descriptors_.end() ? std::nullopt : std::optional{*iterator};
    }

    /// Parses a `space:hex-offset` address representation.
    [[nodiscard]] Result<Address> parse(std::string_view text) const {
        const auto separator = text.find(':');
        if (separator == std::string_view::npos || separator == 0 || separator + 1 >= text.size())
            return std::unexpected(
                Error::make(DiagnosticCode::invalid_argument, "Address must use the space:offset form"));
        std::uint64_t offset{};
        const auto number = text.substr(separator + 1);
        const auto* first = number.data();
        const auto* last = first + number.size();
        const auto [end, error] = std::from_chars(first, last, offset, 16);
        if (error != std::errc{} || end != last)
            return std::unexpected(Error::make(DiagnosticCode::invalid_argument, "Address offset is not hexadecimal"));
        if (!descriptor(text.substr(0, separator)))
            return std::unexpected(Error::make(DiagnosticCode::invalid_argument, "Unknown address space"));
        return Address{AddressSpaceId{std::string(text.substr(0, separator))}, offset};
    }

    /// Returns all descriptors in their stable construction order.
    [[nodiscard]] const std::vector<AddressSpaceDescriptor>& descriptors() const noexcept {
        return descriptors_;
    }

private:
    std::vector<AddressSpaceDescriptor> descriptors_;
};

} // namespace ghidra::core
