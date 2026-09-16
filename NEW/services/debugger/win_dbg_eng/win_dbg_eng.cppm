module;

#ifdef _WIN32
#include <dbgeng.h>
#include <windows.h>
#endif

export module ghidra.service.debugger.win_dbg_eng;

import std;
import ghidra.core;

// Implementation references:
// * Ghidra/Debug/Debugger-agent-dbgeng/src/main/py/src/ghidradbg/util.py
//   (the DbgWorker and same-thread dispatch model)
// * Ghidra/Debug/Debugger-agent-dbgeng/src/main/py/src/ghidradbg/commands.py
//   (target enumeration, register/memory/stack/module mapping, and breakpoint
//   semantics)
// * Ghidra/Debug/Debugger-agent-dbgeng/src/main/py/src/ghidradbg/hooks.py
//   (event-to-trace translation and lightweight callback rules)
// * Microsoft DbgEng documentation for DebugCreate, SetExecutionStatus,
//   WaitForEvent, callback objects, and IDebugControl breakpoint methods.
//
// The C++ service uses those behavioral mappings but exposes only the generic
// core debugger contract.  All native interface pointers and Windows values
// remain in this implementation module.

export namespace ghidra::services::debugger::win_dbg_eng {

namespace core = ghidra::core;
namespace api = ghidra::core::contracts;
namespace model = ghidra::core::debugger;

namespace detail {

/// Builds a diagnostic that retains native information without exposing it in a contract type.
[[nodiscard]] inline core::Error native_error(std::string_view operation, std::string detail) {
    return core::Error::make(
        core::DiagnosticCode::resource_unavailable,
        "WinDbgEng " + std::string(operation) + " failed: " + std::move(detail),
        "Verify that the Windows debugging tools are installed and that the target is accessible.");
}

/// Returns a stable error for a backend unavailable on the current platform.
[[nodiscard]] inline core::Error unsupported_platform() {
    return core::Error::make(core::DiagnosticCode::unsupported,
                             "WinDbgEng is available only on Windows builds with DbgEng installed.",
                             "Use a debugger backend appropriate for the current platform.");
}

/// Returns an error for a lifecycle operation that cannot run in the current state.
[[nodiscard]] inline core::Error invalid_state(model::SessionState state, std::string_view operation) {
    return core::Error::make(core::DiagnosticCode::conflict,
                             "WinDbgEng cannot " + std::string(operation) + " in session state " +
                                 std::to_string(static_cast<unsigned>(state)),
                             "Complete or terminate the current debugger operation before retrying.");
}

#ifdef _WIN32
inline constexpr std::uint32_t execution_go = DEBUG_STATUS_GO;
inline constexpr std::uint32_t execution_break = DEBUG_STATUS_BREAK;
inline constexpr std::uint32_t execution_step_into = DEBUG_STATUS_STEP_INTO;
inline constexpr std::uint32_t execution_step_over = DEBUG_STATUS_STEP_OVER;
#else
inline constexpr std::uint32_t execution_go = 0;
inline constexpr std::uint32_t execution_break = 0;
inline constexpr std::uint32_t execution_step_into = 0;
inline constexpr std::uint32_t execution_step_over = 0;
#endif

#ifdef _WIN32

/// Converts an HRESULT to an English diagnostic while preserving its numeric value.
[[nodiscard]] inline core::Error hresult_error(std::string_view operation, HRESULT result) {
    std::ostringstream text;
    text << "HRESULT 0x" << std::hex << static_cast<unsigned long>(result);
    return native_error(operation, text.str());
}

/// Owns one COM-like DbgEng interface and releases it exactly once.
template <class Interface> class NativePtr final {
public:
    /// Constructs an empty native interface owner.
    NativePtr() = default;

    /// Releases the owned interface at scope exit.
    ~NativePtr() {
        reset();
    }

    /// Prevents accidental interface copying.
    NativePtr(const NativePtr&) = delete;

    /// Prevents accidental interface copying.
    NativePtr& operator=(const NativePtr&) = delete;

    /// Transfers interface ownership from another owner.
    NativePtr(NativePtr&& other) noexcept : pointer_(std::exchange(other.pointer_, nullptr)) {}

    /// Releases the current interface and takes ownership from another owner.
    NativePtr& operator=(NativePtr&& other) noexcept {
        if (this != &other) {
            reset();
            pointer_ = std::exchange(other.pointer_, nullptr);
        }
        return *this;
    }

    /// Returns the raw interface pointer for an engine-thread-only call.
    [[nodiscard]] Interface* get() const noexcept {
        return pointer_;
    }

    /// Provides a pointer slot for native factory/query calls.
    [[nodiscard]] Interface** put() noexcept {
        reset();
        return &pointer_;
    }

    /// Releases the interface and clears this owner.
    void reset() noexcept {
        if (pointer_ != nullptr) {
            pointer_->Release();
            pointer_ = nullptr;
        }
    }

    /// Reports whether an interface is currently owned.
    [[nodiscard]] explicit operator bool() const noexcept {
        return pointer_ != nullptr;
    }

private:
    Interface* pointer_{};
};

/// Retains a callback object for the lifetime of the engine thread.
class CallbackBase {
public:
    /// Starts with one owner reference held by the session object.
    CallbackBase() = default;

    /// Prevents copying callback state.
    CallbackBase(const CallbackBase&) = delete;

    /// Prevents copying callback state.
    CallbackBase& operator=(const CallbackBase&) = delete;

    /// Increments the COM-style callback reference count.
    ULONG AddRef() noexcept {
        return ++references_;
    }

    /// Decrements the COM-style callback reference count without deleting the member object.
    ULONG Release() noexcept {
        return references_ > 0 ? --references_ : 0;
    }

private:
    std::atomic<ULONG> references_{1};
};

#endif

} // namespace detail

/// Implements one backend-independent debugger session on a dedicated DbgEng thread.
class WinDbgEngSession final : public api::IDebugSession {
public:
    /// Starts DbgEng on its dedicated engine thread and validates initialization before returning.
    explicit WinDbgEngSession(model::SessionId id, model::SessionOptions options)
        : id_(id), options_(std::move(options)) {
        std::promise<core::Result<void>> ready;
        auto future = ready.get_future();
        engine_ = std::jthread(
            [this, ready = std::move(ready)](std::stop_token stop) mutable { engine_loop(stop, std::move(ready)); });
        const auto result = future.get();
        if (!result)
            throw std::runtime_error(result.error().message);
#ifdef _WIN32
        interrupt_ = std::jthread([this](std::stop_token stop) { interrupt_loop(stop); });
#endif
    }

    /// Requests the engine thread to stop and releases DbgEng after all callbacks are unregistered.
    ~WinDbgEngSession() override {
        stopping_.store(true, std::memory_order_release);
        queue_condition_.notify_all();
#ifdef _WIN32
        request_interrupt();
        interrupt_condition_.notify_all();
#endif
        if (engine_.joinable()) {
            engine_.request_stop();
            engine_.join();
        }
#ifdef _WIN32
        if (interrupt_.joinable()) {
            interrupt_.request_stop();
            interrupt_.join();
        }
#endif
    }

    /// Prevents copying a thread-owning debugger session.
    WinDbgEngSession(const WinDbgEngSession&) = delete;

    /// Prevents copying a thread-owning debugger session.
    WinDbgEngSession& operator=(const WinDbgEngSession&) = delete;

    /// Returns the stable session identity.
    [[nodiscard]] model::SessionId identity() const noexcept override {
        return id_;
    }

    /// Returns the cached lifecycle state without entering DbgEng from the caller thread.
    [[nodiscard]] model::SessionState state() const noexcept override {
        return state_.load(std::memory_order_acquire);
    }

    /// Queues launch and completes only after DbgEng has delivered its initial stop event.
    [[nodiscard]] api::Task<core::Result<model::Process>> launch(model::LaunchRequest request,
                                                                 api::OperationContext context) override {
        return enqueue_wait<model::Process>([this, request = std::move(request)] { return begin_launch(request); },
                                            [this](const model::StopReason&) { return current_process_native(); },
                                            std::move(context));
    }

    /// Queues attach and completes only after WaitForEvent has returned the attach stop.
    [[nodiscard]] api::Task<core::Result<model::Process>> attach(model::AttachRequest request,
                                                                 api::OperationContext context) override {
        return enqueue_wait<model::Process>([this, request] { return begin_attach(request); },
                                            [this](const model::StopReason&) { return current_process_native(); },
                                            std::move(context));
    }

    /// Requests GO and completes after the engine reports a real stop event.
    [[nodiscard]] api::Task<core::Result<model::StopReason>>
    continue_execution(api::OperationContext context) override {
        return enqueue_wait<model::StopReason>(
            [this] { return begin_execution(detail::execution_go, model::StopReasonKind::unknown); },
            [](const model::StopReason& reason) -> core::Result<model::StopReason> { return reason; },
            std::move(context));
    }

    /// Requests a break and completes after the target has actually stopped.
    [[nodiscard]] api::Task<core::Result<model::StopReason>> pause(api::OperationContext context) override {
#ifdef _WIN32
        if (state() == model::SessionState::running) {
            pause_requested_hint_.store(true, std::memory_order_release);
            request_interrupt();
        }
#endif
        return enqueue_pause(std::move(context));
    }

    /// Requests a single instruction step into operation.
    [[nodiscard]] api::Task<core::Result<model::StopReason>> step_into(api::OperationContext context) override {
        return enqueue_wait<model::StopReason>(
            [this] { return begin_execution(detail::execution_step_into, model::StopReasonKind::step_complete); },
            [](const model::StopReason& reason) -> core::Result<model::StopReason> { return reason; },
            std::move(context));
    }

    /// Requests a single step-over operation.
    [[nodiscard]] api::Task<core::Result<model::StopReason>> step_over(api::OperationContext context) override {
        return enqueue_wait<model::StopReason>(
            [this] { return begin_execution(detail::execution_step_over, model::StopReasonKind::step_complete); },
            [](const model::StopReason& reason) -> core::Result<model::StopReason> { return reason; },
            std::move(context));
    }

    /// Implements step-out with a temporary return-address breakpoint because DbgEng has no generic step-out status.
    [[nodiscard]] api::Task<core::Result<model::StopReason>> step_out(api::OperationContext context) override {
        return enqueue_wait<model::StopReason>(
            [this] { return begin_step_out(); },
            [](const model::StopReason& reason) -> core::Result<model::StopReason> { return reason; },
            std::move(context));
    }

    /// Detaches synchronously through the engine thread and resumes the target as required by DbgEng.
    [[nodiscard]] core::Result<void> detach() override {
        return invoke_sync<void>([this] { return detach_native(); });
    }

    /// Terminates synchronously through the engine thread and waits for the client to end its target.
    [[nodiscard]] core::Result<void> terminate() override {
        return invoke_sync<void>([this] { return terminate_native(); });
    }

    /// Returns the current process snapshot through the engine thread.
    [[nodiscard]] core::Result<model::Process> process() const override {
        return invoke_sync<model::Process>([this] { return current_process_native(); });
    }

    /// Enumerates all processes represented by the DbgEng client.
    [[nodiscard]] core::Result<std::vector<model::Process>> processes() const override {
        auto* self = const_cast<WinDbgEngSession*>(this);
        return invoke_sync<std::vector<model::Process>>([self] { return self->processes_native(); });
    }

    /// Selects a process context through IDebugSystemObjects without exposing engine indexes.
    [[nodiscard]] core::Result<void> select_process(model::ProcessId process) override {
        return invoke_sync<void>([this, process] { return select_process_native(process); });
    }

    /// Returns the selected process identity.
    [[nodiscard]] core::Result<model::ProcessId> current_process_id() const override {
        auto* self = const_cast<WinDbgEngSession*>(this);
        return invoke_sync<model::ProcessId>([self] { return self->current_process_id_native(); });
    }

    /// Enumerates all target threads without exposing DbgEng thread indexes.
    [[nodiscard]] core::Result<std::vector<model::Thread>> threads() const override {
        auto* self = const_cast<WinDbgEngSession*>(this);
        return invoke_sync<std::vector<model::Thread>>([self] { return self->threads_native(); });
    }

    /// Selects a target thread through the dedicated engine thread.
    [[nodiscard]] core::Result<void> select_thread(model::ThreadId thread) override {
        return invoke_sync<void>([this, thread] { return select_thread_native(thread); });
    }

    /// Returns the selected operating-system thread identity.
    [[nodiscard]] core::Result<model::ThreadId> current_thread() const override {
        auto* self = const_cast<WinDbgEngSession*>(this);
        return invoke_sync<model::ThreadId>([self] { return self->current_thread_native(); });
    }

