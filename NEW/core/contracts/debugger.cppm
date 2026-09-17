export module ghidra.core.contracts.debugger;

import std;
import ghidra.core.debugger;
import ghidra.core.replay;
import ghidra.core.address;
import ghidra.core.bytes;
import ghidra.core.contracts.operation;
import ghidra.core.diagnostics;

// The contracts contain only portable values. Native DbgEng and TTD interfaces
// remain implementation details of their Windows service adapters.

export namespace ghidra::core::contracts {

namespace debugger = ghidra::core::debugger;
namespace replay = ghidra::core::replay;

/// Receives one translated debugger event after backend capture is complete.
using DebugEventSink = std::function<void(const debugger::DebugEvent&)>;

/// Defines inspection operations meaningful for both live and recorded targets.
class IBaseDebugSession {
public:
    /// Releases the session and backend resources.
    virtual ~IBaseDebugSession() = default;
    /// Returns the stable session identity.
    [[nodiscard]] virtual debugger::SessionId identity() const noexcept = 0;
    /// Returns the cached portable lifecycle state.
    [[nodiscard]] virtual debugger::SessionState state() const noexcept = 0;
    /// Returns the selected process snapshot.
    [[nodiscard]] virtual Result<debugger::Process> process() const = 0;
    /// Enumerates processes represented by the connection or trace.
    [[nodiscard]] virtual Result<std::vector<debugger::Process>> processes() const = 0;
    /// Selects a process context.
    [[nodiscard]] virtual Result<void> select_process(debugger::ProcessId process) = 0;
    /// Returns the selected process identity.
    [[nodiscard]] virtual Result<debugger::ProcessId> current_process_id() const = 0;
    /// Enumerates threads in the selected process or trace position.
    [[nodiscard]] virtual Result<std::vector<debugger::Thread>> threads() const = 0;
    /// Selects a thread context.
    [[nodiscard]] virtual Result<void> select_thread(debugger::ThreadId thread) = 0;
    /// Returns the selected thread identity.
    [[nodiscard]] virtual Result<debugger::ThreadId> current_thread() const = 0;
    /// Enumerates portable register descriptors.
    [[nodiscard]] virtual Result<std::vector<debugger::Register>> registers(
        std::optional<debugger::ThreadId> thread = std::nullopt) const = 0;
    /// Reads one register value.
    [[nodiscard]] virtual Result<debugger::RegisterValue> read_register(
        std::string_view name, std::optional<debugger::ThreadId> thread = std::nullopt) const = 0;
    /// Reads all register values.
    [[nodiscard]] virtual Result<std::vector<debugger::RegisterValue>> read_registers(
        std::optional<debugger::ThreadId> thread = std::nullopt) const = 0;
    /// Reads a thread instruction pointer.
    [[nodiscard]] virtual Result<Address> instruction_pointer(
        std::optional<debugger::ThreadId> thread = std::nullopt) const = 0;
    /// Reads target virtual memory without modifying it.
    [[nodiscard]] virtual Result<debugger::MemoryReadResult> read_memory(Address address, std::size_t size) const = 0;
    /// Enumerates visible memory mappings.
    [[nodiscard]] virtual Result<std::vector<debugger::MemoryRegion>> memory_regions() const = 0;
    /// Walks a target stack.
    [[nodiscard]] virtual Result<std::vector<debugger::StackFrame>> stack_trace(
        std::optional<debugger::ThreadId> thread = std::nullopt, std::size_t maximum_frames = 64) const = 0;
    /// Enumerates modules visible at the current state.
    [[nodiscard]] virtual Result<std::vector<debugger::Module>> modules() const = 0;
    /// Resolves a backend-neutral symbol name.
    [[nodiscard]] virtual Result<Address> resolve_symbol(std::string_view name) const = 0;
    /// Drains translated events in backend delivery order.
    [[nodiscard]] virtual Result<std::vector<debugger::DebugEvent>> poll_events() = 0;
    /// Installs a callback receiving translated events.
    [[nodiscard]] virtual Result<void> set_event_sink(DebugEventSink sink) = 0;
};

/// Adds lifecycle and execution operations that require a live target.
class ILiveDebugSession : public IBaseDebugSession {
public:
    /// Releases live-session resources.
    ~ILiveDebugSession() override = default;
    /// Launches a target and stops at its initial event.
    [[nodiscard]] virtual Task<Result<debugger::Process>> launch(debugger::LaunchRequest request,
                                                                  OperationContext context) = 0;
    /// Attaches to an existing target process.
    [[nodiscard]] virtual Task<Result<debugger::Process>> attach(debugger::AttachRequest request,
                                                                  OperationContext context) = 0;
    /// Continues live execution until the next stop.
    [[nodiscard]] virtual Task<Result<debugger::StopReason>> continue_execution(OperationContext context) = 0;
    /// Interrupts live execution.
    [[nodiscard]] virtual Task<Result<debugger::StopReason>> pause(OperationContext context) = 0;
    /// Steps live execution into one instruction.
    [[nodiscard]] virtual Task<Result<debugger::StopReason>> step_into(OperationContext context) = 0;
    /// Steps live execution over one call/instruction.
    [[nodiscard]] virtual Task<Result<debugger::StopReason>> step_over(OperationContext context) = 0;
    /// Steps live execution out of the current frame.
    [[nodiscard]] virtual Task<Result<debugger::StopReason>> step_out(OperationContext context) = 0;
    /// Detaches without terminating the target.
    [[nodiscard]] virtual Result<void> detach() = 0;
    /// Terminates the live target.
    [[nodiscard]] virtual Result<void> terminate() = 0;
    /// Writes live target memory.
    [[nodiscard]] virtual Result<void> write_memory(Address address, BytesView bytes) = 0;
    /// Installs a live code breakpoint.
    [[nodiscard]] virtual Result<debugger::Breakpoint> add_breakpoint(
        Address address, debugger::BreakpointKind kind = debugger::BreakpointKind::software, bool one_shot = false) = 0;
    /// Enables or disables a live code breakpoint.
    [[nodiscard]] virtual Result<void> enable_breakpoint(debugger::BreakpointId id, bool enabled) = 0;
    /// Removes a live code breakpoint.
    [[nodiscard]] virtual Result<void> remove_breakpoint(debugger::BreakpointId id) = 0;
    /// Installs a live data watchpoint.
    [[nodiscard]] virtual Result<debugger::Watchpoint> add_watchpoint(
        Address address, std::size_t size, debugger::WatchpointAccess access) = 0;
    /// Enables or disables a live data watchpoint.
    [[nodiscard]] virtual Result<void> enable_watchpoint(debugger::WatchpointId id, bool enabled) = 0;
    /// Removes a live data watchpoint.
    [[nodiscard]] virtual Result<void> remove_watchpoint(debugger::WatchpointId id) = 0;
};

/// Keeps the historical live-session name source-compatible for existing users.
using IDebugSession = ILiveDebugSession;

/// Creates live debugger sessions.
class IDebugger {
public:
    /// Releases the live debugger service.
    virtual ~IDebugger() = default;
    /// Creates an isolated live session.
    [[nodiscard]] virtual Result<std::shared_ptr<ILiveDebugSession>> create_session(
        debugger::SessionOptions options = {}) = 0;
};

/// Defines replay-only timeline lifecycle and navigation operations.
class IReplayDebugSession : public IBaseDebugSession {
public:
    /// Releases the trace engine and cursor.
    ~IReplayDebugSession() override = default;
    /// Opens a replayable trace and positions its cursor at the first point.
    [[nodiscard]] virtual Result<void> open_trace(std::filesystem::path trace) = 0;
    /// Closes the trace and releases all replay resources.
    [[nodiscard]] virtual Result<void> close_trace() = 0;
    /// Returns trace metadata and lifetime boundaries.
    [[nodiscard]] virtual Result<replay::TraceInfo> trace_info() const = 0;
    /// Returns the current opaque timeline position.
    [[nodiscard]] virtual Result<replay::Position> position() const = 0;
    /// Seeks to a valid trace position.
    [[nodiscard]] virtual Result<void> seek(replay::Position position) = 0;
    /// Replays forward by a bounded number of positions/instructions.
    [[nodiscard]] virtual Result<replay::StepResult> step_forward(std::uint64_t count = 1) = 0;
    /// Replays backward by a bounded number of positions/instructions.
    [[nodiscard]] virtual Result<replay::StepResult> step_backward(std::uint64_t count = 1) = 0;
    /// Finds the next or previous matching memory access without mutating the trace.
    [[nodiscard]] virtual Result<replay::StepResult> seek_watchpoint(
        Address address, std::size_t size, debugger::WatchpointAccess access, bool forward) = 0;
};

/// Creates replay debugger sessions without exposing TTD implementation types.
class IReplayDebugger {
public:
    /// Releases the replay service.
    virtual ~IReplayDebugger() = default;
    /// Creates an isolated replay session.
    [[nodiscard]] virtual Result<std::shared_ptr<IReplayDebugSession>> create_session(
        debugger::SessionOptions options = {}) = 0;
};

} // namespace ghidra::core::contracts
