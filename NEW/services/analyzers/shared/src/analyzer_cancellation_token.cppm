export module analyzer_cancellation_token;

import std;

export namespace recode::analyzer {

/// Carries cooperative cancellation state into an analyzer invocation.
// Ported from the cooperative cancellation checks used by
// Ghidra/Framework/Project/src/main/java/ghidra/framework/TaskMonitor.java.
class CancellationToken final {
public:
    /// Constructs a token initially in the non-cancelled state.
    CancellationToken() = default;

    /// Requests cancellation of the current analysis run.
    void cancel() noexcept {
        cancelled_.store(true, std::memory_order_release);
    }

    /// Clears cancellation so a new manager run can be started explicitly.
    void reset() noexcept {
        cancelled_.store(false, std::memory_order_release);
    }

    /// Reports whether cancellation was requested.
    [[nodiscard]] bool is_cancelled() const noexcept {
        return cancelled_.load(std::memory_order_acquire);
    }

private:
    std::atomic_bool cancelled_{false};
};

} // namespace recode::analyzer