    /// Enumerates the register descriptors reported by the target architecture.
    [[nodiscard]] core::Result<std::vector<model::Register>>
    registers(std::optional<model::ThreadId> thread) const override {
        auto* self = const_cast<WinDbgEngSession*>(this);
        return invoke_sync<std::vector<model::Register>>([self, thread] { return self->registers_native(thread); });
    }

    /// Reads one register through IDebugRegisters on the engine thread.
    [[nodiscard]] core::Result<model::RegisterValue>
    read_register(std::string_view name, std::optional<model::ThreadId> thread) const override {
        const std::string requested{name};
        auto* self = const_cast<WinDbgEngSession*>(this);
        return invoke_sync<model::RegisterValue>(
            [self, requested, thread] { return self->read_register_native(requested, thread); });
    }

    /// Reads all available register values through IDebugRegisters.
    [[nodiscard]] core::Result<std::vector<model::RegisterValue>>
    read_registers(std::optional<model::ThreadId> thread) const override {
        auto* self = const_cast<WinDbgEngSession*>(this);
        return invoke_sync<std::vector<model::RegisterValue>>(
            [self, thread] { return self->read_registers_native(thread); });
    }

    /// Reads the instruction pointer from the selected target context.
    [[nodiscard]] core::Result<core::Address>
    instruction_pointer(std::optional<model::ThreadId> thread) const override {
        auto* self = const_cast<WinDbgEngSession*>(this);
        return invoke_sync<core::Address>([self, thread] { return self->instruction_pointer_native(thread); });
    }

    /// Reads target virtual memory through IDebugDataSpaces.
    [[nodiscard]] core::Result<model::MemoryReadResult> read_memory(core::Address address,
                                                                    std::size_t size) const override {
        auto* self = const_cast<WinDbgEngSession*>(this);
        return invoke_sync<model::MemoryReadResult>(
            [self, address, size] { return self->read_memory_native(address, size); });
    }

    /// Writes target virtual memory through IDebugDataSpaces.
    [[nodiscard]] core::Result<void> write_memory(core::Address address, core::BytesView bytes) override {
        std::vector<core::Byte> copy(bytes.span().begin(), bytes.span().end());
        return invoke_sync<void>(
            [this, address, copy = std::move(copy)] { return write_memory_native(address, copy); });
    }

    /// Enumerates virtual memory regions where the backend exposes mappings.
    [[nodiscard]] core::Result<std::vector<model::MemoryRegion>> memory_regions() const override {
        auto* self = const_cast<WinDbgEngSession*>(this);
        return invoke_sync<std::vector<model::MemoryRegion>>([self] { return self->memory_regions_native(); });
    }

    /// Walks a target stack and resolves best-effort symbol names.
    [[nodiscard]] core::Result<std::vector<model::StackFrame>> stack_trace(std::optional<model::ThreadId> thread,
                                                                           std::size_t maximum_frames) const override {
        auto* self = const_cast<WinDbgEngSession*>(this);
        return invoke_sync<std::vector<model::StackFrame>>(
            [self, thread, maximum_frames] { return self->stack_trace_native(thread, maximum_frames); });
    }

    /// Enumerates loaded modules and their base/size metadata.
    [[nodiscard]] core::Result<std::vector<model::Module>> modules() const override {
        auto* self = const_cast<WinDbgEngSession*>(this);
        return invoke_sync<std::vector<model::Module>>([self] { return self->modules_native(); });
    }

    /// Resolves a symbol using the backend's symbol provider without exposing its naming rules.
    [[nodiscard]] core::Result<core::Address> resolve_symbol(std::string_view name) const override {
        const std::string requested{name};
        auto* self = const_cast<WinDbgEngSession*>(this);
        return invoke_sync<core::Address>([self, requested] { return self->resolve_symbol_native(requested); });
    }

    /// Installs a code breakpoint through IDebugControl::AddBreakpoint.
    [[nodiscard]] core::Result<model::Breakpoint> add_breakpoint(core::Address address, model::BreakpointKind kind,
                                                                 bool one_shot) override {
        return invoke_sync<model::Breakpoint>(
            [this, address, kind, one_shot] { return add_breakpoint_native(address, kind, one_shot); });
    }

    /// Enables or disables one installed code breakpoint.
    [[nodiscard]] core::Result<void> enable_breakpoint(model::BreakpointId id, bool enabled) override {
        return invoke_sync<void>([this, id, enabled] { return set_breakpoint_enabled_native(id, enabled); });
    }

    /// Removes one installed code breakpoint.
    [[nodiscard]] core::Result<void> remove_breakpoint(model::BreakpointId id) override {
        return invoke_sync<void>([this, id] { return remove_breakpoint_native(id); });
    }

    /// Installs a data breakpoint/watchpoint with DbgEng access flags.
    [[nodiscard]] core::Result<model::Watchpoint> add_watchpoint(core::Address address, std::size_t size,
                                                                 model::WatchpointAccess access) override {
        return invoke_sync<model::Watchpoint>(
            [this, address, size, access] { return add_watchpoint_native(address, size, access); });
    }

    /// Enables or disables one installed data breakpoint/watchpoint.
    [[nodiscard]] core::Result<void> enable_watchpoint(model::WatchpointId id, bool enabled) override {
        return invoke_sync<void>([this, id, enabled] { return set_watchpoint_enabled_native(id, enabled); });
    }

    /// Removes one installed data breakpoint/watchpoint.
    [[nodiscard]] core::Result<void> remove_watchpoint(model::WatchpointId id) override {
        return invoke_sync<void>([this, id] { return remove_watchpoint_native(id); });
    }

    /// Drains translated events without exposing callback implementation details.
    [[nodiscard]] core::Result<std::vector<model::DebugEvent>> poll_events() override {
        return invoke_sync<std::vector<model::DebugEvent>>([this] {
            std::vector<model::DebugEvent> result;
            result.reserve(events_.size());
            while (!events_.empty()) {
                result.push_back(std::move(events_.front()));
                events_.pop_front();
            }
            return core::Result<std::vector<model::DebugEvent>>{std::move(result)};
        });
    }

    /// Installs a callback that receives already-captured generic events.
    [[nodiscard]] core::Result<void> set_event_sink(api::DebugEventSink sink) override {
        return invoke_sync<void>([this, sink = std::move(sink)]() mutable {
            event_sink_ = std::move(sink);
            return core::Result<void>{};
        });
    }

private:
    using Command = std::function<void()>;

    /// Couples a queued command with a rejection action used during shutdown.
    struct QueuedCommand {
        Command execute;
        Command reject;
    };

    /// Stores a pending wait operation until DbgEng returns from WaitForEvent.
    struct PendingWait {
        std::shared_ptr<api::OperationControl> control;
        std::function<void(const model::StopReason&)> complete;
        std::function<void(core::Error)> fail;
    };

    /// Creates the type-erased completion callback shared by execution and pause waiters.
    template <class Value, class Finish>
    [[nodiscard]] PendingWait make_pending_wait(std::shared_ptr<std::promise<core::Result<Value>>> promise,
                                                std::shared_ptr<api::OperationControl> control, Finish finish) {
        return PendingWait{
            control,
            [promise, control, finish = std::move(finish)](const model::StopReason& reason) mutable {
                if (control->cancellation().stop_requested()) {
                    control->set_status(api::OperationStatus::cancelled);
                    promise->set_value(std::unexpected(
                        core::Error::make(core::DiagnosticCode::cancelled, "Debugger operation was cancelled")));
                    return;
                }
                try {
                    auto result = finish(reason);
                    control->set_status(result ? api::OperationStatus::completed : api::OperationStatus::failed);
                    promise->set_value(std::move(result));
                } catch (...) {
                    control->set_status(api::OperationStatus::failed);
                    promise->set_exception(std::current_exception());
                }
            },
            [promise, control](core::Error error) mutable {
                control->set_status(api::OperationStatus::failed);
                promise->set_value(std::unexpected(std::move(error)));
            }};
    }

    /// Enqueues a command that resolves only after a translated stop event.
    template <class Value, class Begin, class Finish>
    [[nodiscard]] api::Task<core::Result<Value>> enqueue_wait(Begin begin, Finish finish,
                                                              api::OperationContext context) {
        auto control = context.operation ? context.operation : std::make_shared<api::OperationControl>();
        auto promise = std::make_shared<std::promise<core::Result<Value>>>();
        auto future = promise->get_future().share();
        Command command = [this, begin = std::move(begin), finish = std::move(finish), promise, control]() mutable {
            if (control->cancellation().stop_requested()) {
                control->set_status(api::OperationStatus::cancelled);
                promise->set_value(std::unexpected(core::Error::make(core::DiagnosticCode::cancelled,
                                                                     "Debugger operation was cancelled before start")));
                return;
            }
            if (pending_) {
                control->set_status(api::OperationStatus::rejected);
                promise->set_value(std::unexpected(core::Error::make(
                    core::DiagnosticCode::conflict, "Another debugger execution operation is already waiting")));
                return;
            }
            control->set_status(api::OperationStatus::running);
            auto started = begin();
            if (!started) {
                control->set_status(api::OperationStatus::failed);
                promise->set_value(std::unexpected(started.error()));
                return;
            }
            pending_ = make_pending_wait<Value>(promise, control, std::move(finish));
            arm_deadline();
        };
        enqueue(std::move(command), promise);
        return api::Task<core::Result<Value>>{std::move(future), std::move(control)};
    }

    /// Enqueues pause as an interrupt that can target an already-running wait.
    [[nodiscard]] api::Task<core::Result<model::StopReason>> enqueue_pause(api::OperationContext context) {
        auto control = context.operation ? context.operation : std::make_shared<api::OperationControl>();
        auto promise = std::make_shared<std::promise<core::Result<model::StopReason>>>();
        auto future = promise->get_future().share();
        Command command = [this, promise, control]() mutable {
            if (control->cancellation().stop_requested()) {
                control->set_status(api::OperationStatus::cancelled);
                promise->set_value(std::unexpected(
                    core::Error::make(core::DiagnosticCode::cancelled, "Debugger pause was cancelled before start")));
                return;
            }
            if (pending_) {
                if (state() != model::SessionState::running || !waiting_for_event_) {
                    control->set_status(api::OperationStatus::rejected);
                    promise->set_value(std::unexpected(core::Error::make(
                        core::DiagnosticCode::conflict, "Debugger pause conflicts with another pending operation")));
                    return;
                }
#ifdef _WIN32
                // Microsoft documents SetInterrupt as the cross-thread
                // interrupt used to return a blocked WaitForEvent. All other
                // DbgEng calls remain owned by the engine thread.
                pause_requested_ = true;
                request_interrupt();
#else
                pause_requested_ = true;
#endif
                control->set_status(api::OperationStatus::running);
                pause_pending_ = make_pending_wait<model::StopReason>(
                    promise, control,
                    [](const model::StopReason& reason) -> core::Result<model::StopReason> { return reason; });
                arm_deadline();
                return;
            }
            if (state() == model::SessionState::stopped) {
                control->set_status(api::OperationStatus::completed);
                promise->set_value(core::Result<model::StopReason>{model::StopReason{
                    model::StopReasonKind::pause, {}, {}, current_thread_id_, {}, "Target was already stopped"}});
                return;
            }
            control->set_status(api::OperationStatus::running);
            auto started = begin_execution(detail::execution_break, model::StopReasonKind::pause);
            if (!started) {
                control->set_status(api::OperationStatus::failed);
                promise->set_value(std::unexpected(started.error()));
                return;
            }
            pending_ = make_pending_wait<model::StopReason>(
                promise, control,
                [](const model::StopReason& reason) -> core::Result<model::StopReason> { return reason; });
            arm_deadline();
        };
        enqueue(std::move(command), promise);
        return api::Task<core::Result<model::StopReason>>{std::move(future), std::move(control)};
    }

    /// Enqueues an immediate command and waits synchronously for its result.
    template <class Value, class Callable> [[nodiscard]] core::Result<Value> invoke_sync(Callable callable) const {
        auto* self = const_cast<WinDbgEngSession*>(this);
        api::OperationContext context;
        auto task = self->enqueue_immediate<Value>(std::move(callable), std::move(context));
        return task.get();
    }

