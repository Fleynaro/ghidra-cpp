export module ghidra.runtime.event_store.codec;

import std;
import ghidra.core;
import ghidra.core.events.event;
import ghidra.core.diagnostics;

export namespace ghidra::runtime::event_store {

namespace core = ghidra::core;

/// Encodes and decodes deterministic length-delimited event envelopes.
class EventCodec final {
public:
    /// Encodes an envelope without relying on C++ object layout.
    [[nodiscard]] static std::string encode(const core::events::EventEnvelope& event) {
        std::string payload;
        append_string(payload, event.event_id.value());
        append_u64(payload, event.global_sequence);
        append_string(payload, event.project.value());
        append_string(payload, event.aggregate_kind);
        append_string(payload, event.aggregate_id);
        append_string(payload, event.event_type);
        append_u32(payload, event.schema_version);
        append_u64(payload, event.created_at);
        append_string(payload, event.correlation.value());
        append_string(payload, event.causation ? event.causation->value() : std::string{});
        append_string(payload, event.source_service);
        append_string(payload, event.idempotency_key);
        append_string(payload, event.payload);
        return payload;
    }

    /// Decodes one complete payload and validates all length boundaries.
    [[nodiscard]] static core::Result<core::events::EventEnvelope> decode(std::string_view payload) {
        Reader reader{payload};
        auto event_id = reader.string();
        auto sequence = reader.u64();
        auto project = reader.string();
        auto aggregate_kind = reader.string();
        auto aggregate_id = reader.string();
        auto event_type = reader.string();
        auto schema = reader.u32();
        auto created_at = reader.u64();
        auto correlation = reader.string();
        auto causation = reader.string();
        auto source = reader.string();
        auto idempotency = reader.string();
        auto body = reader.string();
        if (!event_id || !sequence || !project || !aggregate_kind || !aggregate_id || !event_type || !schema ||
            !created_at || !correlation || !causation || !source || !idempotency || !body || !reader.empty())
            return std::unexpected(core::Error::make(core::DiagnosticCode::event_corrupt,
                                                     "Event payload is truncated or has trailing bytes",
                                                     "Rebuild the projection from complete event frames."));
        core::events::EventEnvelope result;
        result.event_id = core::EventId{std::move(*event_id)};
        result.global_sequence = *sequence;
        result.project = core::ProjectId{std::move(*project)};
        result.aggregate_kind = std::move(*aggregate_kind);
        result.aggregate_id = std::move(*aggregate_id);
        result.event_type = std::move(*event_type);
        result.schema_version = *schema;
        result.created_at = *created_at;
        result.correlation = core::CorrelationId{std::move(*correlation)};
        if (!causation->empty())
            result.causation = core::CausationId{std::move(*causation)};
        result.source_service = std::move(*source);
        result.idempotency_key = std::move(*idempotency);
        result.payload = std::move(*body);
        result.payload_length = result.payload.size();
        result.checksum = core::events::checksum(payload);
        return result;
    }

private:
    /// Appends a little-endian unsigned 32-bit value.
    static void append_u32(std::string& output, std::uint32_t value) {
        for (std::size_t index = 0; index < sizeof(value); ++index)
            output.push_back(static_cast<char>((value >> (index * 8)) & 0xffU));
    }

    /// Appends a little-endian unsigned 64-bit value.
    static void append_u64(std::string& output, std::uint64_t value) {
        for (std::size_t index = 0; index < sizeof(value); ++index)
            output.push_back(static_cast<char>((value >> (index * 8)) & 0xffU));
    }

    /// Appends a bounded length-prefixed string.
    static void append_string(std::string& output, std::string_view value) {
        append_u64(output, value.size());
        output.append(value);
    }

    /// Reads little-endian values and strings while preserving truncation errors.
    class Reader final {
    public:
        /// Creates a reader over one complete encoded envelope.
        explicit Reader(std::string_view value) : value_(value) {}

        /// Reads an unsigned 32-bit value.
        [[nodiscard]] std::optional<std::uint32_t> u32() {
            if (remaining() < sizeof(std::uint32_t))
                return std::nullopt;
            std::uint32_t result{};
            for (std::size_t index = 0; index < sizeof(result); ++index)
                result |= static_cast<std::uint32_t>(static_cast<std::uint8_t>(value_[offset_++])) << (index * 8);
            return result;
        }

        /// Reads an unsigned 64-bit value.
        [[nodiscard]] std::optional<std::uint64_t> u64() {
            if (remaining() < sizeof(std::uint64_t))
                return std::nullopt;
            std::uint64_t result{};
            for (std::size_t index = 0; index < sizeof(result); ++index)
                result |= static_cast<std::uint64_t>(static_cast<std::uint8_t>(value_[offset_++])) << (index * 8);
            return result;
        }

        /// Reads one length-delimited string.
        [[nodiscard]] std::optional<std::string> string() {
            const auto length = u64();
            if (!length || *length > remaining())
                return std::nullopt;
            std::string result(value_.substr(offset_, static_cast<std::size_t>(*length)));
            offset_ += static_cast<std::size_t>(*length);
            return result;
        }

        /// Reports whether all encoded bytes were consumed.
        [[nodiscard]] bool empty() const noexcept {
            return offset_ == value_.size();
        }

    private:
        /// Returns bytes not yet consumed by the reader.
        [[nodiscard]] std::size_t remaining() const noexcept {
            return value_.size() - offset_;
        }

        std::string_view value_;
        std::size_t offset_{};
    };
};

} // namespace ghidra::runtime::event_store
