export module analyzer_manager;

import analyzer_base;
import analyzer_cancellation_token;
import analyzer_context;
import analyzer_registry;
import analyzer_types;
import std;

// MSVC does not emit inline definitions from independently compiled named
// module interfaces unless the owning class is explicitly exported.
#if defined(_MSC_VER)
#define GHIDRA_ANALYZER_MODULE_EXPORT __declspec(dllexport)
#else
#define GHIDRA_ANALYZER_MODULE_EXPORT
#endif

export namespace ghidra::analyzer {

class AutoAnalysisManager;

/// Supplies the aggregate target's feature registrations without making the
/// shared runtime depend on every optional analyzer library.
export void register_builtin_analyzers_impl(AutoAnalysisManager& manager);

/// Coalesces program events and executes eligible analyzers by priority.
export class GHIDRA_ANALYZER_MODULE_EXPORT AutoAnalysisManager final {
private:
    /// Returns whether an event kind is present in an analyzer trigger set.
    [[nodiscard]] static bool triggered_by(const AnalyzerDescriptor& descriptor, EventKind kind) {
        return descriptor.triggers.contains(kind);
    }

public:
    /// Creates a manager attached to one mutable analysis context.
    explicit AutoAnalysisManager(AnalysisContext& context) : context_(context) {}

    /// Registers one future analyzer implementation.
    void register_analyzer(std::unique_ptr<Analyzer> analyzer) {
        registry_.register_analyzer(std::move(analyzer));
    }

    /// Registers all requested built-in analyzers through the aggregate target adapter.
    void register_builtin_analyzers() {
        register_builtin_analyzers_impl(*this);
    }

    /// Requests cancellation of the current or next run.
    void cancel() noexcept {
        cancellation_.cancel();
    }

    /// Runs the event-driven pipeline from PE memory and metadata seeds.
    [[nodiscard]] AnalysisResult analyze() {
        std::vector<Address> seeds;
        for (const auto& region : context_.image().memory_regions()) {
            if (region.executable) {
                seeds.push_back(region.start);
            }
        }
        if (const auto entry = context_.image().entry_point_va(); entry && context_.image().is_executable(*entry)) {
            seeds.push_back(*entry);
        }
        for (const auto& symbol : context_.image().exported_symbols()) {
            if (!symbol.forwarded && context_.image().is_executable(symbol.address_va)) {
                seeds.push_back(symbol.address_va);
            }
        }
        if (const auto& tls = context_.image().tls(); tls) {
            for (const Address callback : tls->callback_addresses) {
                if (context_.image().is_executable(callback)) {
                    seeds.push_back(callback);
                }
            }
        }
        for (const auto& runtime : context_.image().exception_functions()) {
            if (context_.image().is_executable(runtime.begin_va)) {
                seeds.push_back(runtime.begin_va);
            }
        }
        return analyze(seeds);
    }

