export module recode.core.bytes;

import std;

export namespace recode::core {

/// Names one byte in an artifact or memory image.
using Byte = std::uint8_t;

/// Owns a byte sequence returned by a provider or stored in a value snapshot.
class Bytes final {
public:
    /// Constructs an empty byte sequence.
    Bytes() = default;

    /// Copies bytes from a caller-owned range.
    explicit Bytes(std::span<const Byte> bytes) : values_(bytes.begin(), bytes.end()) {}

    /// Takes ownership of an existing byte vector.
    explicit Bytes(std::vector<Byte> bytes) : values_(std::move(bytes)) {}

    /// Returns the owned bytes.
    [[nodiscard]] const std::vector<Byte>& values() const noexcept {
        return values_;
    }

    /// Returns a non-owning read view over the owned bytes.
    [[nodiscard]] std::span<const Byte> view() const noexcept {
        return values_;
    }

    /// Returns the number of bytes.
    [[nodiscard]] std::size_t size() const noexcept {
        return values_.size();
    }

    /// Reports whether no bytes are present.
    [[nodiscard]] bool empty() const noexcept {
        return values_.empty();
    }

    /// Compares byte sequences by content.
    friend bool operator==(const Bytes&, const Bytes&) = default;

private:
    std::vector<Byte> values_;
};

/// Provides a non-owning byte window with an explicit caller lifetime contract.
class BytesView final {
public:
    /// Constructs an empty view.
    BytesView() = default;

    /// Views a caller-owned byte range; the range must outlive this object.
    explicit BytesView(std::span<const Byte> bytes) : view_(bytes) {}

    /// Returns the underlying read-only span.
    [[nodiscard]] std::span<const Byte> span() const noexcept {
        return view_;
    }

    /// Returns the number of visible bytes.
    [[nodiscard]] std::size_t size() const noexcept {
        return view_.size();
    }

    /// Reports whether no bytes are visible.
    [[nodiscard]] bool empty() const noexcept {
        return view_.empty();
    }

private:
    std::span<const Byte> view_;
};

} // namespace recode::core
