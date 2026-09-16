export module ghidra.bindings.cpp.debugger;

import std;
import ghidra.core.contracts.debugger;
import ghidra.core.contracts.operation;
import ghidra.core.debugger;
import ghidra.core.diagnostics;
import ghidra.core.address;
import ghidra.core.bytes;

// This module is an adapter only.  It intentionally does not import the
// WinDbgEng service or expose any native engine pointer.  Applications choose
// a backend at composition time and hand its generic IDebugger to this facade.

export namespace ghidra::bindings::cpp {

namespace core = ghidra::core;
namespace api = ghidra::core::contracts;
namespace model = ghidra::core::debugger;

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

    /// Returns copied thread snapshots.
    [[nodiscard]] core::Result<std::vector<model::Thread>> threads() const {
        return session_->threads();
    }

    /// Reads target memory through the generic value API.
    [[nodiscard]] core::Result<core::Bytes> read_memory(core::Address address, std::size_t size) const {
        return session_->read_memory(address, size);
    }

    /// Writes target memory through the generic value API.
    [[nodiscard]] core::Result<void> write_memory(core::Address address, core::BytesView bytes) {
        return session_->write_memory(address, bytes);
    }

    /// Drains translated debugger events.
    [[nodiscard]] core::Result<std::vector<model::DebugEvent>> poll_events() {
        return session_->poll_events();
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

} // namespace ghidra::bindings::cpp