    /// Wraps an immediate command in the repository Task result type.
    template <class Value, class Callable>
    [[nodiscard]] api::Task<core::Result<Value>> enqueue_immediate(Callable callable, api::OperationContext context) {
        auto control = context.operation ? context.operation : std::make_shared<api::OperationControl>();
        auto promise = std::make_shared<std::promise<core::Result<Value>>>();
        auto future = promise->get_future().share();
        Command command = [callable = std::move(callable), promise, control]() mutable {
            if (control->cancellation().stop_requested()) {
                control->set_status(api::OperationStatus::cancelled);
                promise->set_value(std::unexpected(
                    core::Error::make(core::DiagnosticCode::cancelled, "Debugger query was cancelled before start")));
                return;
            }
            control->set_status(api::OperationStatus::running);
            try {
                auto result = callable();
                control->set_status(result ? api::OperationStatus::completed : api::OperationStatus::failed);
                promise->set_value(std::move(result));
            } catch (...) {
                control->set_status(api::OperationStatus::failed);
                promise->set_exception(std::current_exception());
            }
        };
        enqueue(std::move(command), promise);
        return api::Task<core::Result<Value>>{std::move(future), std::move(control)};
    }

    /// Places a command on the engine queue or resolves its promise with a queue error.
    template <class ResultType>
    void enqueue(Command command, const std::shared_ptr<std::promise<ResultType>>& promise) {
        {
            std::scoped_lock lock(queue_mutex_);
            if (stopping_.load(std::memory_order_acquire)) {
                promise->set_value(std::unexpected(
                    core::Error::make(core::DiagnosticCode::project_closed, "Debugger session is shutting down")));
                return;
            }
            commands_.push_back(
                QueuedCommand{std::move(command), [promise] {
                                  promise->set_value(std::unexpected(core::Error::make(
                                      core::DiagnosticCode::project_closed, "Debugger session is shutting down")));
                              }});
        }
        queue_condition_.notify_one();
    }

    /// Resolves active waiters with cancellation before native interfaces are released.
    void cancel_pending_waits() noexcept {
        clear_deadline();
        const model::StopReason shutdown_reason{model::StopReasonKind::unknown,     {}, {}, {}, {},
                                                "Debugger session is shutting down"};
        if (pending_) {
            pending_->control->request_cancel();
            auto pending = std::move(*pending_);
            pending_.reset();
            pending.complete(shutdown_reason);
        }
        if (pause_pending_) {
            pause_pending_->control->request_cancel();
            auto pause = std::move(*pause_pending_);
            pause_pending_.reset();
            pause.complete(shutdown_reason);
        }
    }

    /// Rejects commands that were queued but never reached the engine thread.
    void reject_queued_commands() noexcept {
        std::deque<QueuedCommand> queued;
        {
            std::scoped_lock lock(queue_mutex_);
            queued.swap(commands_);
        }
        for (auto& command : queued)
            command.reject();
    }

    /// Arms the watchdog that prevents a DbgEng wait from hanging forever.
    void arm_deadline() noexcept {
#ifdef _WIN32
        if (options_.operation_timeout.count() <= 0)
            return;
        std::scoped_lock lock(interrupt_mutex_);
        if (!deadline_)
            deadline_ = std::chrono::steady_clock::now() + options_.operation_timeout;
        interrupt_condition_.notify_one();
#endif
    }

    /// Clears the active execution watchdog after a stop or terminal operation.
    void clear_deadline() noexcept {
#ifdef _WIN32
        std::scoped_lock lock(interrupt_mutex_);
        deadline_.reset();
        timeout_requested_ = false;
        interrupt_condition_.notify_one();
#endif
    }

    /// Fails active waiters with an explicit timeout diagnostic.
    void fail_pending_timeout() noexcept {
        clear_deadline();
        const auto error = core::Error::make(
            core::DiagnosticCode::timeout, "Debugger execution operation exceeded its configured timeout",
            "Inspect the target state and retry with a larger operation timeout if appropriate.");
        if (pending_) {
            auto pending = std::move(*pending_);
            pending_.reset();
            pending.fail(error);
        }
        if (pause_pending_) {
            auto pause = std::move(*pause_pending_);
            pause_pending_.reset();
            pause.fail(error);
        }
        pause_requested_ = false;
    }

    /// Runs the engine thread, initializes native interfaces, and owns every DbgEng call.
    void engine_loop(std::stop_token stop, std::promise<core::Result<void>> ready) {
        try {
#ifdef _WIN32
            const auto initialized = initialize_native();
#else
            const auto initialized = core::Result<void>{std::unexpected(detail::unsupported_platform())};
#endif
            if (!initialized) {
#ifdef _WIN32
                shutdown_native();
#endif
                ready.set_value(initialized);
                return;
            }
            ready.set_value(initialized);
            while (!stop.stop_requested() && !stopping_.load(std::memory_order_acquire)) {
                QueuedCommand queued_command;
                bool has_command = false;
                {
                    std::unique_lock lock(queue_mutex_);
                    if (commands_.empty() && !waiting_for_event_)
                        queue_condition_.wait_for(lock, std::chrono::milliseconds(50), [&] {
                            return stop.stop_requested() || stopping_.load(std::memory_order_acquire) ||
                                   !commands_.empty() || waiting_for_event_;
                        });
                    if (!commands_.empty()) {
                        queued_command = std::move(commands_.front());
                        commands_.pop_front();
                        has_command = true;
                    }
                }
                if (has_command && queued_command.execute)
                    queued_command.execute();
#ifdef _WIN32
                if (waiting_for_event_) {
                    bool commands_pending;
                    {
                        std::scoped_lock lock(queue_mutex_);
                        commands_pending = !commands_.empty();
                    }
                    if (!commands_pending)
                        pump_event();
                }
#endif
            }
            cancel_pending_waits();
            reject_queued_commands();
#ifdef _WIN32
            shutdown_native();
#endif
        } catch (const std::exception& error) {
#ifdef _WIN32
            shutdown_native();
#endif
            cancel_pending_waits();
            reject_queued_commands();
            try {
                ready.set_value(std::unexpected(detail::native_error("engine initialization", error.what())));
            } catch (...) {
            }
            state_.store(model::SessionState::failed, std::memory_order_release);
        } catch (...) {
#ifdef _WIN32
            shutdown_native();
#endif
            cancel_pending_waits();
            reject_queued_commands();
            try {
                ready.set_value(std::unexpected(detail::native_error("engine initialization", "unknown exception")));
            } catch (...) {
            }
            state_.store(model::SessionState::failed, std::memory_order_release);
        }
    }

    /// Starts a process on Windows or returns an explicit unsupported result elsewhere.
    [[nodiscard]] core::Result<void> begin_launch(const model::LaunchRequest& request) {
#ifndef _WIN32
        static_cast<void>(request);
        return std::unexpected(detail::unsupported_platform());
#else
        if (state() != model::SessionState::created && state() != model::SessionState::detached)
            return std::unexpected(detail::invalid_state(state(), "launch"));
        if (request.program.empty())
            return std::unexpected(
                core::Error::make(core::DiagnosticCode::invalid_argument, "Debugger launch requires a program path"));
        if (!request.working_directory.empty() || !request.environment.empty() || !request.inherit_environment)
            return std::unexpected(core::Error::make(
                core::DiagnosticCode::unsupported,
                "WinDbgEng CreateProcess does not expose portable working-directory or environment requests",
                "Launch with inherited environment/default working directory or use a backend that supports those "
                "controls."));
        state_.store(model::SessionState::launching, std::memory_order_release);
        std::string command_line = quote_command_line(request.program.string());
        for (const auto& argument : request.arguments)
            command_line += " " + quote_command_line(argument);
        std::vector<char> mutable_command(command_line.begin(), command_line.end());
        mutable_command.push_back('\0');
        const HRESULT result = client_.get()->CreateProcess(0, mutable_command.data(), DEBUG_PROCESS);
        if (FAILED(result)) {
            state_.store(model::SessionState::failed, std::memory_order_release);
            return std::unexpected(detail::hresult_error("IDebugClient::CreateProcess", result));
        }
        waiting_for_event_ = true;
        return {};
#endif
    }

    /// Attaches to a process and relies on WaitForEvent for the initial attach event.
    [[nodiscard]] core::Result<void> begin_attach(const model::AttachRequest& request) {
#ifndef _WIN32
        static_cast<void>(request);
        return std::unexpected(detail::unsupported_platform());
#else
        if (state() != model::SessionState::created && state() != model::SessionState::detached)
            return std::unexpected(detail::invalid_state(state(), "attach"));
        if (request.process.value.empty())
            return std::unexpected(core::Error::make(core::DiagnosticCode::invalid_argument,
                                                     "Debugger attach requires a non-zero process identity"));
        ULONG native_process_id{};
        const auto [end, error] =
            std::from_chars(request.process.value.data(), request.process.value.data() + request.process.value.size(),
                            native_process_id);
        if (error != std::errc{} || end != request.process.value.data() + request.process.value.size())
            return std::unexpected(core::Error::make(
                core::DiagnosticCode::unsupported, "WinDbgEng attach requires a numeric local process identity",
                "Use a remote-capable debugger backend for opaque target locators."));
        state_.store(model::SessionState::attaching, std::memory_order_release);
        const HRESULT result = client_.get()->AttachProcess(0, native_process_id, DEBUG_ATTACH_DEFAULT);
        if (FAILED(result)) {
            state_.store(model::SessionState::failed, std::memory_order_release);
            return std::unexpected(detail::hresult_error("IDebugClient::AttachProcess", result));
        }
        waiting_for_event_ = true;
        return {};
#endif
    }

    /// Sets DbgEng execution status; actual execution begins only in pump_event's WaitForEvent call.
    [[nodiscard]] core::Result<void> begin_execution(std::uint32_t status, model::StopReasonKind expected) {
#ifndef _WIN32
        static_cast<void>(status);
        static_cast<void>(expected);
        return std::unexpected(detail::unsupported_platform());
#else
        if (state() != model::SessionState::stopped)
            return std::unexpected(detail::invalid_state(state(), "execute"));
        expected_stop_kind_ = expected;
        const HRESULT result = control_.get()->SetExecutionStatus(status);
        if (FAILED(result))
            return std::unexpected(detail::hresult_error("IDebugControl::SetExecutionStatus", result));
        state_.store(model::SessionState::running, std::memory_order_release);
        waiting_for_event_ = true;
        return {};
#endif
    }

    /// Creates a temporary breakpoint at the caller's return address for generic step-out behavior.
    [[nodiscard]] core::Result<void> begin_step_out() {
#ifndef _WIN32
        return std::unexpected(detail::unsupported_platform());
#else
        if (state() != model::SessionState::stopped)
            return std::unexpected(detail::invalid_state(state(), "step out"));
        const auto frames = stack_trace_native(std::nullopt, 2);
        if (!frames || frames->size() < 2)
            return std::unexpected(frames ? core::Error::make(core::DiagnosticCode::unsupported,
                                                              "The current stack has no caller frame for step out")
                                          : frames.error());
        auto breakpoint = add_breakpoint_native(frames->at(1).instruction, model::BreakpointKind::temporary, true);
        if (!breakpoint)
            return std::unexpected(breakpoint.error());
        temporary_step_out_breakpoint_ = breakpoint->id;
        return begin_execution(DEBUG_STATUS_GO, model::StopReasonKind::step_complete);
#endif
    }

    /// Detaches through IDebugClient and updates the cached lifecycle state.
    [[nodiscard]] core::Result<void> detach_native() {
#ifndef _WIN32
        return std::unexpected(detail::unsupported_platform());
#else
        if (state() == model::SessionState::created || state() == model::SessionState::detached)
            return std::unexpected(detail::invalid_state(state(), "detach"));
        const HRESULT result = client_.get()->DetachProcesses();
        if (FAILED(result))
            return std::unexpected(detail::hresult_error("IDebugClient::DetachProcesses", result));
        waiting_for_event_ = false;
        state_.store(model::SessionState::detached, std::memory_order_release);
        return {};
#endif
    }

