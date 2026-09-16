export module ghidra.core.contracts.debugger;

import std;
import ghidra.core.debugger;
import ghidra.core.address;
import ghidra.core.bytes;
import ghidra.core.contracts.operation;
import ghidra.core.diagnostics;

// The contract is intentionally semantic.  In particular, no method exposes
// an engine client, native process handle, native status code, protocol number, or
// debugger-specific execution status.  A remote implementation can therefore
// satisfy this interface without pretending that its target is a local host.

export namespace ghidra::core::contracts {

namespace debugger = ghidra::core::debugger;

/// Receives one debugger event after the backend has translated it to a value.
using DebugEventSink = std::function<void(const debugger::DebugEvent&)>;

/// Provides the lifecycle and inspection contract for one debugger session.
class IDebugSession {
public:
    /// Releases the session and all backend resources owned by it.
    virtual ~IDebugSession() = default;

    /// Returns the stable session identity without contacting the backend.
    [[nodiscard]] virtual debugger::SessionId identity() const noexcept = 0;

    /// Returns the last validated lifecycle state.
    [[nodiscard]] virtual debugger::SessionState state() const noexcept = 0;

    /// Launches a target and completes after the first meaningful stop event.
    [[nodiscard]] virtual Task<Result<debugger::Process>> launch(debugger::LaunchRequest request,
                                                                 OperationContext context) = 0;

    /// Attaches to a target and completes after the first meaningful stop event.
    [[nodiscard]] virtual Task<Result<debugger::Process>> attach(debugger::AttachRequest request,
                                                                 OperationContext context) = 0;

    /// Requests target continuation and completes when the target stops again.
    [[nodiscard]] virtual Task<Result<debugger::StopReason>> continue_execution(OperationContext context) = 0;

    /// Requests an asynchronous target break and completes at the resulting stop.
    [[nodiscard]] virtual Task<Result<debugger::StopReason>> pause(OperationContext context) = 0;

    /// Executes one source-independent instruction step into operation.
    [[nodiscard]] virtual Task<Result<debugger::StopReason>> step_into(OperationContext context) = 0;

    /// Executes one instruction or call step over operation.
    [[nodiscard]] virtual Task<Result<debugger::StopReason>> step_over(OperationContext context) = 0;

    /// Executes until the current frame returns to its caller.
    [[nodiscard]] virtual Task<Result<debugger::StopReason>> step_out(OperationContext context) = 0;

    /// Detaches without terminating the target process.
    [[nodiscard]] virtual Result<void> detach() = 0;

    /// Terminates the target when the backend permits termination.
    [[nodiscard]] virtual Result<void> terminate() = 0;

    /// Returns the current process snapshot, if one exists.
    [[nodiscard]] virtual Result<debugger::Process> process() const = 0;

    /// Enumerates all processes represented by the target connection.
    [[nodiscard]] virtual Result<std::vector<debugger::Process>> processes() const = 0;

    /// Enumerates all threads in the current process.
    [[nodiscard]] virtual Result<std::vector<debugger::Thread>> threads() const = 0;

    /// Selects a thread by its portable identity.
    [[nodiscard]] virtual Result<void> select_thread(debugger::ThreadId thread) = 0;

    /// Returns the selected thread identity, if a target is stopped.
    [[nodiscard]] virtual Result<debugger::ThreadId> current_thread() const = 0;

    /// Enumerates registers supported by the selected or requested thread.
    [[nodiscard]] virtual Result<std::vector<debugger::Register>>
    registers(std::optional<debugger::ThreadId> thread = std::nullopt) const = 0;

    /// Reads one selected or requested register value.
    [[nodiscard]] virtual Result<debugger::RegisterValue>
    read_register(std::string_view name, std::optional<debugger::ThreadId> thread = std::nullopt) const = 0;

    /// Reads all selected or requested register values.
    [[nodiscard]] virtual Result<std::vector<debugger::RegisterValue>>
    read_registers(std::optional<debugger::ThreadId> thread = std::nullopt) const = 0;

    /// Reads the instruction pointer of the selected or requested thread.
    [[nodiscard]] virtual Result<Address>
    instruction_pointer(std::optional<debugger::ThreadId> thread = std::nullopt) const = 0;

    /// Reads target virtual memory while stopped or when the backend permits it.
    [[nodiscard]] virtual Result<Bytes> read_memory(Address address, std::size_t size) const = 0;

    /// Writes target virtual memory while stopped or when the backend permits it.
    [[nodiscard]] virtual Result<void> write_memory(Address address, BytesView bytes) = 0;

    /// Enumerates target memory regions visible to the connection.
    [[nodiscard]] virtual Result<std::vector<debugger::MemoryRegion>> memory_regions() const = 0;

    /// Walks the selected or requested thread stack.
    [[nodiscard]] virtual Result<std::vector<debugger::StackFrame>>
    stack_trace(std::optional<debugger::ThreadId> thread = std::nullopt, std::size_t maximum_frames = 64) const = 0;

    /// Enumerates loaded target modules.
    [[nodiscard]] virtual Result<std::vector<debugger::Module>> modules() const = 0;

    /// Resolves a backend-neutral symbol/function name to a target address.
    [[nodiscard]] virtual Result<Address> resolve_symbol(std::string_view name) const = 0;

    /// Installs a semantic code breakpoint at an address.
    [[nodiscard]] virtual Result<debugger::Breakpoint>
    add_breakpoint(Address address, debugger::BreakpointKind kind = debugger::BreakpointKind::software,
                   bool one_shot = false) = 0;

    /// Enables or disables an installed code breakpoint.
    [[nodiscard]] virtual Result<void> enable_breakpoint(debugger::BreakpointId id, bool enabled) = 0;

    /// Removes an installed code breakpoint.
    [[nodiscard]] virtual Result<void> remove_breakpoint(debugger::BreakpointId id) = 0;

    /// Installs a semantic data breakpoint/watchpoint.
    [[nodiscard]] virtual Result<debugger::Watchpoint> add_watchpoint(Address address, std::size_t size,
                                                                      debugger::WatchpointAccess access) = 0;

    /// Enables or disables an installed watchpoint.
    [[nodiscard]] virtual Result<void> enable_watchpoint(debugger::WatchpointId id, bool enabled) = 0;

    /// Removes an installed watchpoint.
    [[nodiscard]] virtual Result<void> remove_watchpoint(debugger::WatchpointId id) = 0;

    /// Drains translated events in backend delivery order.
    [[nodiscard]] virtual Result<std::vector<debugger::DebugEvent>> poll_events() = 0;

    /// Installs a lightweight event sink; the backend invokes it only after capture.
    [[nodiscard]] virtual Result<void> set_event_sink(DebugEventSink sink) = 0;
};

/// Creates implementation-independent debugger sessions.
class IDebugger {
public:
    /// Releases the debugger service and any sessions it owns.
    virtual ~IDebugger() = default;

    /// Creates a session whose backend resources are isolated from other sessions.
    [[nodiscard]] virtual Result<std::shared_ptr<IDebugSession>>
    create_session(debugger::SessionOptions options = {}) = 0;
};

} // namespace ghidra::core::contracts
