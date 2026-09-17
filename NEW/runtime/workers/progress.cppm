export module recode.runtime.workers.progress;

import std;

export namespace recode::runtime::workers {

/// Carries bounded progress information without borrowing a caller-owned sink.
struct ProgressUpdate {
    std::uint64_t completed{};
    std::uint64_t total{};
    std::string phase;
};

/// Stores a bounded, thread-safe progress queue owned by one operation.
class ProgressChannel final {
public:
    /// Constructs a channel with a fixed maximum number of retained updates.
    explicit ProgressChannel(std::size_t capacity = 64) : capacity_(capacity) {}

    /// Adds an update, dropping the oldest update when the bounded queue is full.
    void publish(ProgressUpdate update) {
        std::scoped_lock lock(mutex_);
        if (updates_.size() == capacity_)
            updates_.pop_front();
        updates_.push_back(std::move(update));
    }

    /// Returns all currently retained updates in publication order.
    [[nodiscard]] std::vector<ProgressUpdate> drain() {
        std::scoped_lock lock(mutex_);
        std::vector<ProgressUpdate> result(updates_.begin(), updates_.end());
        updates_.clear();
        return result;
    }

private:
    std::size_t capacity_;
    std::mutex mutex_;
    std::deque<ProgressUpdate> updates_;
};

} // namespace recode::runtime::workers
