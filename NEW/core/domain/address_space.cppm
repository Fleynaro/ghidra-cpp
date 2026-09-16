export module ghidra.core.address_space;

import std;

export namespace ghidra::core {

/// Identifies the semantic kind of an address space.
enum class AddressSpaceKind : std::uint8_t {
    ram,
    code,
    reg,
    stack,
    constant,
    unique,
    join,
    external,
    variable,
    other,
};

/// Names one address space without exposing native Ghidra pointers.
class AddressSpaceId final {
public:
    /// Constructs the default unnamed space identifier.
    AddressSpaceId() = default;

    /// Constructs an identifier from a stable space name.
    explicit AddressSpaceId(std::string name) : name_(std::move(name)) {}

    /// Returns the stable space name.
    [[nodiscard]] const std::string& name() const noexcept {
        return name_;
    }

    /// Compares space identifiers by name.
    friend auto operator<=>(const AddressSpaceId&, const AddressSpaceId&) = default;

private:
    std::string name_;
};

/// Describes immutable address-space properties needed by providers and serializers.
struct AddressSpaceDescriptor {
    AddressSpaceId id;
    AddressSpaceKind kind{AddressSpaceKind::other};
    std::uint32_t address_bits{64};
    std::uint32_t addressable_unit_size{1};
    std::uint32_t pointer_size{8};
    bool big_endian{};
    bool signed_offsets{};
    bool physical{};
};

} // namespace ghidra::core

export namespace std {

/// Hashes address-space IDs for provider indexes.
template <> struct hash<ghidra::core::AddressSpaceId> {
    [[nodiscard]] std::size_t operator()(const ghidra::core::AddressSpaceId& id) const noexcept {
        return std::hash<std::string>{}(id.name());
    }
};

} // namespace std
