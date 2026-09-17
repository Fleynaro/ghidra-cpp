export module ghidra.core.contracts.trace_recorder;

import std;
import ghidra.core.trace_recording;
import ghidra.core.contracts.operation;
import ghidra.core.diagnostics;

export namespace ghidra::core::contracts {

/// Records an execution into an implementation-independent replay artifact.
class ITraceRecorder {
public:
    /// Releases recorder resources.
    virtual ~ITraceRecorder() = default;
    /// Starts, waits for, and reports one recording operation.
    [[nodiscard]] virtual Task<Result<trace_recording::RecordingResult>> record(
        trace_recording::RecordingRequest request, OperationContext context) = 0;
    /// Requests cancellation of the active recording, if supported.
    [[nodiscard]] virtual Result<void> cancel() = 0;
};

} // namespace ghidra::core::contracts
