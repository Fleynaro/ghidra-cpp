export module recode.core.events.event;

import std;
import recode.core.identifiers;

export namespace recode::core::events {

/// Describes how one state-event partition is updated.
enum class StateOperation : std::uint8_t { upsert, replace_scope, remove, invalidate };

/// Carries common provenance and idempotency metadata for state changes.
struct StateChangeHeader {
    std::string scope;
    std::string producer;
    std::int32_t source_priority{};
    std::optional<AnalysisRunId> analysis_run_id;
    std::uint64_t generation{};
    std::string idempotency_key;
    StateOperation operation{StateOperation::upsert};
};

/// Represents a pre-append event created by a command or service.
struct EventDraft {
    ProjectId project;
    std::string aggregate_kind;
    std::string aggregate_id;
    std::string event_type;
    std::uint32_t schema_version{1};
    CorrelationId correlation;
    std::optional<CausationId> causation;
    std::string source_service;
    std::string idempotency_key;
    std::string payload;
};

/// Represents a committed event with store-assigned identity and ordering.
struct EventEnvelope {
    EventId event_id;
    std::uint64_t global_sequence{};
    ProjectId project;
    std::string aggregate_kind;
    std::string aggregate_id;
    std::string event_type;
    std::uint32_t schema_version{1};
    std::uint64_t created_at{};
    CorrelationId correlation;
    std::optional<CausationId> causation;
    std::string source_service;
    std::string idempotency_key;
    std::uint64_t payload_length{};
    std::uint32_t checksum{};
    std::string payload;
};

/// Groups committed events in one atomic project commit.
struct EventBatch {
    std::vector<EventEnvelope> events;

    /// Returns the final revision represented by this batch.
    [[nodiscard]] std::uint64_t last_sequence() const noexcept {
        return events.empty() ? 0 : events.back().global_sequence;
    }
};

/// Encodes a small deterministic key/value payload without pointer serialization.
[[nodiscard]] inline std::string encode_fields(std::initializer_list<std::pair<std::string, std::string>> fields) {
    std::string result;
    for (const auto& [key, value] : fields) {
        result += std::string(key);
        result.push_back('=');
        for (const char character : value) {
            if (character == '\\' || character == ';' || character == '=')
                result.push_back('\\');
            result.push_back(character);
        }
        result.push_back(';');
    }
    return result;
}

/// Decodes a deterministic event payload into escaped key/value fields.
[[nodiscard]] inline std::map<std::string, std::string> decode_fields(std::string_view payload) {
    std::map<std::string, std::string> fields;
    std::string key;
    std::string value;
    bool reading_value = false;
    bool escaped = false;
    for (const char character : payload) {
        if (escaped) {
            (reading_value ? value : key).push_back(character);
            escaped = false;
        } else if (character == '\\') {
            escaped = true;
        } else if (character == '=' && !reading_value) {
            reading_value = true;
        } else if (character == ';') {
            if (!key.empty())
                fields.emplace(std::move(key), std::move(value));
            key.clear();
            value.clear();
            reading_value = false;
        } else {
            (reading_value ? value : key).push_back(character);
        }
    }
    return fields;
}

/// Computes the stable checksum used for event payload integrity checks.
[[nodiscard]] inline std::uint32_t checksum(std::string_view value) noexcept {
    std::uint32_t result = 2166136261U;
    for (const auto byte : value) {
        result ^= static_cast<std::uint8_t>(byte);
        result *= 16777619U;
    }
    return result;
}

} // namespace recode::core::events