    /// Runs the scheduler from explicit seed addresses with deterministic priorities.
    // Ported from Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/analysis/AutoAnalysisManager.java.
    // Relevant methods: startAnalysis(), scheduleAnalysis(), and the event-driven
    // AnalysisTaskList/AnalysisScheduler dispatch loop.
    [[nodiscard]] AnalysisResult analyze(std::span<const Address> seeds) {
        struct PendingTask {
            std::size_t index{};
            std::int32_t priority{};
            std::string name;
            std::uint64_t sequence{};
        };
        struct TaskOrder {
            /// Places the numerically lowest analysis priority at the queue front.
            bool operator()(const PendingTask& left, const PendingTask& right) const {
                if (left.priority != right.priority) {
                    return left.priority > right.priority;
                }
                if (left.name != right.name) {
                    return left.name > right.name;
                }
                return left.sequence > right.sequence;
            }
        };

        AnalysisResult result;
        if (cancellation_.is_cancelled()) {
            result.cancelled = true;
            context_.pending_events_.clear();
            for (const auto& analyzer : registry_.analyzers()) {
                analyzer->analysis_ended(context_, true);
            }
            cancellation_.reset();
            return result;
        }
        for (const auto& analyzer : registry_.analyzers()) {
            const auto descriptor = analyzer->descriptor();
            for (const auto& prerequisite : descriptor.prerequisites) {
                const auto dependency =
                    std::find_if(registry_.analyzers().begin(), registry_.analyzers().end(),
                                 [&](const auto& candidate) { return candidate->descriptor().name == prerequisite; });
                if (dependency == registry_.analyzers().end()) {
                    result.errors.push_back(descriptor.name + ": missing prerequisite analyzer " + prerequisite);
                } else if (dependency->get()->descriptor().priority >= descriptor.priority) {
                    result.errors.push_back(descriptor.name +
                                            ": prerequisite must have a higher priority: " + prerequisite);
                }
            }
        }
        if (!result.errors.empty()) {
            // A rejected analysis setup is still a completed lifecycle attempt. Ghidra
            // notifies analyzers when a run ends, including runs that cannot start because
            // their analysis graph is invalid.
            for (const auto& analyzer : registry_.analyzers()) {
                try {
                    analyzer->analysis_ended(context_, false);
                } catch (const std::exception& error) {
                    result.errors.push_back(analyzer->descriptor().name + ": analysis-ended: " + error.what());
                }
            }
            cancellation_.reset();
            return result;
        }
        if (context_.options().seed_provider_functions) {
            context_.seed_provider_functions();
        }
        std::priority_queue<PendingTask, std::vector<PendingTask>, TaskOrder> queue;
        std::map<std::size_t, std::vector<AnalysisEvent>> pending;
        std::set<std::size_t> scheduled;
        std::uint64_t schedule_sequence = 1;
        std::size_t processed_events = 0;
        std::size_t dispatched_events = 0;
        auto dispatch = [&](const std::vector<AnalysisEvent>& events) {
            for (const auto& event : events) {
                if (++dispatched_events > context_.options().maximum_events) {
                    result.errors.push_back("AutoAnalysisManager: maximum event count exceeded");
                    return;
                }
                for (std::size_t index = 0; index < registry_.analyzers().size(); ++index) {
                    const auto descriptor = registry_.analyzers()[index]->descriptor();
                    if (!triggered_by(descriptor, event.kind)) {
                        continue;
                    }
                    auto& target = pending[index];
                    auto existing = std::find_if(target.begin(), target.end(), [&](const AnalysisEvent& queued) {
                        return queued.kind == event.kind && queued.removed == event.removed;
                    });
                    if (existing == target.end()) {
                        target.push_back(event);
                    } else {
                        for (const Address address : event.addresses) {
                            if (std::find(existing->addresses.begin(), existing->addresses.end(), address) ==
                                existing->addresses.end()) {
                                existing->addresses.push_back(address);
                            }
                        }
                        existing->sequence = std::min(existing->sequence, event.sequence);
                    }
                    if (scheduled.insert(index).second) {
                        queue.push(PendingTask{index, descriptor.priority, descriptor.name, schedule_sequence++});
                    }
                }
            }
        };
        for (const Address seed : seeds) {
            context_.emit(EventKind::memory_added, seed);
        }
        dispatch(std::exchange(context_.pending_events_, std::vector<AnalysisEvent>{}));
        while (!queue.empty()) {
            if (++processed_events > context_.options().maximum_events) {
                result.errors.push_back("AutoAnalysisManager: maximum event count exceeded");
                break;
            }
            if (cancellation_.is_cancelled()) {
                result.cancelled = true;
                break;
            }
            const PendingTask task = queue.top();
            queue.pop();
            scheduled.erase(task.index);
            auto events = std::move(pending[task.index]);
            pending.erase(task.index);
            try {
                std::vector<AnalysisEvent> added;
                std::vector<AnalysisEvent> removed;
                for (const auto& event : events) {
                    (event.removed ? removed : added).push_back(event);
                }
                if (!added.empty()) {
                    registry_.analyzers()[task.index]->analyze(context_, added, cancellation_);
                }
                if (!removed.empty()) {
                    registry_.analyzers()[task.index]->removed(context_, removed, cancellation_);
                }
                result.executed_analyzers.push_back(task.name);
            } catch (const std::exception& error) {
                result.errors.push_back(task.name + ": " + error.what());
            }
            dispatch(std::exchange(context_.pending_events_, std::vector<AnalysisEvent>{}));
        }
        if (result.cancelled) {
            // Mutations performed before cancellation are retained by the context, but
            // their follow-up work must not leak into a later independent run. The caller
            // can explicitly request re-analysis when it is ready to resume.
            pending.clear();
            scheduled.clear();
            while (!queue.empty()) {
                queue.pop();
            }
            context_.pending_events_.clear();
        }
        result.completed = !result.cancelled && result.errors.empty();
        for (const auto& analyzer : registry_.analyzers()) {
            try {
                analyzer->analysis_ended(context_, result.cancelled);
            } catch (const std::exception& error) {
                result.errors.push_back(analyzer->descriptor().name + ": analysis-ended: " + error.what());
                result.completed = false;
            }
        }
        cancellation_.reset();
        return result;
    }

    /// Requeues existing program state for a bounded repeat-analysis pass.
    [[nodiscard]] AnalysisResult re_analyze_all(std::span<const Address> restrict_set = {}) {
        if (restrict_set.empty()) {
            for (const auto& region : context_.image().memory_regions()) {
                context_.emit(EventKind::memory_added, region.start);
            }
            for (const auto& [address, instruction] : context_.instructions()) {
                static_cast<void>(instruction);
                context_.emit(EventKind::code_added, address);
            }
            for (const auto& [address, data] : context_.data()) {
                static_cast<void>(data);
                context_.emit(EventKind::data_added, address);
            }
            for (const auto& [entry, function] : context_.functions()) {
                static_cast<void>(function);
                context_.emit(EventKind::function_added, entry);
            }
            for (const auto& reference : context_.references()) {
                context_.emit(EventKind::reference_added, reference.source);
            }
            for (const auto& fact : context_.constant_facts()) {
                context_.emit(EventKind::constant_added, fact.instruction);
            }
            for (const auto& symbol : context_.external_symbols()) {
                context_.emit(EventKind::external_added, symbol.iat_address);
            }
            return analyze(std::span<const Address>{});
        }
        for (const Address address : restrict_set) {
            context_.emit(EventKind::memory_added, address);
            if (context_.instructions().contains(address)) {
                context_.emit(EventKind::code_added, address);
            }
            if (context_.data().contains(address)) {
                context_.emit(EventKind::data_added, address);
            }
            if (context_.function_containing(address)) {
                context_.emit(EventKind::function_added, context_.function_containing(address)->entry);
            }
            for (const auto& reference : context_.references()) {
                if (reference.source == address || reference.target == address) {
                    context_.emit(EventKind::reference_added, reference.source);
                }
            }
            for (const auto& symbol : context_.external_symbols()) {
                if (symbol.iat_address == address) {
                    context_.emit(EventKind::external_added, address);
                }
            }
        }
        return analyze(std::span<const Address>{});
    }

    /// Returns the registry for inspection and extension.
    [[nodiscard]] const AnalyzerRegistry& registry() const noexcept {
        return registry_;
    }

private:
    AnalysisContext& context_;
    AnalyzerRegistry registry_;
    CancellationToken cancellation_;
};

} // namespace ghidra::analyzer