    /// Terminates all target processes through DbgEng's client-level operation.
    [[nodiscard]] core::Result<void> terminate_native() {
#ifndef _WIN32
        return std::unexpected(detail::unsupported_platform());
#else
        if (state() == model::SessionState::created || state() == model::SessionState::detached ||
            state() == model::SessionState::exited)
            return std::unexpected(detail::invalid_state(state(), "terminate"));
        state_.store(model::SessionState::exiting, std::memory_order_release);
        const HRESULT result = client_.get()->TerminateProcesses();
        if (FAILED(result))
            return std::unexpected(detail::hresult_error("IDebugClient::TerminateProcesses", result));
        waiting_for_event_ = false;
        state_.store(model::SessionState::exited, std::memory_order_release);
        return {};
#endif
    }

#ifdef _WIN32
    /// Initializes DbgEng interfaces and registers callbacks on this engine thread only.
    [[nodiscard]] core::Result<void> initialize_native() {
        const HRESULT com_result = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        com_initialized_ = SUCCEEDED(com_result);
        HRESULT result = DebugCreate(__uuidof(IDebugClient), reinterpret_cast<void**>(client_.put()));
        if (FAILED(result))
            return std::unexpected(detail::hresult_error("DebugCreate", result));
        result = client_.get()->QueryInterface(__uuidof(IDebugControl2), reinterpret_cast<void**>(control_.put()));
        if (FAILED(result))
            return std::unexpected(detail::hresult_error("IDebugClient::QueryInterface(IDebugControl2)", result));
        result = client_.get()->QueryInterface(__uuidof(IDebugRegisters), reinterpret_cast<void**>(registers_.put()));
        if (FAILED(result))
            return std::unexpected(detail::hresult_error("IDebugClient::QueryInterface(IDebugRegisters)", result));
        result =
            client_.get()->QueryInterface(__uuidof(IDebugDataSpaces2), reinterpret_cast<void**>(data_spaces_.put()));
        if (FAILED(result))
            return std::unexpected(detail::hresult_error("IDebugClient::QueryInterface(IDebugDataSpaces2)", result));
        result = client_.get()->QueryInterface(__uuidof(IDebugSymbols), reinterpret_cast<void**>(symbols_.put()));
        if (FAILED(result))
            return std::unexpected(detail::hresult_error("IDebugClient::QueryInterface(IDebugSymbols)", result));
        result = client_.get()->QueryInterface(__uuidof(IDebugSystemObjects),
                                               reinterpret_cast<void**>(system_objects_.put()));
        if (FAILED(result))
            return std::unexpected(detail::hresult_error("IDebugClient::QueryInterface(IDebugSystemObjects)", result));
        result = control_.get()->AddEngineOptions(DEBUG_ENGOPT_INITIAL_BREAK);
        if (FAILED(result))
            return std::unexpected(detail::hresult_error("IDebugControl::AddEngineOptions", result));
        event_callbacks_ = std::make_unique<EventCallbacks>(this);
        output_callbacks_ = std::make_unique<OutputCallbacks>();
        result = client_.get()->SetEventCallbacks(event_callbacks_.get());
        if (FAILED(result))
            return std::unexpected(detail::hresult_error("IDebugClient::SetEventCallbacks", result));
        result = client_.get()->SetOutputCallbacks(output_callbacks_.get());
        if (FAILED(result))
            return std::unexpected(detail::hresult_error("IDebugClient::SetOutputCallbacks", result));
        state_.store(model::SessionState::created, std::memory_order_release);
        return {};
    }

    /// Signals the documented cross-thread DbgEng interrupt path.
    void request_interrupt() noexcept {
        {
            std::scoped_lock lock(interrupt_mutex_);
            interrupt_requested_ = true;
        }
        interrupt_condition_.notify_one();
    }

    /// Waits for an interrupt request and calls only the DbgEng method documented as cross-thread safe.
    void interrupt_loop(std::stop_token stop) noexcept {
        std::unique_lock lock(interrupt_mutex_);
        while (!stop.stop_requested()) {
            bool timed_out = false;
            if (deadline_) {
                if (std::chrono::steady_clock::now() >= *deadline_) {
                    deadline_.reset();
                    timeout_requested_.store(true, std::memory_order_release);
                    timed_out = true;
                } else {
                    interrupt_condition_.wait_until(lock, *deadline_, [&] {
                        return stop.stop_requested() || interrupt_requested_ ||
                               stopping_.load(std::memory_order_acquire);
                    });
                }
            } else {
                interrupt_condition_.wait(lock, [&] {
                    return stop.stop_requested() || interrupt_requested_ || stopping_.load(std::memory_order_acquire) ||
                           deadline_.has_value();
                });
            }
            if (stop.stop_requested())
                break;
            if (stopping_.load(std::memory_order_acquire) && !interrupt_requested_ && !timed_out)
                break;
            const bool should_interrupt = interrupt_requested_ || timed_out;
            interrupt_requested_ = false;
            if (!should_interrupt)
                continue;
            lock.unlock();
            if (control_) {
                const HRESULT result = control_.get()->SetInterrupt(DEBUG_INTERRUPT_ACTIVE);
                static_cast<void>(result);
            }
            lock.lock();
        }
    }

    /// Unregisters callbacks before releasing native interfaces and COM state.
    void shutdown_native() noexcept {
        if (client_) {
            const auto current = state();
            if (current != model::SessionState::created && current != model::SessionState::detached &&
                current != model::SessionState::exited && current != model::SessionState::failed)
                static_cast<void>(client_.get()->TerminateProcesses());
            static_cast<void>(client_.get()->SetEventCallbacks(nullptr));
            static_cast<void>(client_.get()->SetOutputCallbacks(nullptr));
        }
        system_objects_.reset();
        symbols_.reset();
        data_spaces_.reset();
        registers_.reset();
        control_.reset();
        // SetEventCallbacks can fail while DbgEng is unwinding a target. Keep
        // callback objects alive until the client itself releases any retained
        // callback reference, then destroy the member owners.
        client_.reset();
        event_callbacks_.reset();
        output_callbacks_.reset();
        if (com_initialized_)
            CoUninitialize();
        com_initialized_ = false;
    }

    /// Waits for one stop while the interrupt/watchdog thread bounds the otherwise blocking DbgEng call.
    void pump_event() {
        if (pending_ && pending_->control->cancellation().stop_requested())
            request_interrupt();
        const HRESULT result = control_.get()->WaitForEvent(DEBUG_WAIT_DEFAULT, INFINITE);
        if (result == S_FALSE)
            return;
        if (FAILED(result)) {
            state_.store(model::SessionState::failed, std::memory_order_release);
            if (pending_) {
                auto pending = std::move(*pending_);
                pending_.reset();
                pending.complete(
                    model::StopReason{model::StopReasonKind::unknown, {}, {}, {}, {}, "DbgEng WaitForEvent failed"});
            }
            if (pause_pending_) {
                auto pause = std::move(*pause_pending_);
                pause_pending_.reset();
                pause.complete(
                    model::StopReason{model::StopReasonKind::unknown, {}, {}, {}, {}, "DbgEng WaitForEvent failed"});
            }
            pause_requested_ = false;
            waiting_for_event_ = false;
            clear_deadline();
            return;
        }
        waiting_for_event_ = false;
#ifdef _WIN32
        if (timeout_requested_.exchange(false, std::memory_order_acq_rel)) {
            state_.store(model::SessionState::stopped, std::memory_order_release);
            fail_pending_timeout();
            return;
        }
#endif
        if (pause_requested_hint_.exchange(false, std::memory_order_acq_rel))
            pause_requested_ = true;
        refresh_context_ids();
        if (process_created_pending_) {
            auto process = query_current_process();
            if (process)
                emit_event(
                    model::DebugEvent{next_event_++, model::EventKind::process_created, model::ProcessCreatedEvent {
                                          *process
                                      }});
        }
        if (thread_event_pending_)
            emit_event(model::DebugEvent{next_event_++, model::EventKind::thread_created, model::ThreadCreatedEvent {
                                             current_process_id_,
                                             model::Thread {
                                                 current_thread_id_,
                                                 {},
                                                 model::ThreadState::stopped,
                                                 true,
                                                 std::nullopt
                                             }
                                         }});
        if (process_exited_) {
            state_.store(model::SessionState::exited, std::memory_order_release);
            expected_stop_kind_ = model::StopReasonKind::process_exit;
        } else {
            state_.store(model::SessionState::stopped, std::memory_order_release);
        }
        if (expected_stop_kind_ != model::StopReasonKind::unknown &&
            last_stop_reason_.kind == model::StopReasonKind::unknown)
            last_stop_reason_.kind = expected_stop_kind_;
        if (last_stop_reason_.kind == model::StopReasonKind::unknown)
            last_stop_reason_.kind =
                process_created_pending_ ? model::StopReasonKind::initial_stop : model::StopReasonKind::unknown;
        if (pause_requested_ && last_stop_reason_.kind == model::StopReasonKind::unknown)
            last_stop_reason_.kind = model::StopReasonKind::pause;
        emit_execution_stopped(last_stop_reason_);
        for (const auto id : temporary_breakpoints_to_remove_)
            static_cast<void>(remove_breakpoint_native(id));
        temporary_breakpoints_to_remove_.clear();
        temporary_step_out_breakpoint_.reset();
        process_created_pending_ = false;
        thread_event_pending_ = false;
        process_exited_ = false;
        if (pending_) {
            auto pending = std::move(*pending_);
            pending_.reset();
            pending.complete(last_stop_reason_);
        }
        if (pause_pending_) {
            auto pause = std::move(*pause_pending_);
            pause_pending_.reset();
            pause.complete(last_stop_reason_);
        }
        clear_deadline();
        pause_requested_ = false;
        last_stop_reason_ = model::StopReason{};
        expected_stop_kind_ = model::StopReasonKind::unknown;
        dispatch_sink_events();
    }

    /// Updates portable process/thread identities after a DbgEng wait returns.
    void refresh_context_ids() {
        ULONG process{};
        ULONG thread{};
        if (system_objects_.get()->GetCurrentProcessSystemId(&process) == S_OK)
            current_process_id_ = model::ProcessId{std::to_string(process)};
        if (system_objects_.get()->GetCurrentThreadSystemId(&thread) == S_OK)
            current_thread_id_ = model::ThreadId{std::to_string(thread)};
    }

    /// Captures a callback breakpoint without calling application code or waiting from the callback.
    void on_breakpoint(ULONG native_id) {
        const auto code = native_to_breakpoint_.find(native_id);
        if (code != native_to_breakpoint_.end()) {
            auto& value = breakpoints_.at(code->second);
            ++value.hit_count;
            const bool step_out_return =
                temporary_step_out_breakpoint_.has_value() && temporary_step_out_breakpoint_.value() == value.id;
            last_stop_reason_ = model::StopReason{
                step_out_return ? model::StopReasonKind::step_complete : model::StopReasonKind::breakpoint,
                step_out_return ? std::optional<model::BreakpointId>{} : std::optional<model::BreakpointId>{value.id},
                {},
                current_thread_id_,
                {},
                step_out_return ? "Step-out return address reached" : "Code breakpoint hit"};
            emit_event(model::DebugEvent{next_event_++, model::EventKind::breakpoint_hit, model::BreakpointHitEvent {
                                             value,
                                             current_thread_id_
                                         }});
            if (value.one_shot)
                temporary_breakpoints_to_remove_.push_back(value.id);
            return;
        }
        const auto watch = native_to_watchpoint_.find(native_id);
        if (watch != native_to_watchpoint_.end()) {
            auto& value = watchpoints_.at(watch->second);
            ++value.hit_count;
            last_stop_reason_ = model::StopReason{
                model::StopReasonKind::watchpoint, {}, value.id, current_thread_id_, {}, "Data breakpoint hit"};
            emit_event(model::DebugEvent{next_event_++, model::EventKind::watchpoint_hit, model::WatchpointHitEvent {
                                             value,
                                             current_thread_id_
                                         }});
        }
    }

    /// Captures an exception record into a portable value before DbgEng may invalidate it.
    void on_exception(const EXCEPTION_RECORD64& record, ULONG first_chance) {
        model::DebugException exception{record.ExceptionCode, core::Address{process_space_, record.ExceptionAddress},
                                        first_chance != 0, "DbgEng exception event"};
        if ((first_chance == 0 || options_.stop_on_first_chance_exceptions) &&
            !(pause_requested_ || pause_requested_hint_.load(std::memory_order_acquire)) &&
            record.ExceptionCode == EXCEPTION_BREAKPOINT)
            last_stop_reason_ = model::StopReason{
                model::StopReasonKind::exception, {}, {}, current_thread_id_, exception, "Target breakpoint exception"};
        else if ((first_chance == 0 || options_.stop_on_first_chance_exceptions) &&
                 !(pause_requested_ || pause_requested_hint_.load(std::memory_order_acquire)))
            last_stop_reason_ = model::StopReason{
                model::StopReasonKind::exception, {}, {}, current_thread_id_, exception, "Target exception"};
        emit_event(model::DebugEvent{next_event_++, model::EventKind::exception, model::ExceptionEvent {
                                         current_process_id_,
                                         current_thread_id_,
                                         exception
                                     }});
    }

