export module ghidra.core.diagnostics;

import std;
import ghidra.core.identifiers;

export namespace ghidra::core {

/// Classifies the user-visible seriousness of a diagnostic.
enum class Severity : std::uint8_t { info, warning, error, fatal };

/// Provides stable machine-readable categories for recoverable operations.
enum class DiagnosticCode : std::uint16_t {
    unknown,
    invalid_argument,
    io_failure,
    parse_failure,
    resource_unavailable,
    resource_mismatch,
    conflict,
    cancelled,
    timeout,
    project_closed,
    queue_full,
    event_corrupt,
    projection_failure,
    unsupported,
};

/// Identifies a source location when a diagnostic can point into an artifact.
struct SourceLocation {
    std::optional<std::string> resource;
    std::optional<std::uint64_t> offset;

    /// Compares source locations as value data.
    friend bool operator==(const SourceLocation&, const SourceLocation&) = default;
};

/// Carries a non-fatal observation emitted by a service or runtime component.
struct Diagnostic {
    Severity severity{Severity::info};
    DiagnosticCode code{DiagnosticCode::unknown};
    std::string message;
    std::optional<SourceLocation> location;
    std::string remediation;
};

/// Carries stable error information returned through every service contract.
struct Error {
    DiagnosticCode code{DiagnosticCode::unknown};
    std::string message;
    std::string remediation;
    std::optional<SourceLocation> location;

    /// Creates a concise error value for a failed operation.
    static Error make(DiagnosticCode error_code, std::string text, std::string hint = {}) {
        return Error{error_code, std::move(text), std::move(hint), std::nullopt};
    }
};

/// Defines the expected result type used by core and service APIs.
template <class T> using Result = std::expected<T, Error>;

} // namespace ghidra::core
