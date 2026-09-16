export module ghidra.core.debugger;

import std;
import ghidra.core.address;
import ghidra.core.address_range;
import ghidra.core.bytes;

// Porting reference:
// Ghidra/Debug/Debugger-api/src/main/java/ghidra/debug/api/target/TargetObject.java
// and Ghidra/Debug/Debugger-api/src/main/java/ghidra/debug/api/tracemgr/DebuggerCoordinates.java
// provide the semantic debugger vocabulary.  This module deliberately keeps
// that vocabulary independent of a particular debugger protocol or operating
// system so the same contract can be used by local and remote implementations.

export namespace ghidra::core::debugger {

/// Identifies a debugger-owned session without exposing a backend handle.
struct SessionId {
    std::uint64_t value{};

    /// Compares session identities by their stable numeric value.
    friend auto operator<=>(const SessionId&, const SessionId&) = default;
};

/// Identifies a target process using the backend-independent process identity.
struct ProcessId {
    std::string value;

    /// Constructs an empty identity for an unavailable process.
    ProcessId() = default;

    /// Constructs an opaque process identity from a serialized locator.
    explicit ProcessId(std::string identity) : value(std::move(identity)) {}

    /// Compares process identities by value.
    friend auto operator<=>(const ProcessId&, const ProcessId&) = default;
};

/// Identifies a target thread using the backend-independent thread identity.
struct ThreadId {
    std::string value;

    /// Constructs an empty identity for an unavailable thread.
    ThreadId() = default;

    /// Constructs an opaque thread identity from a serialized locator.
    explicit ThreadId(std::string identity) : value(std::move(identity)) {}

    /// Compares thread identities by value.
    friend auto operator<=>(const ThreadId&, const ThreadId&) = default;
};

/// Identifies a user-visible code breakpoint.
struct BreakpointId {
    std::uint64_t value{};

    /// Compares breakpoint identities by value.
    friend auto operator<=>(const BreakpointId&, const BreakpointId&) = default;
};

/// Identifies a user-visible data breakpoint/watchpoint.
struct WatchpointId {
    std::uint64_t value{};

    /// Compares watchpoint identities by value.
    friend auto operator<=>(const WatchpointId&, const WatchpointId&) = default;
};

/// Describes the lifecycle state of one debugger session.
enum class SessionState : std::uint8_t {
    created,
    launching,
    attaching,
    running,
    stopped,
    exiting,
    exited,
    detached,
    failed,
};

/// Describes whether target execution is currently able to make progress.
enum class ExecutionState : std::uint8_t { unknown, running, stopped, exited };

/// Describes the state of a target process independently of its host platform.
enum class ProcessState : std::uint8_t { unknown, created, running, stopped, exited, detached };

/// Describes the state of one target thread.
enum class ThreadState : std::uint8_t { unknown, running, stopped, blocked, exited };

/// Describes the target process identity and current lifecycle state.
struct Process {
    ProcessId id;
    std::string image;
    std::string command_line;
    ProcessState state{ProcessState::unknown};
    std::optional<std::int64_t> exit_code;
    std::string target_description;

    /// Compares process snapshots by their complete value state.
    friend bool operator==(const Process&, const Process&) = default;
};

/// Describes one target thread and its last known instruction address.
struct Thread {
    ThreadId id;
    std::string name;
    ThreadState state{ThreadState::unknown};
    bool current{};
    std::optional<Address> instruction_pointer;

    /// Compares thread snapshots by their complete value state.
    friend bool operator==(const Thread&, const Thread&) = default;
};

/// Describes byte order used when a register value is interpreted numerically.
enum class ByteOrder : std::uint8_t { unknown, little, big };

/// Describes one architecture register without exposing a native register file.
struct Register {
    std::string name;
    std::uint32_t bit_width{};
    bool general_purpose{};
    bool instruction_pointer{};
    bool stack_pointer{};
    bool frame_pointer{};
    bool flags{};
    bool vector{};