    /// Captures a process callback and defers snapshot queries until WaitForEvent has returned.
    void on_process_created() {
        process_created_pending_ = true;
    }

    /// Captures process termination without re-entering DbgEng from the callback.
    void on_process_exited(ULONG code) {
        process_exited_ = true;
        last_stop_reason_ = model::StopReason{
            model::StopReasonKind::process_exit, {}, {}, current_thread_id_, {}, "Target process exited"};
        emit_event(model::DebugEvent{next_event_++, model::EventKind::process_exited, model::ProcessExitedEvent {
                                         current_process_id_,
                                         static_cast<std::int64_t>(code)
                                     }});
    }

    /// Captures a thread callback for later enumeration refresh.
    void on_thread_created() {
        thread_event_pending_ = true;
        if (options_.stop_on_thread_events)
            last_stop_reason_ = model::StopReason{
                model::StopReasonKind::thread_event, {}, {}, current_thread_id_, {}, "Thread created"};
    }

    /// Captures a thread exit callback using the last selected thread identity.
    void on_thread_exited(ULONG code) {
        emit_event(model::DebugEvent{next_event_++, model::EventKind::thread_exited, model::ThreadExitedEvent {
                                         current_process_id_,
                                         current_thread_id_,
                                         static_cast<std::int64_t>(code)
                                     }});
    }

    /// Captures a loaded module from callback parameters without performing symbol work in the callback.
    void on_module_loaded(ULONG64 base, ULONG size, PCSTR name, PCSTR image) {
        const std::string module_name = name != nullptr ? name : "";
        const std::string module_path = image != nullptr ? image : module_name;
        emit_event(model::DebugEvent{next_event_++, model::EventKind::module_loaded, model::ModuleLoadedEvent {
                                         current_process_id_,
                                         model::Module {
                                             module_name,
                                             module_path,
                                             core::Address{process_space_, base},
                                             size,
                                             false
                                         }
                                     }});
        if (options_.stop_on_shared_library_events)
            last_stop_reason_ =
                model::StopReason{model::StopReasonKind::module_event, {}, {}, current_thread_id_, {}, "Module loaded"};
    }

    /// Captures a module unload callback as an immutable generic event.
    void on_module_unloaded(ULONG64 base, PCSTR name) {
        emit_event(
            model::DebugEvent{next_event_++, model::EventKind::module_unloaded,
                              model::ModuleUnloadedEvent{current_process_id_, core::Address{process_space_, base},
                                                         name != nullptr ? name : ""}});
    }

    /// Appends an event and defers sink delivery until after native callback return.
    void emit_event(model::DebugEvent event) {
        sink_events_.push_back(event);
        events_.push_back(std::move(event));
    }

    /// Delivers captured events after WaitForEvent has returned to the engine loop.
    void dispatch_sink_events() {
        while (!sink_events_.empty() && event_sink_) {
            auto event = std::move(sink_events_.front());
            sink_events_.pop_front();
            try {
                event_sink_(event);
            } catch (...) {
                // A user sink must never unwind into DbgEng or terminate the engine thread.
            }
        }
    }

    /// Emits the final stop event after WaitForEvent returns, preserving callback-before-stop ordering.
    void emit_execution_stopped(const model::StopReason& reason) {
        emit_event(
            model::DebugEvent{next_event_++, model::EventKind::execution_stopped, model::ExecutionStoppedEvent {
                                  reason,
                                  process_exited_ ? model::ExecutionState::exited : model::ExecutionState::stopped
                              }});
    }

    /// Quotes one command-line token using the Windows process creation grammar.
    [[nodiscard]] static std::string quote_command_line(const std::string& value) {
        if (value.find_first_of(" \t\"") == std::string::npos)
            return value;
        std::string result{"\""};
        for (const char character : value) {
            if (character == '"')
                result += '\\';
            result += character;
        }
        result += '"';
        return result;
    }

#endif

    /// Converts a native register value into endian-preserving portable bytes.
#ifdef _WIN32
    [[nodiscard]] static core::Bytes register_bytes(const DEBUG_VALUE& value) {
        const auto copy = [](const void* source, std::size_t size) {
            const auto* bytes = static_cast<const core::Byte*>(source);
            return core::Bytes{std::vector<core::Byte>(bytes, bytes + size)};
        };
        switch (value.Type) {
            case DEBUG_VALUE_INT8:
                return copy(&value.I8, sizeof(value.I8));
            case DEBUG_VALUE_INT16:
                return copy(&value.I16, sizeof(value.I16));
            case DEBUG_VALUE_INT32:
                return copy(&value.I32, sizeof(value.I32));
            case DEBUG_VALUE_INT64:
                return copy(&value.I64, sizeof(value.I64));
            case DEBUG_VALUE_FLOAT32:
                return copy(&value.F32, sizeof(value.F32));
            case DEBUG_VALUE_FLOAT64:
                return copy(&value.F64, sizeof(value.F64));
            case DEBUG_VALUE_FLOAT80:
                return copy(value.F80Bytes, sizeof(value.F80Bytes));
            case DEBUG_VALUE_FLOAT128:
                return copy(value.F128Bytes, sizeof(value.F128Bytes));
            case DEBUG_VALUE_VECTOR128:
                return copy(value.VI8, sizeof(value.VI8));
            default:
                return {};
        }
    }

    /// Maps a register name to the generic descriptor roles used by consumers.
    [[nodiscard]] static model::Register register_descriptor(std::string name, std::uint32_t bits) {
        const auto upper = [&] {
            std::string result = name;
            std::ranges::transform(result, result.begin(),
                                   [](unsigned char value) { return static_cast<char>(std::toupper(value)); });
            return result;
        }();
        model::Register descriptor{std::move(name),
                                   bits,
                                   upper.starts_with("R") || upper.starts_with("E"),
                                   upper == "RIP" || upper == "EIP",
                                   upper == "RSP" || upper == "ESP",
                                   upper == "RBP" || upper == "EBP"};
        descriptor.flags = upper == "RFLAGS" || upper == "EFLAGS" || upper == "EFL";
        descriptor.vector = upper.starts_with("XMM") || upper.starts_with("YMM") || upper.starts_with("ZMM");
        return descriptor;
    }

    /// Resolves a native breakpoint object by its DbgEng identifier.
    [[nodiscard]] core::Result<detail::NativePtr<IDebugBreakpoint>> breakpoint_object(ULONG native_id) {
        detail::NativePtr<IDebugBreakpoint> result;
        const HRESULT status = control_.get()->GetBreakpointById(native_id, result.put());
        if (FAILED(status))
            return std::unexpected(detail::hresult_error("IDebugControl::GetBreakpointById", status));
        return result;
    }

    /// Refreshes one portable process snapshot from the current DbgEng target.
    [[nodiscard]] core::Result<model::Process> current_process_native() const {
        if (!system_objects_)
            return std::unexpected(detail::unsupported_platform());
        return const_cast<WinDbgEngSession*>(this)->query_current_process();
    }

    /// Queries the current process on the engine thread.
    [[nodiscard]] core::Result<model::Process> query_current_process() {
        ULONG system_id{};
        if (const HRESULT status = system_objects_.get()->GetCurrentProcessSystemId(&system_id); FAILED(status))
            return std::unexpected(detail::hresult_error("IDebugSystemObjects::GetCurrentProcessSystemId", status));
        char image[1024]{};
        ULONG size{};
        const HRESULT image_status =
            system_objects_.get()->GetCurrentProcessExecutableName(image, sizeof(image), &size);
        if (FAILED(image_status) && image_status != S_FALSE)
            return std::unexpected(
                detail::hresult_error("IDebugSystemObjects::GetCurrentProcessExecutableName", image_status));
        ULONG execution_status{DEBUG_STATUS_NO_DEBUGGEE};
        static_cast<void>(control_.get()->GetExecutionStatus(&execution_status));
        const auto process_state = execution_status == DEBUG_STATUS_NO_DEBUGGEE ? model::ProcessState::exited
                                   : state() == model::SessionState::running    ? model::ProcessState::running
                                                                                : model::ProcessState::stopped;
        current_process_id_ = model::ProcessId{std::to_string(system_id)};
        return model::Process{current_process_id_, image, {}, process_state, std::nullopt, options_.target_description};
    }

    /// Enumerates process IDs and returns portable process snapshots.
    [[nodiscard]] core::Result<std::vector<model::Process>> processes_native() {
        ULONG count{};
        if (const HRESULT status = system_objects_.get()->GetNumberProcesses(&count); FAILED(status))
            return std::unexpected(detail::hresult_error("IDebugSystemObjects::GetNumberProcesses", status));
        std::vector<ULONG> engine_ids(count);
        std::vector<ULONG> system_ids(count);
        if (count != 0) {
            const HRESULT status =
                system_objects_.get()->GetProcessIdsByIndex(0, count, engine_ids.data(), system_ids.data());
            if (FAILED(status))
                return std::unexpected(detail::hresult_error("IDebugSystemObjects::GetProcessIdsByIndex", status));
        }
        std::vector<model::Process> result;
        result.reserve(count);
        for (const ULONG id : system_ids)
            result.push_back(model::Process{model::ProcessId{std::to_string(id)},
                                            {},
                                            {},
                                            model::ProcessState::unknown,
                                            std::nullopt,
                                            options_.target_description});
        if (!result.empty()) {
            auto current = query_current_process();
            if (current)
                for (auto& process : result)
                    if (process.id == current->id)
                        process = *current;
        }
        return result;
    }

    /// Converts an opaque local process identity to a DbgEng process index and selects it.
    [[nodiscard]] core::Result<void> select_process_native(model::ProcessId process) {
        ULONG native_process_id{};
        const auto [end, error] =
            std::from_chars(process.value.data(), process.value.data() + process.value.size(), native_process_id);
        if (error != std::errc{} || end != process.value.data() + process.value.size())
            return std::unexpected(
                core::Error::make(core::DiagnosticCode::unsupported,
                                  "WinDbgEng process selection requires a numeric local process identity",
                                  "Use a backend that supports opaque remote process locators."));
        ULONG count{};
        if (const HRESULT status = system_objects_.get()->GetNumberProcesses(&count); FAILED(status))
            return std::unexpected(detail::hresult_error("IDebugSystemObjects::GetNumberProcesses", status));
        std::vector<ULONG> engine_ids(count);
        std::vector<ULONG> system_ids(count);
        if (count != 0) {
            const HRESULT status =
                system_objects_.get()->GetProcessIdsByIndex(0, count, engine_ids.data(), system_ids.data());
            if (FAILED(status))
                return std::unexpected(detail::hresult_error("IDebugSystemObjects::GetProcessIdsByIndex", status));
        }
        const auto match = std::ranges::find(system_ids, native_process_id);
        if (match == system_ids.end())
            return std::unexpected(
                core::Error::make(core::DiagnosticCode::invalid_argument, "Requested debugger process is not present"));
        const auto index = static_cast<std::size_t>(std::distance(system_ids.begin(), match));
        const HRESULT status = system_objects_.get()->SetCurrentProcessId(engine_ids[index]);
        if (FAILED(status))
            return std::unexpected(detail::hresult_error("IDebugSystemObjects::SetCurrentProcessId", status));
        current_process_id_ = process;
        return {};
    }

    /// Reads the selected process's opaque identity from DbgEng system objects.
    [[nodiscard]] core::Result<model::ProcessId> current_process_id_native() {
        ULONG native_process_id{};
        if (const HRESULT status = system_objects_.get()->GetCurrentProcessSystemId(&native_process_id); FAILED(status))
            return std::unexpected(detail::hresult_error("IDebugSystemObjects::GetCurrentProcessSystemId", status));
        current_process_id_ = model::ProcessId{std::to_string(native_process_id)};
        return current_process_id_;
    }

    /// Enumerates system thread IDs and identifies the selected thread without changing other contexts.
    [[nodiscard]] core::Result<std::vector<model::Thread>> threads_native() {
        ULONG count{};
        if (const HRESULT status = system_objects_.get()->GetNumberThreads(&count); FAILED(status))
            return std::unexpected(detail::hresult_error("IDebugSystemObjects::GetNumberThreads", status));
        std::vector<ULONG> engine_ids(count);
        std::vector<ULONG> system_ids(count);
        if (count != 0) {
            const HRESULT status =
                system_objects_.get()->GetThreadIdsByIndex(0, count, engine_ids.data(), system_ids.data());
            if (FAILED(status))
                return std::unexpected(detail::hresult_error("IDebugSystemObjects::GetThreadIdsByIndex", status));
        }
        std::vector<model::Thread> result;
        result.reserve(count);
        for (const ULONG id : system_ids)
            result.push_back(model::Thread{model::ThreadId{std::to_string(id)},
                                           {},
                                           model::ThreadState::unknown,
                                           current_thread_id_.value == std::to_string(id),
                                           std::nullopt});
        return result;
    }

