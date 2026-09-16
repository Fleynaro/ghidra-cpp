export module ghidra.core.contracts.operation;

import std;
import ghidra.core.diagnostics;
import ghidra.core.identifiers;

export namespace ghidra::core::contracts {

/// Selects whether a command executes inline or through the runtime pool.
enum class ExecutionMode : std::uint8_t { inline_mode, queued };

/// Orders queued work; lower numeric values are more urgent.
enum class WorkPriority : std::int32_t { interactive = 0, analysis = 100, background = 200 };

/// Represents the terminal state of a runtime-owned operation.
enum class OperationStatus : std::uint8_t { queued, running, completed, cancelled, failed, rejected };

/// Provides a thread-safe read-only cancellation query.
class CancellationToken final {
public:
    /// Constructs a token that is not cancelled.
    CancellationToken() : cancelled_(std::make_shared<std::atomic_bool>(false)) {}

    /// Constructs a token observing shared cancellation state.
    explicit CancellationToken(std::shared_ptr<std::atomic_bool> state) : cancelled_(std::move(state)) {}

    /// Reports whether cancellation has been requested.
    [[nodiscard]] bool stop_requested() const noexcept {
        return cancelled_->load(std::memory_order_acquire);
    }

private:
    std::shared_ptr<std::atomic_bool> cancelled_;
};

/// Owns cancellation and terminal status for one queued operation.
class OperationControl final {
public:
    /// Constructs a fresh queued operation state.
    OperationControl() : cancelled_(std::make_shared<std::atomic_bool>(false)) {}

    /// Requests cooperative cancellation.
    void request_cancel() noexcept {
        cancelled_->store(true, std::memory_order_release);
    }

    /// Returns the read-only cancellation token.
    [[nodiscard]] CancellationToken cancellation() const noexcept {
        return CancellationToken{cancelled_};
    }

    /// Updates the operation status for observers.
    void set_status(OperationStatus value) noexcept {
        status_.store(value, std::memory_order_release);
    }

    /// Reads the current operation status.
    [[nodiscard]] OperationStatus status() const noexcept {
        return status_.load(std::memory_order_acquire);
    }

private:
    std::shared_ptr<std::atomic_bool> cancelled_;
    std::atomic<OperationStatus> status_{OperationStatus::queued};
};

/// Carries a shareable result produced by the runtime worker pool.
template <class T> class Task final {
public:
    /// Constructs an invalid task used as a default value.
    Task() = default;

    /// Takes ownership of a shared future and its operation state.
    Task(std::shared_future<T> future, std::shared_ptr<OperationControl> control)
        : future_(std::move(future)), control_(std::move(control)) {}

    /// Reports whether a result state is attached.
    [[nodiscard]] bool valid() const noexcept {
        return future_.valid();
    }

    /// Waits for completion without consuming the result.
    void wait() const {
        future_.wait();
    }

    /// Returns the result, propagating the worker's exception if one escaped.
    [[nodiscard]] T get() const {
        return future_.get();
    }

    /// Requests cooperative cancellation of this operation.
    void cancel() const noexcept {
        if (control_)
            control_->request_cancel();
    }

    /// Returns the shared operation state for status/progress observation.
    [[nodiscard]] std::shared_ptr<OperationControl> control() const noexcept {
        return control_;
    }

private:
    std::shared_future<T> future_;
    std::shared_ptr<OperationControl> control_;
};

/// Carries project identity and cancellation state into a service invocation.
struct OperationContext {
    ProjectId project;
    Revision read_revision;
    CancellationToken cancellation;
    std::shared_ptr<OperationControl> operation;
};

} // namespace ghidra::core::contracts
