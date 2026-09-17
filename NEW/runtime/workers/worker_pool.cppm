export module recode.runtime.workers.pool;

import std;
import recode.core.contracts.operation;
import recode.core.diagnostics;
import recode.core.identifiers;

export namespace recode::runtime::workers {

namespace core = recode::core;
namespace contracts = recode::core::contracts;
using core::contracts::WorkPriority;

/// Describes bounded shared-pool construction and shutdown behavior.
struct WorkerPoolConfig {
    std::size_t worker_count{std::max<std::size_t>(
        1, std::thread::hardware_concurrency() > 1 ? std::thread::hardware_concurrency() - 1 : 1)};
    std::size_t queue_capacity{256};
};

/// Executes runtime tasks in deterministic priority/sequence order.
class WorkerPool final {
public:
    /// Starts the configured worker threads and bounded queue.
    explicit WorkerPool(WorkerPoolConfig config = {}) : capacity_(config.queue_capacity) {
        if (capacity_ == 0)
            throw std::invalid_argument("Worker pool queue capacity must be positive");
        const auto count = std::max<std::size_t>(1, config.worker_count);
        workers_.reserve(count);
        for (std::size_t index = 0; index < count; ++index)
            workers_.emplace_back([this] { worker_loop(); });
    }

    /// Requests shutdown, drains no unstarted work, and joins all workers.
    ~WorkerPool() {
        shutdown();
    }

    /// Prevents copying a thread-owning pool.
    WorkerPool(const WorkerPool&) = delete;

    /// Prevents copying a thread-owning pool.
    WorkerPool& operator=(const WorkerPool&) = delete;

    /// Queues a callable that receives a cooperative cancellation token.
    template <class Callable>
    [[nodiscard]] auto submit(core::ProjectId project, WorkPriority priority, Callable&& callable)
        -> core::Result<contracts::Task<std::invoke_result_t<Callable, contracts::CancellationToken>>> {
        using ResultType = std::invoke_result_t<Callable, contracts::CancellationToken>;
        auto control = std::make_shared<contracts::OperationControl>();
        auto promise = std::make_shared<std::promise<ResultType>>();
        auto future = promise->get_future().share();
        auto work = [control, promise, callable = std::forward<Callable>(callable)]() mutable {
            if (control->cancellation().stop_requested()) {
                control->set_status(contracts::OperationStatus::cancelled);
                promise->set_exception(
                    std::make_exception_ptr(std::runtime_error("Operation was cancelled before start")));
                return;
            }
            control->set_status(contracts::OperationStatus::running);
            try {
                if constexpr (std::is_void_v<ResultType>) {
                    if constexpr (std::is_invocable_v<Callable, contracts::CancellationToken>)
                        std::invoke(callable, control->cancellation());
                    else
                        std::invoke(callable);
                    control->set_status(control->cancellation().stop_requested()
                                            ? contracts::OperationStatus::cancelled
                                            : contracts::OperationStatus::completed);
                    promise->set_value();
                } else {
                    // Publish the operation state before making the shared future
                    // ready. Otherwise wait()/get() can observe a ready result
                    // while status() still reports running.
                    auto value = [&] {
                        if constexpr (std::is_invocable_v<Callable, contracts::CancellationToken>)
                            return std::invoke(callable, control->cancellation());
                        else
                            return std::invoke(callable);
                    }();
                    control->set_status(control->cancellation().stop_requested()
                                            ? contracts::OperationStatus::cancelled
                                            : contracts::OperationStatus::completed);
                    promise->set_value(std::move(value));
                    return;
                }
            } catch (...) {
                control->set_status(contracts::OperationStatus::failed);
                promise->set_exception(std::current_exception());
            }
        };
        {
            std::scoped_lock lock(mutex_);
            if (stopping_)
                return std::unexpected(
                    core::Error::make(core::DiagnosticCode::project_closed, "Worker pool is shutting down"));
            if (queue_.size() >= capacity_)
                return std::unexpected(
                    core::Error::make(core::DiagnosticCode::queue_full, "Worker pool queue is full",
                                      "Wait for an active task or increase the runtime queue capacity."));
            queue_.push_back(WorkItem{project, priority, sequence_++, std::move(work), control});
        }
        condition_.notify_one();
        return contracts::Task<ResultType>{std::move(future), std::move(control)};
    }

    /// Returns the number of queued, not yet running operations.
    [[nodiscard]] std::size_t queued() const noexcept {
        std::scoped_lock lock(mutex_);
        return queue_.size();
    }

    /// Stops accepting work and joins workers exactly once.
    void shutdown() noexcept {
        {
            std::scoped_lock lock(mutex_);
            if (stopping_)
                return;
            stopping_ = true;
        }
        condition_.notify_all();
        for (auto& worker : workers_)
            if (worker.joinable())
                worker.join();
        workers_.clear();
    }

private:
    /// Represents one queued operation and its deterministic scheduling keys.
    struct WorkItem {
        core::ProjectId project;
        WorkPriority priority;
        std::uint64_t sequence;
        std::function<void()> function;
        std::shared_ptr<contracts::OperationControl> control;
    };

    /// Selects and executes work until shutdown and queue drain are complete.
    void worker_loop() {
        for (;;) {
            WorkItem item;
            {
                std::unique_lock lock(mutex_);
                condition_.wait(lock, [&] { return stopping_ || !queue_.empty(); });
                if (queue_.empty()) {
                    if (stopping_)
                        return;
                    continue;
                }
                auto iterator = queue_.begin();
                for (auto candidate = std::next(iterator); candidate != queue_.end(); ++candidate) {
                    const bool higher_priority =
                        static_cast<std::int32_t>(candidate->priority) < static_cast<std::int32_t>(iterator->priority);
                    const bool same_priority = candidate->priority == iterator->priority;
                    if (higher_priority || (same_priority && candidate->sequence < iterator->sequence))
                        iterator = candidate;
                }
                item = std::move(*iterator);
                queue_.erase(iterator);
            }
            item.function();
        }
    }

    std::size_t capacity_;
    mutable std::mutex mutex_;
    std::condition_variable condition_;
    std::vector<std::thread> workers_;
    std::vector<WorkItem> queue_;
    std::uint64_t sequence_{};
    bool stopping_{};
};

} // namespace recode::runtime::workers
