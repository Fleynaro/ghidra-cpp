export module recode.bindings.cpp.debugger;

import std;
import recode.core.contracts.debugger;
import recode.core.contracts.operation;
import recode.core.debugger;
import recode.core.diagnostics;
import recode.core.address;
import recode.core.bytes;

// This module is an adapter only.  It intentionally does not import the
// WinDbgEng service or expose any native engine pointer.  Applications choose
// a backend at composition time and hand its generic IDebugger to this facade.

export namespace recode::bindings::cpp {

namespace core = recode::core;
namespace api = recode::core::contracts;
namespace model = recode::core::debugger;

/// Native binding handle for one backend-independent debugger session.
class DebugSession final {
public:
    /// Returns the stable session identity.
    [[nodiscard]] model::SessionId identity() const noexcept {
        return session_->identity();
    }

    /// Returns the cached lifecycle state.
    [[nodiscard]] model::SessionState state() const noexcept {
        return session_->state();
    }

    /// Launches a process through the generic asynchronous contract.
    [[nodiscard]] api::Task<core::Result<model::Process>> launch(model::LaunchRequest request,
                                                                 api::OperationContext context = {}) {
        return session_->launch(std::move(request), std::move(context));
    }

    /// Attaches to a process through the generic asynchronous contract.
    [[nodiscard]] api::Task<core::Result<model::Process>> attach(model::AttachRequest request,
                                                                 api::OperationContext context = {}) {
        return session_->attach(std::move(request), std::move(context));
    }

    /// Continues execution until the next generic stop event.
    [[nodiscard]] api::Task<core::Result<model::StopReason>> continue_execution(api::OperationContext context = {}) {
        return session_->continue_execution(std::move(context));
    }

    /// Requests a target pause and waits for the actual stopped state.
    [[nodiscard]] api::Task<core::Result<model::StopReason>> pause(api::OperationContext context = {}) {
        return session_->pause(std::move(context));
    }

    /// Performs one step-into operation.
    [[nodiscard]] api::Task<core::Result<model::StopReason>> step_into(api::OperationContext context = {}) {
        return session_->step_into(std::move(context));
    }

    /// Performs one step-over operation.
    [[nodiscard]] api::Task<core::Result<model::StopReason>> step_over(api::OperationContext context = {}) {
        return session_->step_over(std::move(context));
    }

    /// Performs one step-out operation.
    [[nodiscard]] api::Task<core::Result<model::StopReason>> step_out(api::OperationContext context = {}) {
        return session_->step_out(std::move(context));
    }

    /// Returns a copied process snapshot.
    [[nodiscard]] core::Result<model::Process> process() const {
        return session_->process();
    }

    /// Selects the process context used by subsequent generic queries.
    [[nodiscard]] core::Result<void> select_process(model::ProcessId process) {
        return session_->select_process(std::move(process));
    }

    /// Returns the selected process identity.
    [[nodiscard]] core::Result<model::ProcessId> current_process_id() const {
        return session_->current_process_id();
    }

    /// Returns copied thread snapshots.
    [[nodiscard]] core::Result<std::vector<model::Thread>> threads() const {
        return session_->threads();
    }

    /// Selects the thread context used by register and stack queries.
    [[nodiscard]] core::Result<void> select_thread(model::ThreadId thread) {
        return session_->select_thread(std::move(thread));
    }

    /// Returns the selected thread identity.
    [[nodiscard]] core::Result<model::ThreadId> current_thread() const {
        return session_->current_thread();
    }

    /// Enumerates registers exposed by the selected thread context.
    [[nodiscard]] core::Result<std::vector<model::Register>>
    registers(std::optional<model::ThreadId> thread = std::nullopt) const {
        return session_->registers(std::move(thread));
    }

    /// Reads one register value through the generic contract.
    [[nodiscard]] core::Result<model::RegisterValue>
    read_register(std::string_view name, std::optional<model::ThreadId> thread = std::nullopt) const {
        return session_->read_register(name, std::move(thread));
    }

    /// Reads all register values through the generic contract.
    [[nodiscard]] core::Result<std::vector<model::RegisterValue>>
    read_registers(std::optional<model::ThreadId> thread = std::nullopt) const {
        return session_->read_registers(std::move(thread));
    }

    /// Reads the selected thread instruction pointer.
    [[nodiscard]] core::Result<core::Address>
    instruction_pointer(std::optional<model::ThreadId> thread = std::nullopt) const {
        return session_->instruction_pointer(std::move(thread));
    }

