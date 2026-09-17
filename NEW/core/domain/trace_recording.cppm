export module recode.core.trace_recording;

import std;

// This vocabulary is deliberately independent of TTD, operating systems, and
// recorder command lines. Implementations may map it to any replayable format.

export namespace recode::core::trace_recording {

/// Describes one target execution that should be captured as a replay trace.
struct RecordingRequest {
    std::filesystem::path program;
    std::vector<std::string> arguments;
    std::filesystem::path output_trace;
    std::filesystem::path working_directory;
    std::map<std::string, std::string> environment;
    bool inherit_environment{true};
    bool record_children{};
    std::map<std::string, std::string> options;
};

/// Reports the completed recorder process and generated artifact.
struct RecordingResult {
    std::filesystem::path trace;
    std::int64_t exit_code{};
    bool completed{};
    bool cancelled{};
    std::string diagnostic;
};

} // namespace recode::core::trace_recording