    /// Converts a system thread identity to an engine thread index and selects it.
    [[nodiscard]] core::Result<void> select_thread_native(model::ThreadId thread) {
        ULONG count{};
        if (const HRESULT status = system_objects_.get()->GetNumberThreads(&count); FAILED(status))
            return std::unexpected(detail::hresult_error("IDebugSystemObjects::GetNumberThreads", status));
        std::vector<ULONG> engine_ids(count);
        std::vector<ULONG> system_ids(count);
        if (count != 0) {
            const HRESULT status =
                system_objects_.get()->GetThreadIdsByIndex(0, count, engine_ids.data(), system_ids.data());
            if (FAILED(status))
                return std::unexpected(detail::hresult_error("IDebugSystemObjects::GetThreadIdsByIndex", status));
        }
        ULONG native_thread_id{};
        const auto [end, error] =
            std::from_chars(thread.value.data(), thread.value.data() + thread.value.size(), native_thread_id);
        if (error != std::errc{} || end != thread.value.data() + thread.value.size())
            return std::unexpected(
                core::Error::make(core::DiagnosticCode::unsupported,
                                  "WinDbgEng thread selection requires a numeric local thread identity"));
        const auto match = std::ranges::find(system_ids, native_thread_id);
        if (match == system_ids.end())
            return std::unexpected(
                core::Error::make(core::DiagnosticCode::invalid_argument, "Requested debugger thread is not present"));
        const auto index = static_cast<std::size_t>(std::distance(system_ids.begin(), match));
        const HRESULT status = system_objects_.get()->SetCurrentThreadId(engine_ids[index]);
        if (FAILED(status))
            return std::unexpected(detail::hresult_error("IDebugSystemObjects::SetCurrentThreadId", status));
        current_thread_id_ = thread;
        return {};
    }

    /// Reads the currently selected system thread identity.
    [[nodiscard]] core::Result<model::ThreadId> current_thread_native() {
        ULONG id{};
        if (const HRESULT status = system_objects_.get()->GetCurrentThreadSystemId(&id); FAILED(status))
            return std::unexpected(detail::hresult_error("IDebugSystemObjects::GetCurrentThreadSystemId", status));
        current_thread_id_ = model::ThreadId{std::to_string(id)};
        return current_thread_id_;
    }

    /// Enumerates register descriptions and derives portable role flags from names.
    [[nodiscard]] core::Result<std::vector<model::Register>> registers_native(std::optional<model::ThreadId> thread) {
        if (thread) {
            auto selected = select_thread_native(*thread);
            if (!selected)
                return std::unexpected(selected.error());
        }
        ULONG count{};
        if (const HRESULT status = registers_.get()->GetNumberRegisters(&count); FAILED(status))
            return std::unexpected(detail::hresult_error("IDebugRegisters::GetNumberRegisters", status));
        std::vector<model::Register> result;
        result.reserve(count);
        for (ULONG index = 0; index < count; ++index) {
            char name[128]{};
            ULONG name_size{};
            DEBUG_REGISTER_DESCRIPTION description{};
            const HRESULT status =
                registers_.get()->GetDescription(index, name, sizeof(name), &name_size, &description);
            if (FAILED(status))
                continue;
            result.push_back(register_descriptor(name, description.Type == DEBUG_REGISTER_SUB_REGISTER ? 32U : 64U));
        }
        return result;
    }

    /// Reads one native register and converts its scalar/vector bytes.
    [[nodiscard]] core::Result<model::RegisterValue> read_register_native(const std::string& name,
                                                                          std::optional<model::ThreadId> thread) {
        if (thread) {
            auto selected = select_thread_native(*thread);
            if (!selected)
                return std::unexpected(selected.error());
        }
        ULONG index{};
        std::string native_name = name;
        HRESULT status = registers_.get()->GetIndexByName(native_name.c_str(), &index);
        if (FAILED(status)) {
            std::ranges::transform(native_name, native_name.begin(),
                                   [](unsigned char value) { return static_cast<char>(std::toupper(value)); });
            if (native_name == "RFLAGS") {
                native_name = "efl";
                status = registers_.get()->GetIndexByName(native_name.c_str(), &index);
            }
        }
        if (FAILED(status))
            return std::unexpected(detail::hresult_error("IDebugRegisters::GetIndexByName", status));
        DEBUG_VALUE value{};
        status = registers_.get()->GetValue(index, &value);
        if (FAILED(status))
            return std::unexpected(detail::hresult_error("IDebugRegisters::GetValue", status));
        auto descriptor = register_descriptor(name, value.Type == DEBUG_VALUE_INT32 ? 32U : 64U);
        return model::RegisterValue{std::move(descriptor), register_bytes(value), model::ByteOrder::little};
    }

    /// Reads every available register, preserving partial descriptions as a best-effort query.
    [[nodiscard]] core::Result<std::vector<model::RegisterValue>>
    read_registers_native(std::optional<model::ThreadId> thread) {
        auto descriptions = registers_native(thread);
        if (!descriptions)
            return std::unexpected(descriptions.error());
        std::vector<model::RegisterValue> result;
        result.reserve(descriptions->size());
        for (const auto& description : *descriptions) {
            auto value = read_register_native(description.name, std::nullopt);
            if (value)
                result.push_back(std::move(*value));
        }
        return result;
    }

    /// Reads the instruction offset reported by IDebugControl.
    [[nodiscard]] core::Result<core::Address> instruction_pointer_native(std::optional<model::ThreadId> thread) {
        if (thread) {
            auto selected = select_thread_native(*thread);
            if (!selected)
                return std::unexpected(selected.error());
        }
        ULONG64 offset{};
        if (const HRESULT status = registers_.get()->GetInstructionOffset(&offset); FAILED(status))
            return std::unexpected(detail::hresult_error("IDebugRegisters::GetInstructionOffset", status));
        return core::Address{process_space_, offset};
    }

    /// Reads a bounded virtual range while preserving requested and transferred sizes.
    [[nodiscard]] core::Result<model::MemoryReadResult> read_memory_native(core::Address address, std::size_t size) {
        if (size > (std::numeric_limits<ULONG>::max)())
            return std::unexpected(core::Error::make(core::DiagnosticCode::invalid_argument,
                                                     "Debugger memory read exceeds the native transfer limit"));
        if (size == 0)
            return model::MemoryReadResult{address, 0, {}, 0};
        std::vector<core::Byte> result(size);
        ULONG transferred{};
        const HRESULT status =
            data_spaces_.get()->ReadVirtual(address.offset, result.data(), static_cast<ULONG>(size), &transferred);
        if (FAILED(status) && transferred == 0)
            return std::unexpected(detail::hresult_error("IDebugDataSpaces::ReadVirtual", status));
        result.resize(transferred);
        return model::MemoryReadResult{address, size, core::Bytes{std::move(result)}, transferred};
    }

    /// Writes a bounded virtual range and rejects partial writes as a generic I/O failure.
    [[nodiscard]] core::Result<void> write_memory_native(core::Address address, const std::vector<core::Byte>& bytes) {
        if (bytes.size() > (std::numeric_limits<ULONG>::max)())
            return std::unexpected(core::Error::make(core::DiagnosticCode::invalid_argument,
                                                     "Debugger memory write exceeds the native transfer limit"));
        ULONG transferred{};
        const HRESULT status = data_spaces_.get()->WriteVirtual(address.offset, const_cast<core::Byte*>(bytes.data()),
                                                                static_cast<ULONG>(bytes.size()), &transferred);
        if (FAILED(status) || transferred != bytes.size())
            return std::unexpected(
                detail::hresult_error("IDebugDataSpaces::WriteVirtual", FAILED(status) ? status : E_FAIL));
        return {};
    }

    /// Queries virtual memory mappings page by page through IDebugDataSpaces.
    [[nodiscard]] core::Result<std::vector<model::MemoryRegion>> memory_regions_native() {
        std::vector<model::MemoryRegion> result;
        ULONG64 address{};
        for (std::size_t count = 0; count < 1'000'000; ++count) {
            MEMORY_BASIC_INFORMATION64 info{};
            const HRESULT status = data_spaces_.get()->QueryVirtual(address, &info);
            if (FAILED(status))
                break;
            if (info.RegionSize == 0 || info.RegionSize > (std::numeric_limits<ULONG64>::max)() - address)
                break;
            if (info.State == MEM_COMMIT) {
                result.push_back(model::MemoryRegion{
                    core::Address{process_space_, info.BaseAddress}, info.RegionSize,
                    (info.Protect & PAGE_NOACCESS) == 0,
                    (info.Protect &
                     (PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)) != 0,
                    (info.Protect &
                     (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)) != 0,
                    "virtual"});
            }
            address = info.BaseAddress + info.RegionSize;
            if (address == 0)
                break;
        }
        return result;
    }

    /// Walks native stack frames and resolves symbols by offset where available.
    [[nodiscard]] core::Result<std::vector<model::StackFrame>> stack_trace_native(std::optional<model::ThreadId> thread,
                                                                                  std::size_t maximum_frames) {
        if (thread) {
            auto selected = select_thread_native(*thread);
            if (!selected)
                return std::unexpected(selected.error());
        }
        const ULONG count = static_cast<ULONG>(std::min<std::size_t>(maximum_frames, 256));
        std::vector<DEBUG_STACK_FRAME> native_frames(count);
        ULONG filled{};
        const HRESULT status = control_.get()->GetStackTrace(0, 0, 0, native_frames.data(), count, &filled);
        if (FAILED(status))
            return std::unexpected(detail::hresult_error("IDebugControl::GetStackTrace", status));
        std::vector<model::StackFrame> result;
        result.reserve(filled);
        for (ULONG index = 0; index < filled; ++index) {
            auto& frame = native_frames[index];
            char name[256]{};
            ULONG name_size{};
            ULONG64 displacement{};
            const HRESULT symbol_status =
                symbols_.get()->GetNameByOffset(frame.InstructionOffset, name, sizeof(name), &name_size, &displacement);
            result.push_back(model::StackFrame{index,
                                               core::Address{process_space_, frame.InstructionOffset},
                                               core::Address{process_space_, frame.StackOffset},
                                               core::Address{process_space_, frame.FrameOffset},
                                               SUCCEEDED(symbol_status) ? std::string{name} : std::string{},
                                               {},
                                               {}});
        }
        return result;
    }

    /// Enumerates DbgEng module metadata and symbol availability.
    [[nodiscard]] core::Result<std::vector<model::Module>> modules_native() {
        ULONG loaded{};
        ULONG unloaded{};
        if (const HRESULT status = symbols_.get()->GetNumberModules(&loaded, &unloaded); FAILED(status))
            return std::unexpected(detail::hresult_error("IDebugSymbols::GetNumberModules", status));
        std::vector<model::Module> result;
        result.reserve(loaded);
        for (ULONG index = 0; index < loaded; ++index) {
            ULONG64 base{};
            if (const HRESULT status = symbols_.get()->GetModuleByIndex(index, &base); FAILED(status))
                continue;
            DEBUG_MODULE_PARAMETERS parameters{};
            ULONG64 base_array = base;
            if (const HRESULT status = symbols_.get()->GetModuleParameters(1, &base_array, 0, &parameters);
                FAILED(status))
                continue;
            char image[512]{};
            char module_name[512]{};
            char loaded_image[512]{};
            ULONG image_size{};
            ULONG module_size{};
            ULONG loaded_size{};
            static_cast<void>(symbols_.get()->GetModuleNames(index, base, image, sizeof(image), &image_size,
                                                             module_name, sizeof(module_name), &module_size,
                                                             loaded_image, sizeof(loaded_image), &loaded_size));
            result.push_back(model::Module{
                module_name[0] != '\0' ? module_name : image, loaded_image[0] != '\0' ? loaded_image : image,
                core::Address{process_space_, base}, parameters.Size, parameters.Flags != 0});
        }
        return result;
    }

    /// Resolves one symbol through IDebugSymbols::GetOffsetByName.
    [[nodiscard]] core::Result<core::Address> resolve_symbol_native(const std::string& name) {
        ULONG64 offset{};
        const HRESULT status = symbols_.get()->GetOffsetByName(name.c_str(), &offset);
        if (FAILED(status))
            return std::unexpected(detail::hresult_error("IDebugSymbols::GetOffsetByName", status));
        return core::Address{process_space_, offset};
    }