    /// Compares register descriptors by their semantic fields.
    friend bool operator==(const Register&, const Register&) = default;
};

/// Carries a register descriptor and its opaque, endian-preserving value bytes.
struct RegisterValue {
    Register register_info;
    Bytes raw_value;
    ByteOrder byte_order{ByteOrder::unknown};

    /// Returns a numeric value only when the target byte order is explicitly known and little-endian.
    [[nodiscard]] std::optional<std::uint64_t> unsigned_value() const {
        if (byte_order != ByteOrder::little || raw_value.size() == 0 || raw_value.size() > sizeof(std::uint64_t))
            return std::nullopt;
        std::uint64_t value{};
        for (std::size_t index = 0; index < raw_value.size(); ++index)
            value |= static_cast<std::uint64_t>(raw_value.values()[index]) << (index * 8U);
        return value;
    }

    /// Compares register snapshots by descriptor and raw bytes.
    friend bool operator==(const RegisterValue&, const RegisterValue&) = default;
};

/// Describes a loaded image/module in the target address space.
struct Module {
    std::string name;
    std::string path;
    Address base;
    std::uint64_t size{};
    bool symbols_loaded{};

    /// Compares module snapshots by their complete value state.
    friend bool operator==(const Module&, const Module&) = default;
};

/// Describes one readable/writable/executable target memory region.
struct MemoryRegion {
    Address start;
    std::uint64_t size{};
    bool readable{};
    bool writable{};
    bool executable{};
    std::string name;

    /// Reports whether the region contains a half-open address range.
    [[nodiscard]] bool contains(Address address) const noexcept {
        return address.space == start.space && address.offset >= start.offset && address.offset - start.offset < size;
    }
};

/// Preserves completeness information for a target memory read.
struct MemoryReadResult {
    Address start;
    std::size_t requested_size{};
    Bytes bytes;
    std::size_t transferred_size{};

    /// Reports whether every requested byte was transferred.
    [[nodiscard]] bool complete() const noexcept {
        return transferred_size == requested_size;
    }
};

/// Describes one frame returned by a target stack walk.
struct StackFrame {
    std::uint32_t level{};
    Address instruction;
    std::optional<Address> stack_pointer;
    std::optional<Address> frame_pointer;
    std::string function;
    std::string module;
    std::string source;

