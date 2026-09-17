export module recode.core.trace_analysis;

import std;

export namespace recode::core {

/// Identifies a function by loaded module name and module-relative entry address.
/// The identity remains stable when the recorded process uses ASLR.
struct TraceFunction {
    std::string module;
    std::uint64_t relative_entry{};
    std::string display_name;

    /// Compares function identities by their stable module-relative location and label.
    friend bool operator==(const TraceFunction&, const TraceFunction&) = default;
};

/// Carries one aggregate function invocation count.
struct FunctionCallCount {
    TraceFunction function;
    std::uint64_t call_count{};
};

/// Separates recorder/open/replay/merge costs for an offline analysis run.
struct TraceAnalysisTimings {
    std::chrono::nanoseconds trace_open{};
    std::chrono::nanoseconds replay{};
    std::chrono::nanoseconds aggregation{};
};

/// Contains one complete function-entry mining result.
struct FunctionCallStatistics {
    std::vector<FunctionCallCount> functions;
    std::uint64_t unresolved_entries{};
    std::uint64_t executed_instructions{};
    std::uint64_t analyzed_segments{};
    TraceAnalysisTimings timings;
};

/// Selects one trace and the stable function catalog to mine.
struct TraceAnalysisRequest {
    std::filesystem::path trace;
    std::vector<TraceFunction> functions;
};

} // namespace recode::core