    /// Installs and records one code breakpoint while keeping its native ID private.
    [[nodiscard]] core::Result<model::Breakpoint> add_breakpoint_native(core::Address address,
                                                                        model::BreakpointKind kind, bool one_shot) {
        ULONG native_type = DEBUG_BREAKPOINT_CODE;
        if (kind == model::BreakpointKind::hardware)
            native_type = DEBUG_BREAKPOINT_CODE;
        detail::NativePtr<IDebugBreakpoint> native;
        const HRESULT status = control_.get()->AddBreakpoint(native_type, DEBUG_ANY_ID, native.put());
        if (FAILED(status))
            return std::unexpected(detail::hresult_error("IDebugControl::AddBreakpoint", status));
        if (const HRESULT offset_status = native.get()->SetOffset(address.offset); FAILED(offset_status))
            return std::unexpected(detail::hresult_error("IDebugBreakpoint::SetOffset", offset_status));
        if (const HRESULT flags_status = native.get()->SetFlags(DEBUG_BREAKPOINT_ENABLED); FAILED(flags_status))
            return std::unexpected(detail::hresult_error("IDebugBreakpoint::SetFlags", flags_status));
        ULONG native_id{};
        if (const HRESULT id_status = native.get()->GetId(&native_id); FAILED(id_status))
            return std::unexpected(detail::hresult_error("IDebugBreakpoint::GetId", id_status));
        const model::BreakpointId id{next_breakpoint_id_++};
        model::Breakpoint value{id, kind, address, true, one_shot, 0, {}};
        breakpoints_.emplace(id, value);
        breakpoint_native_.emplace(id, native_id);
        native_to_breakpoint_.emplace(native_id, id);
        return value;
    }

    /// Converts a generic breakpoint ID to a native enabled flag update.
    [[nodiscard]] core::Result<void> set_breakpoint_enabled_native(model::BreakpointId id, bool enabled) {
        const auto iterator = breakpoint_native_.find(id);
        if (iterator == breakpoint_native_.end())
            return std::unexpected(
                core::Error::make(core::DiagnosticCode::invalid_argument, "Unknown debugger code breakpoint identity"));
        auto native = breakpoint_object(iterator->second);
        if (!native)
            return std::unexpected(native.error());
        ULONG flags{};
        if (const HRESULT status = native->get()->GetFlags(&flags); FAILED(status))
            return std::unexpected(detail::hresult_error("IDebugBreakpoint::GetFlags", status));
        if (enabled)
            flags |= DEBUG_BREAKPOINT_ENABLED;
        else
            flags &= ~DEBUG_BREAKPOINT_ENABLED;
        if (const HRESULT status = native->get()->SetFlags(flags); FAILED(status))
            return std::unexpected(detail::hresult_error("IDebugBreakpoint::SetFlags", status));
        breakpoints_.at(id).enabled = enabled;
        return {};
    }

    /// Removes a native breakpoint and all generic lookup mappings.
    [[nodiscard]] core::Result<void> remove_breakpoint_native(model::BreakpointId id) {
        const auto iterator = breakpoint_native_.find(id);
        if (iterator == breakpoint_native_.end())
            return std::unexpected(
                core::Error::make(core::DiagnosticCode::invalid_argument, "Unknown debugger code breakpoint identity"));
        auto native = breakpoint_object(iterator->second);
        if (!native)
            return std::unexpected(native.error());
        if (const HRESULT status = control_.get()->RemoveBreakpoint(native->get()); FAILED(status))
            return std::unexpected(detail::hresult_error("IDebugControl::RemoveBreakpoint", status));
        native_to_breakpoint_.erase(iterator->second);
        breakpoint_native_.erase(iterator);
        breakpoints_.erase(id);
        return {};
    }

    /// Installs a DbgEng data breakpoint and maps the generic access semantics.
    [[nodiscard]] core::Result<model::Watchpoint> add_watchpoint_native(core::Address address, std::size_t size,
                                                                        model::WatchpointAccess access) {
        if (access == model::WatchpointAccess::execute)
            return std::unexpected(
                core::Error::make(core::DiagnosticCode::unsupported,
                                  "DbgEng data breakpoints cannot represent execute-only watchpoint access",
                                  "Use add_breakpoint for instruction execution stops."));
        detail::NativePtr<IDebugBreakpoint> native;
        const HRESULT status = control_.get()->AddBreakpoint(DEBUG_BREAKPOINT_DATA, DEBUG_ANY_ID, native.put());
        if (FAILED(status))
            return std::unexpected(detail::hresult_error("IDebugControl::AddBreakpoint(data)", status));
        if (const HRESULT offset_status = native.get()->SetOffset(address.offset); FAILED(offset_status))
            return std::unexpected(detail::hresult_error("IDebugBreakpoint::SetOffset(data)", offset_status));
        ULONG flags = access == model::WatchpointAccess::read    ? DEBUG_BREAK_READ
                      : access == model::WatchpointAccess::write ? DEBUG_BREAK_WRITE
                                                                 : DEBUG_BREAK_READ | DEBUG_BREAK_WRITE;
        if (const HRESULT data_status = native.get()->SetDataParameters(static_cast<ULONG>(size), flags);
            FAILED(data_status))
            return std::unexpected(detail::hresult_error("IDebugBreakpoint::SetDataParameters", data_status));
        if (const HRESULT flags_status = native.get()->SetFlags(DEBUG_BREAKPOINT_ENABLED); FAILED(flags_status))
            return std::unexpected(detail::hresult_error("IDebugBreakpoint::SetFlags(data)", flags_status));
        ULONG native_id{};
        if (const HRESULT id_status = native.get()->GetId(&native_id); FAILED(id_status))
            return std::unexpected(detail::hresult_error("IDebugBreakpoint::GetId(data)", id_status));
        const model::WatchpointId id{next_watchpoint_id_++};
        model::Watchpoint value{id, address, size, access, true, 0};
        watchpoints_.emplace(id, value);
        watchpoint_native_.emplace(id, native_id);
        native_to_watchpoint_.emplace(native_id, id);
        return value;
    }

    /// Updates a data breakpoint enabled flag using the same native mechanism as code breakpoints.
    [[nodiscard]] core::Result<void> set_watchpoint_enabled_native(model::WatchpointId id, bool enabled) {
        const auto iterator = watchpoint_native_.find(id);
        if (iterator == watchpoint_native_.end())
            return std::unexpected(
                core::Error::make(core::DiagnosticCode::invalid_argument, "Unknown debugger watchpoint identity"));
        auto native = breakpoint_object(iterator->second);
        if (!native)
            return std::unexpected(native.error());
        ULONG flags{};
        if (const HRESULT status = native->get()->GetFlags(&flags); FAILED(status))
            return std::unexpected(detail::hresult_error("IDebugBreakpoint::GetFlags(data)", status));
        if (enabled)
            flags |= DEBUG_BREAKPOINT_ENABLED;
        else
            flags &= ~DEBUG_BREAKPOINT_ENABLED;
        if (const HRESULT status = native->get()->SetFlags(flags); FAILED(status))
            return std::unexpected(detail::hresult_error("IDebugBreakpoint::SetFlags(data)", status));
        watchpoints_.at(id).enabled = enabled;
        return {};
    }

    /// Removes a native data breakpoint and its generic mappings.
    [[nodiscard]] core::Result<void> remove_watchpoint_native(model::WatchpointId id) {
        const auto iterator = watchpoint_native_.find(id);
        if (iterator == watchpoint_native_.end())
            return std::unexpected(
                core::Error::make(core::DiagnosticCode::invalid_argument, "Unknown debugger watchpoint identity"));
        auto native = breakpoint_object(iterator->second);
        if (!native)
            return std::unexpected(native.error());
        if (const HRESULT status = control_.get()->RemoveBreakpoint(native->get()); FAILED(status))
            return std::unexpected(detail::hresult_error("IDebugControl::RemoveBreakpoint(data)", status));
        native_to_watchpoint_.erase(iterator->second);
        watchpoint_native_.erase(iterator);
        watchpoints_.erase(id);
        return {};
    }

    /// Receives DbgEng event callbacks and captures only immutable values before returning.
    class EventCallbacks final : public IDebugEventCallbacks, public detail::CallbackBase {
    public:
        /// Associates callbacks with one engine-thread-owned session.
        explicit EventCallbacks(WinDbgEngSession* owner) : owner_(owner) {}

        /// Supports the COM identity query required by DbgEng callback registration.
        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID identifier, PVOID* object) override {
            if (object == nullptr)
                return E_INVALIDARG;
            if (identifier == __uuidof(IUnknown) || identifier == __uuidof(IDebugEventCallbacks)) {
                *object = static_cast<IDebugEventCallbacks*>(this);
                AddRef();
                return S_OK;
            }
            *object = nullptr;
            return E_NOINTERFACE;
        }

        /// Adds one DbgEng callback reference.
        ULONG STDMETHODCALLTYPE AddRef() override {
            return CallbackBase::AddRef();
        }

        /// Releases one DbgEng callback reference.
        ULONG STDMETHODCALLTYPE Release() override {
            return CallbackBase::Release();
        }

        /// Requests all target and engine events needed for generic translation.
        HRESULT STDMETHODCALLTYPE GetInterestMask(PULONG mask) override {
            if (mask == nullptr)
                return E_INVALIDARG;
            *mask = DEBUG_EVENT_BREAKPOINT | DEBUG_EVENT_EXCEPTION | DEBUG_EVENT_CREATE_THREAD |
                    DEBUG_EVENT_EXIT_THREAD | DEBUG_EVENT_CREATE_PROCESS | DEBUG_EVENT_EXIT_PROCESS |
                    DEBUG_EVENT_LOAD_MODULE | DEBUG_EVENT_UNLOAD_MODULE | DEBUG_EVENT_SESSION_STATUS |
                    DEBUG_EVENT_CHANGE_DEBUGGEE_STATE | DEBUG_EVENT_CHANGE_ENGINE_STATE |
                    DEBUG_EVENT_CHANGE_SYMBOL_STATE;
            return S_OK;
        }

        /// Captures a code or data breakpoint ID and returns a stop status.
        HRESULT STDMETHODCALLTYPE Breakpoint(PDEBUG_BREAKPOINT breakpoint) override {
            if (breakpoint != nullptr) {
                ULONG id{};
                if (SUCCEEDED(breakpoint->GetId(&id)))
                    owner_->on_breakpoint(id);
            }
            return DEBUG_STATUS_BREAK;
        }

        /// Ignores debuggee-state notifications because the explicit wait result owns stop completion.
        HRESULT STDMETHODCALLTYPE ChangeDebuggeeState(ULONG, ULONG64) override {
            return DEBUG_STATUS_NO_CHANGE;
        }

        /// Ignores engine-state notifications that do not represent a target event.
        HRESULT STDMETHODCALLTYPE ChangeEngineState(ULONG, ULONG64) override {
            return DEBUG_STATUS_NO_CHANGE;
        }

        /// Ignores symbol-state notifications; module snapshots are refreshed on demand.
        HRESULT STDMETHODCALLTYPE ChangeSymbolState(ULONG, ULONG64) override {
            return DEBUG_STATUS_NO_CHANGE;
        }

        /// Marks process creation for snapshot emission after WaitForEvent returns.
        HRESULT STDMETHODCALLTYPE CreateProcess(ULONG64, ULONG64, ULONG64, ULONG, PCSTR, PCSTR, ULONG, ULONG, ULONG64,
                                                ULONG64, ULONG64) override {
            owner_->on_process_created();
            return DEBUG_STATUS_BREAK;
        }

        /// Marks thread creation for later enumeration refresh.
        HRESULT STDMETHODCALLTYPE CreateThread(ULONG64, ULONG64, ULONG64) override {
            owner_->on_thread_created();
            return owner_->options_.stop_on_thread_events ? DEBUG_STATUS_BREAK : DEBUG_STATUS_NO_CHANGE;
        }

        /// Captures exception code/address before DbgEng invalidates the record.
        HRESULT STDMETHODCALLTYPE Exception(PEXCEPTION_RECORD64 exception, ULONG first_chance) override {
            if (exception != nullptr)
                owner_->on_exception(*exception, first_chance);
            return (first_chance == 0 || owner_->options_.stop_on_first_chance_exceptions) ? DEBUG_STATUS_BREAK
                                                                                           : DEBUG_STATUS_NO_CHANGE;
        }

