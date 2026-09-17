export module recode.core.replay;

import std;

// Porting reference: TEST/debugger/TTD/docs/IReplayEngine.h/struct-Position.md.
// The numeric pair is intentionally opaque: valid positions are trace-specific
// and may contain gaps, so callers must not synthesize positions by arithmetic.

export namespace recode::core::replay {

/// Identifies one immutable point in a recorded execution timeline.
struct Position {
    std::uint64_t sequence{};
    std::uint64_t steps{};

    /// Compares positions lexicographically for display and deterministic tests.
    friend auto operator<=>(const Position&, const Position&) = default;
};

/// Describes the first and last valid points available in a trace.
struct PositionRange {
    Position first;
    Position last;

    /// Compares trace lifetimes by their boundary positions.
    friend bool operator==(const PositionRange&, const PositionRange&) = default;
};

/// Reports why a replay operation stopped.
enum class StopReason : std::uint8_t { completed, boundary, watchpoint, exception, thread_event, error, unknown };

/// Carries replay progress and the resulting cursor position.
struct StepResult {
    Position position;
    Position previous_position;
    StopReason reason{StopReason::unknown};
    std::uint64_t steps_executed{};
    std::uint64_t instructions_executed{};
};

/// Summarizes trace-wide metadata without exposing the recorder or replay engine.
struct TraceInfo {
    PositionRange lifetime;
    std::uint32_t process_id{};
    std::uint64_t peb_address{};
    std::size_t thread_count{};
    std::size_t module_count{};
    std::size_t exception_count{};
    std::size_t keyframe_count{};
};

} // namespace recode::core::replay