    /// Reads target memory through the generic value API.
    [[nodiscard]] core::Result<model::MemoryReadResult> read_memory(core::Address address, std::size_t size) const {
        return session_->read_memory(address, size);
    }

    /// Writes target memory through the generic value API.
    [[nodiscard]] core::Result<void> write_memory(core::Address address, core::BytesView bytes) {
        return session_->write_memory(address, bytes);
    }

    /// Enumerates target memory mappings.
    [[nodiscard]] core::Result<std::vector<model::MemoryRegion>> memory_regions() const {
        return session_->memory_regions();
    }

    /// Walks the selected thread stack.
    [[nodiscard]] core::Result<std::vector<model::StackFrame>>
    stack_trace(std::optional<model::ThreadId> thread = std::nullopt, std::size_t maximum_frames = 64) const {
        return session_->stack_trace(std::move(thread), maximum_frames);
    }

    /// Enumerates loaded target modules.
    [[nodiscard]] core::Result<std::vector<model::Module>> modules() const {
        return session_->modules();
    }

    /// Resolves a symbol/function name to a target address.
    [[nodiscard]] core::Result<core::Address> resolve_symbol(std::string_view name) const {
        return session_->resolve_symbol(name);
    }

    /// Installs a code breakpoint.
    [[nodiscard]] core::Result<model::Breakpoint>
    add_breakpoint(core::Address address, model::BreakpointKind kind = model::BreakpointKind::software,
                   bool one_shot = false) {
        return session_->add_breakpoint(address, kind, one_shot);
    }

    /// Enables or disables a code breakpoint.
    [[nodiscard]] core::Result<void> enable_breakpoint(model::BreakpointId id, bool enabled) {
        return session_->enable_breakpoint(id, enabled);
    }

    /// Removes a code breakpoint.
    [[nodiscard]] core::Result<void> remove_breakpoint(model::BreakpointId id) {
        return session_->remove_breakpoint(id);
    }

    /// Installs a data breakpoint/watchpoint.
    [[nodiscard]] core::Result<model::Watchpoint> add_watchpoint(core::Address address, std::size_t size,
                                                                 model::WatchpointAccess access) {
        return session_->add_watchpoint(address, size, access);
    }

    /// Enables or disables a data breakpoint/watchpoint.
    [[nodiscard]] core::Result<void> enable_watchpoint(model::WatchpointId id, bool enabled) {
        return session_->enable_watchpoint(id, enabled);
    }

    /// Removes a data breakpoint/watchpoint.
    [[nodiscard]] core::Result<void> remove_watchpoint(model::WatchpointId id) {
        return session_->remove_watchpoint(id);
    }

    /// Detaches from the target without terminating it.
    [[nodiscard]] core::Result<void> detach() {
        return session_->detach();
    }

    /// Terminates the target through the generic session contract.
    [[nodiscard]] core::Result<void> terminate() {
        return session_->terminate();
    }

    /// Drains translated debugger events.
    [[nodiscard]] core::Result<std::vector<model::DebugEvent>> poll_events() {
        return session_->poll_events();
    }

    /// Registers a generic event sink with the contract-defined callback lifetime.
    [[nodiscard]] core::Result<void> set_event_sink(api::DebugEventSink sink) {
        return session_->set_event_sink(std::move(sink));
    }

private:
    friend class Debugger;

    /// Wraps an existing generic session implementation without taking backend ownership assumptions.
    explicit DebugSession(std::shared_ptr<api::IDebugSession> session) : session_(std::move(session)) {}

    std::shared_ptr<api::IDebugSession> session_;
};

/// Native binding handle for any implementation of the generic debugger service.
class Debugger final {
public:
    /// Wraps a backend supplied by the application composition root.
    explicit Debugger(std::shared_ptr<api::IDebugger> debugger) : debugger_(std::move(debugger)) {}

    /// Creates one generic session and returns a binding-owned handle.
    [[nodiscard]] core::Result<std::shared_ptr<DebugSession>> create_session(model::SessionOptions options = {}) {
        if (!debugger_)
            return std::unexpected(core::Error::make(core::DiagnosticCode::resource_unavailable,
                                                     "The C++ debugger binding has no backend instance"));
        auto session = debugger_->create_session(std::move(options));
        if (!session)
            return std::unexpected(session.error());
        return std::shared_ptr<DebugSession>(new DebugSession(std::move(*session)));
    }

private:
    std::shared_ptr<api::IDebugger> debugger_;
};

} // namespace recode::bindings::cpp