        /// Captures process exit code without querying DbgEng reentrantly.
        HRESULT STDMETHODCALLTYPE ExitProcess(ULONG exit_code) override {
            owner_->on_process_exited(exit_code);
            return DEBUG_STATUS_BREAK;
        }

        /// Captures thread exit code and selected thread identity.
        HRESULT STDMETHODCALLTYPE ExitThread(ULONG exit_code) override {
            owner_->on_thread_exited(exit_code);
            return owner_->options_.stop_on_thread_events ? DEBUG_STATUS_BREAK : DEBUG_STATUS_NO_CHANGE;
        }

        /// Captures loaded module metadata for the generic event queue.
        HRESULT STDMETHODCALLTYPE LoadModule(ULONG64 image_file_handle, ULONG64 base_offset, ULONG module_size,
                                             PCSTR module_name, PCSTR image_name, ULONG checksum,
                                             ULONG time_date_stamp) override {
            static_cast<void>(image_file_handle);
            static_cast<void>(checksum);
            static_cast<void>(time_date_stamp);
            owner_->on_module_loaded(base_offset, module_size, module_name, image_name);
            return owner_->options_.stop_on_shared_library_events ? DEBUG_STATUS_BREAK : DEBUG_STATUS_NO_CHANGE;
        }

        /// Captures unloaded module identity without symbol queries.
        HRESULT STDMETHODCALLTYPE UnloadModule(PCSTR image_base_name, ULONG64 base_offset) override {
            owner_->on_module_unloaded(base_offset, image_base_name);
            return DEBUG_STATUS_NO_CHANGE;
        }

        /// Ignores session notifications that do not carry a target stop reason.
        HRESULT STDMETHODCALLTYPE SessionStatus(ULONG) override {
            return DEBUG_STATUS_NO_CHANGE;
        }

        /// Converts system errors into a diagnostic event without throwing from the callback.
        HRESULT STDMETHODCALLTYPE SystemError(ULONG error, ULONG level) override {
            model::DebugException exception{error, core::Address{owner_->process_space_, 0}, false,
                                            "DbgEng system error"};
            owner_->emit_event(
                model::DebugEvent{owner_->next_event_++, model::EventKind::exception, model::ExceptionEvent {
                                      owner_->current_process_id_,
                                      owner_->current_thread_id_,
                                      exception
                                  }});
            static_cast<void>(level);
            return DEBUG_STATUS_BREAK;
        }

    private:
        WinDbgEngSession* owner_;
    };

    /// Suppresses DbgEng diagnostic output so native text cannot leak into the generic API.
    class OutputCallbacks final : public IDebugOutputCallbacks, public detail::CallbackBase {
    public:
        /// Constructs a sink that intentionally discards engine output.
        OutputCallbacks() = default;

        /// Supports the COM identity query required by DbgEng callback registration.
        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID identifier, PVOID* object) override {
            if (object == nullptr)
                return E_INVALIDARG;
            if (identifier == __uuidof(IUnknown) || identifier == __uuidof(IDebugOutputCallbacks)) {
                *object = static_cast<IDebugOutputCallbacks*>(this);
                AddRef();
                return S_OK;
            }
            *object = nullptr;
            return E_NOINTERFACE;
        }

        /// Adds one output callback reference.
        ULONG STDMETHODCALLTYPE AddRef() override {
            return CallbackBase::AddRef();
        }

        /// Releases one output callback reference.
        ULONG STDMETHODCALLTYPE Release() override {
            return CallbackBase::Release();
        }

        /// Discards output while keeping callback execution lightweight.
        HRESULT STDMETHODCALLTYPE Output(ULONG, PCSTR) override {
            return S_OK;
        }
    };
#else
    /// Reports that process queries are unavailable on non-Windows builds.
    [[nodiscard]] core::Result<model::Process> current_process_native() const {
        return std::unexpected(detail::unsupported_platform());
    }

    /// Reports that process enumeration is unavailable on non-Windows builds.
    [[nodiscard]] core::Result<std::vector<model::Process>> processes_native() {
        return std::unexpected(detail::unsupported_platform());
    }

    /// Reports that thread enumeration is unavailable on non-Windows builds.
    [[nodiscard]] core::Result<std::vector<model::Thread>> threads_native() {
        return std::unexpected(detail::unsupported_platform());
    }

    /// Reports that thread selection is unavailable on non-Windows builds.
    [[nodiscard]] core::Result<void> select_thread_native(model::ThreadId) {
        return std::unexpected(detail::unsupported_platform());
    }

    /// Reports that current-thread queries are unavailable on non-Windows builds.
    [[nodiscard]] core::Result<model::ThreadId> current_thread_native() {
        return std::unexpected(detail::unsupported_platform());
    }

    /// Reports that register enumeration is unavailable on non-Windows builds.
    [[nodiscard]] core::Result<std::vector<model::Register>> registers_native(std::optional<model::ThreadId>) {
        return std::unexpected(detail::unsupported_platform());
    }

    /// Reports that register reads are unavailable on non-Windows builds.
    [[nodiscard]] core::Result<model::RegisterValue> read_register_native(const std::string&,
                                                                          std::optional<model::ThreadId>) {
        return std::unexpected(detail::unsupported_platform());
    }

    /// Reports that register reads are unavailable on non-Windows builds.
    [[nodiscard]] core::Result<std::vector<model::RegisterValue>>
    read_registers_native(std::optional<model::ThreadId>) {
        return std::unexpected(detail::unsupported_platform());
    }

    /// Reports that instruction-pointer queries are unavailable on non-Windows builds.
    [[nodiscard]] core::Result<core::Address> instruction_pointer_native(std::optional<model::ThreadId>) {
        return std::unexpected(detail::unsupported_platform());
    }

    /// Reports that memory reads are unavailable on non-Windows builds.
    [[nodiscard]] core::Result<model::MemoryReadResult> read_memory_native(core::Address, std::size_t) {
        return std::unexpected(detail::unsupported_platform());
    }

    /// Reports that memory writes are unavailable on non-Windows builds.
    [[nodiscard]] core::Result<void> write_memory_native(core::Address, const std::vector<core::Byte>&) {
        return std::unexpected(detail::unsupported_platform());
    }

    /// Reports that memory-region enumeration is unavailable on non-Windows builds.
    [[nodiscard]] core::Result<std::vector<model::MemoryRegion>> memory_regions_native() {
        return std::unexpected(detail::unsupported_platform());
    }

    /// Reports that stack walking is unavailable on non-Windows builds.
    [[nodiscard]] core::Result<std::vector<model::StackFrame>> stack_trace_native(std::optional<model::ThreadId>,
                                                                                  std::size_t) {
        return std::unexpected(detail::unsupported_platform());
    }

    /// Reports that module enumeration is unavailable on non-Windows builds.
    [[nodiscard]] core::Result<std::vector<model::Module>> modules_native() {
        return std::unexpected(detail::unsupported_platform());
    }

    /// Reports that symbol resolution is unavailable on non-Windows builds.
    [[nodiscard]] core::Result<core::Address> resolve_symbol_native(const std::string&) {
        return std::unexpected(detail::unsupported_platform());
    }

    /// Reports that code breakpoints are unavailable on non-Windows builds.
    [[nodiscard]] core::Result<model::Breakpoint> add_breakpoint_native(core::Address, model::BreakpointKind, bool) {
        return std::unexpected(detail::unsupported_platform());
    }

    /// Reports that code breakpoint updates are unavailable on non-Windows builds.
    [[nodiscard]] core::Result<void> set_breakpoint_enabled_native(model::BreakpointId, bool) {
        return std::unexpected(detail::unsupported_platform());
    }

    /// Reports that code breakpoint removal is unavailable on non-Windows builds.
    [[nodiscard]] core::Result<void> remove_breakpoint_native(model::BreakpointId) {
        return std::unexpected(detail::unsupported_platform());
    }

    /// Reports that watchpoints are unavailable on non-Windows builds.
    [[nodiscard]] core::Result<model::Watchpoint> add_watchpoint_native(core::Address, std::size_t,
                                                                        model::WatchpointAccess) {
        return std::unexpected(detail::unsupported_platform());
    }

    /// Reports that watchpoint updates are unavailable on non-Windows builds.
    [[nodiscard]] core::Result<void> set_watchpoint_enabled_native(model::WatchpointId, bool) {
        return std::unexpected(detail::unsupported_platform());
    }

    /// Reports that watchpoint removal is unavailable on non-Windows builds.
    [[nodiscard]] core::Result<void> remove_watchpoint_native(model::WatchpointId) {
        return std::unexpected(detail::unsupported_platform());
    }
#endif

    model::SessionId id_;
    model::SessionOptions options_;
    std::atomic<model::SessionState> state_{model::SessionState::created};
    std::atomic_bool stopping_{};
    std::jthread engine_;
#ifdef _WIN32
    std::jthread interrupt_;
    std::mutex interrupt_mutex_;
    std::condition_variable interrupt_condition_;
    bool interrupt_requested_{};
    std::optional<std::chrono::steady_clock::time_point> deadline_;
    std::atomic_bool timeout_requested_{};
#endif
    mutable std::mutex queue_mutex_;
    std::condition_variable queue_condition_;
    std::deque<QueuedCommand> commands_;
    std::deque<model::DebugEvent> events_;
    std::deque<model::DebugEvent> sink_events_;
    api::DebugEventSink event_sink_;
    std::optional<PendingWait> pending_;
    std::optional<PendingWait> pause_pending_;
    bool waiting_for_event_{};
    bool pause_requested_{};
    std::atomic_bool pause_requested_hint_{};
    model::StopReasonKind expected_stop_kind_{model::StopReasonKind::unknown};
    model::StopReason last_stop_reason_;
    model::SessionState state_before_target_{};
    bool process_created_pending_{};
    bool process_exited_{};
    bool thread_event_pending_{};
    std::optional<model::BreakpointId> temporary_step_out_breakpoint_;
    std::vector<model::BreakpointId> temporary_breakpoints_to_remove_;
    std::uint64_t next_event_{1};
    std::uint64_t next_breakpoint_id_{1};
    std::uint64_t next_watchpoint_id_{1};
    model::ProcessId current_process_id_;
    model::ThreadId current_thread_id_;
    core::AddressSpaceId process_space_{"process"};
    std::unordered_map<model::BreakpointId, model::Breakpoint> breakpoints_;
    std::unordered_map<model::BreakpointId, ULONG> breakpoint_native_;
    std::unordered_map<ULONG, model::BreakpointId> native_to_breakpoint_;
    std::unordered_map<model::WatchpointId, model::Watchpoint> watchpoints_;
    std::unordered_map<model::WatchpointId, ULONG> watchpoint_native_;
    std::unordered_map<ULONG, model::WatchpointId> native_to_watchpoint_;

#ifdef _WIN32
    bool com_initialized_{};
    detail::NativePtr<IDebugClient> client_;
    detail::NativePtr<IDebugControl2> control_;
    detail::NativePtr<IDebugRegisters> registers_;
    detail::NativePtr<IDebugDataSpaces2> data_spaces_;
    detail::NativePtr<IDebugSymbols> symbols_;
    detail::NativePtr<IDebugSystemObjects> system_objects_;
    std::unique_ptr<EventCallbacks> event_callbacks_;
    std::unique_ptr<OutputCallbacks> output_callbacks_;
#endif
};

/// Provides the first concrete implementation of the generic debugger service.
class WinDbgEngDebugger final : public api::IDebugger {
public:
    /// Creates an empty backend factory; DbgEng is initialized per session.
    WinDbgEngDebugger() = default;

    /// Creates a session with an isolated dedicated engine thread.
    [[nodiscard]] core::Result<std::shared_ptr<api::IDebugSession>>
    create_session(model::SessionOptions options) override {
        try {
            static std::atomic<std::uint64_t> next_session{1};
            return std::shared_ptr<api::IDebugSession>(
                std::make_shared<WinDbgEngSession>(model::SessionId{next_session.fetch_add(1)}, std::move(options)));
        } catch (const std::exception& error) {
            return std::unexpected(detail::native_error("create session", error.what()));
        } catch (...) {
            return std::unexpected(detail::native_error("create session", "unknown exception"));
        }
    }
};

/// Constructs the concrete DbgEng service without exposing native initialization details.
[[nodiscard]] inline core::Result<std::shared_ptr<api::IDebugger>> create_win_dbg_eng() {
#ifndef _WIN32
    return std::unexpected(detail::unsupported_platform());
#else
    return std::shared_ptr<api::IDebugger>(std::make_shared<WinDbgEngDebugger>());
#endif
}

} // namespace ghidra::services::debugger::win_dbg_eng