    /// Compares stack-frame snapshots by their complete value state.
    friend bool operator==(const StackFrame&, const StackFrame&) = default;
};

/// Selects the semantic kind of a code breakpoint.
enum class BreakpointKind : std::uint8_t { software, hardware, temporary };

/// Describes an installed code breakpoint.
struct Breakpoint {
    BreakpointId id;
    BreakpointKind kind{BreakpointKind::software};
    Address location;
    bool enabled{};
    bool one_shot{};
    std::uint64_t hit_count{};
    std::string symbol;
};

/// Selects which data access should stop execution.
enum class WatchpointAccess : std::uint8_t { execute, read, write, read_write };

/// Describes an installed data breakpoint/watchpoint.
struct Watchpoint {
    WatchpointId id;
    Address location;
    std::uint64_t size{};
    WatchpointAccess access{WatchpointAccess::write};
    bool enabled{};
    std::uint64_t hit_count{};
};

/// Carries an exception without exposing a platform exception-record type.
struct DebugException {
    std::uint64_t code{};
    Address location;
    bool first_chance{};
    std::string description;
};

/// Explains why a wait-for-stop operation completed.
enum class StopReasonKind : std::uint8_t {
    initial_stop,
    breakpoint,
    watchpoint,
    exception,
    pause,
    step_complete,
    thread_event,
    process_exit,
    module_event,
    unknown,
};

/// Provides a stable semantic result for continue, pause, and stepping tasks.
struct StopReason {
    StopReasonKind kind{StopReasonKind::unknown};
    std::optional<BreakpointId> breakpoint;
    std::optional<WatchpointId> watchpoint;
    std::optional<ThreadId> thread;
    std::optional<DebugException> exception;
    std::string description;
};

/// Identifies the kind of event retained by the session event queue.
enum class EventKind : std::uint8_t {
    process_created,
    process_exited,
    thread_created,
    thread_exited,
    module_loaded,
    module_unloaded,
    breakpoint_hit,
    watchpoint_hit,
    exception,
    execution_stopped,
};

/// Describes a process creation event.
struct ProcessCreatedEvent {
    Process process;
};

/// Describes a process termination event.
struct ProcessExitedEvent {
    ProcessId process;
    std::int64_t exit_code{};
};

/// Describes a thread creation event.
struct ThreadCreatedEvent {
    ProcessId process;
    Thread thread;
};

/// Describes a thread termination event.
struct ThreadExitedEvent {
    ProcessId process;
    ThreadId thread;
    std::int64_t exit_code{};
};

/// Describes a module load event.
struct ModuleLoadedEvent {
    ProcessId process;
    Module module;
};

/// Describes a module unload event.
struct ModuleUnloadedEvent {
    ProcessId process;
    Address base;
    std::string name;
};

/// Describes a code breakpoint hit event.
struct BreakpointHitEvent {
    Breakpoint breakpoint;
    ThreadId thread;
};

/// Describes a data breakpoint/watchpoint hit event.
struct WatchpointHitEvent {
    Watchpoint watchpoint;
    ThreadId thread;
};

/// Describes an exception event.
struct ExceptionEvent {
    ProcessId process;
    ThreadId thread;
    DebugException exception;
};

/// Describes an execution stop after the backend has returned from its wait call.
struct ExecutionStoppedEvent {
    StopReason reason;
    ExecutionState state{ExecutionState::stopped};
};

using EventPayload = std::variant<std::monostate, ProcessCreatedEvent, ProcessExitedEvent, ThreadCreatedEvent,
                                  ThreadExitedEvent, ModuleLoadedEvent, ModuleUnloadedEvent, BreakpointHitEvent,
                                  WatchpointHitEvent, ExceptionEvent, ExecutionStoppedEvent>;

/// Owns one translated debugger event and its monotonically increasing order.
struct DebugEvent {
    std::uint64_t sequence{};
    EventKind kind{EventKind::execution_stopped};
    EventPayload payload;
};

/// Supplies immutable launch parameters to any debugger backend.
struct LaunchRequest {
    std::filesystem::path program;
    std::vector<std::string> arguments;
    std::filesystem::path working_directory;
    std::map<std::string, std::string> environment;
    bool inherit_environment{true};
};

/// Supplies a process identity for an attach operation.
struct AttachRequest {
    ProcessId process;
};

/// Configures session-level policies shared by local and remote backends.
struct SessionOptions {
    std::string target_description;
    bool stop_on_shared_library_events{};
    bool stop_on_first_chance_exceptions{true};
    bool stop_on_thread_events{};
    std::chrono::milliseconds operation_timeout{std::chrono::seconds(30)};
};

} // namespace ghidra::core::debugger

export namespace std {

/// Hashes session identities for backend-independent lookup tables.
template <> struct hash<ghidra::core::debugger::SessionId> {
    [[nodiscard]] std::size_t operator()(ghidra::core::debugger::SessionId value) const noexcept {
        return std::hash<std::uint64_t>{}(value.value);
    }
};

template <> struct hash<ghidra::core::debugger::ProcessId> {
    [[nodiscard]] std::size_t operator()(ghidra::core::debugger::ProcessId value) const noexcept {
        return std::hash<std::string>{}(value.value);
    }
};

template <> struct hash<ghidra::core::debugger::ThreadId> {
    [[nodiscard]] std::size_t operator()(ghidra::core::debugger::ThreadId value) const noexcept {
        return std::hash<std::string>{}(value.value);
    }
};

template <> struct hash<ghidra::core::debugger::BreakpointId> {
    [[nodiscard]] std::size_t operator()(ghidra::core::debugger::BreakpointId value) const noexcept {
        return std::hash<std::uint64_t>{}(value.value);
    }
};

template <> struct hash<ghidra::core::debugger::WatchpointId> {
    [[nodiscard]] std::size_t operator()(ghidra::core::debugger::WatchpointId value) const noexcept {
        return std::hash<std::uint64_t>{}(value.value);
    }
};

} // namespace std
