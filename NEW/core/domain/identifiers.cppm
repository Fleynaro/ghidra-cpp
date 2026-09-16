export module ghidra.core.identifiers;

import std;

export namespace ghidra::core {

/// Stores a stable identifier without allowing callers to confuse it with an unrelated string.
/// The project uses canonical textual identifiers in the MVP so event codecs remain portable.
template <class Tag> class StrongIdentifier final {
public:
    /// Constructs an empty identifier for optional/uninitialized values.
    StrongIdentifier() = default;

    /// Constructs an identifier from its canonical textual representation.
    explicit StrongIdentifier(std::string value) : value_(std::move(value)) {}

    /// Returns the canonical identifier text.
    [[nodiscard]] const std::string& value() const noexcept {
        return value_;
    }

    /// Reports whether this identifier has a usable value.
    [[nodiscard]] bool empty() const noexcept {
        return value_.empty();
    }

    /// Provides boolean validity checks without implicit string conversion.
    explicit operator bool() const noexcept {
        return !empty();
    }

    /// Compares identifiers by their canonical representation.
    friend auto operator<=>(const StrongIdentifier&, const StrongIdentifier&) = default;

private:
    std::string value_;
};

/// Tags the identifier of a project session.
struct ProjectIdTag;
/// Tags the identifier of an input artifact.
struct ArtifactIdTag;
/// Tags the identifier of a projected entity.
struct EntityIdTag;
/// Tags the identifier of a command request.
struct CommandIdTag;
/// Tags the identifier of a persisted event.
struct EventIdTag;
/// Tags a request/response correlation.
struct CorrelationIdTag;
/// Tags the event or command that caused a new event.
struct CausationIdTag;
/// Tags one automatic analysis run.
struct AnalysisRunIdTag;

/// Stable project identifier.
using ProjectId = StrongIdentifier<ProjectIdTag>;
/// Stable artifact identifier.
using ArtifactId = StrongIdentifier<ArtifactIdTag>;
/// Stable projected entity identifier.
using EntityId = StrongIdentifier<EntityIdTag>;
/// Stable command identifier.
using CommandId = StrongIdentifier<CommandIdTag>;
/// Stable event identifier.
using EventId = StrongIdentifier<EventIdTag>;
/// Stable correlation identifier.
using CorrelationId = StrongIdentifier<CorrelationIdTag>;
/// Stable causation identifier.
using CausationId = StrongIdentifier<CausationIdTag>;
/// Stable analysis-run identifier.
using AnalysisRunId = StrongIdentifier<AnalysisRunIdTag>;

/// Represents a monotonically increasing position in one project's event log.
struct Revision {
    std::uint64_t value{};

    /// Compares revisions by their log position.
    friend auto operator<=>(const Revision&, const Revision&) = default;
};

/// Creates a deterministic identifier suitable for project-local generated entities.
[[nodiscard]] inline std::string make_identifier(std::string_view prefix, std::uint64_t ordinal) {
    return std::string(prefix) + "-" + std::to_string(ordinal);
}

} // namespace ghidra::core

export namespace std {

/// Hashes strong identifiers using their canonical string representation.
template <class Tag> struct hash<ghidra::core::StrongIdentifier<Tag>> {
    [[nodiscard]] std::size_t operator()(const ghidra::core::StrongIdentifier<Tag>& id) const noexcept {
        return std::hash<std::string>{}(id.value());
    }
};

} // namespace std
