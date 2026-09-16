export module ghidra.runtime.analysis.scheduler;

import std;
import ghidra.core;
import ghidra.runtime.analysis.registry;
import ghidra.runtime.analysis.coalescer;
import ghidra.runtime.workers.pool;

export namespace ghidra::runtime::analysis {

namespace core = ghidra::core;
namespace workers = ghidra::runtime::workers;

/// Reports one deterministic scheduler run and all analyzer outcomes.
struct SchedulerReport {
    core::Revision read_revision;
    std::vector<std::string> executed_analyzers;
    std::vector<core::contracts::MutationCommand> commands;
    std::vector<core::Diagnostic> diagnostics;
    std::vector<core::EntityId> affected_entities;
};

/// Schedules read-only analyzer work and serializes mutation proposals for a project commit lane.
class AnalysisScheduler final {
public:
    /// Constructs a scheduler over a registry and shared worker pool.
    AnalysisScheduler(std::shared_ptr<AnalyzerRegistry> registry, std::shared_ptr<workers::WorkerPool> workers)
        : registry_(std::move(registry)), workers_(std::move(workers)) {}

    /// Runs triggered analyzers in prerequisite/priority order with immutable inputs.
    [[nodiscard]] core::Result<SchedulerReport> run(const core::contracts::AnalysisSnapshot& snapshot,
                                                    const core::events::EventBatch& events,
                                                    core::contracts::OperationContext context) {
        auto ordered = registry_->ordered();
        if (!ordered)
            return std::unexpected(ordered.error());
        SchedulerReport report{snapshot.revision};
        for (const auto& analyzer : *ordered) {
            const auto descriptor = analyzer->descriptor();
            if (!triggered(descriptor, events))
                continue;
            auto task = analyzer->analyze(snapshot, events, context);
            auto result = task.get();
            if (!result)
                return std::unexpected(result.error());
            report.executed_analyzers.push_back(descriptor.stable_id);
            report.commands.insert(report.commands.end(), result->commands.begin(), result->commands.end());
            report.diagnostics.insert(report.diagnostics.end(), result->diagnostics.begin(), result->diagnostics.end());
            report.affected_entities.insert(report.affected_entities.end(), result->affected_entities.begin(),
                                            result->affected_entities.end());
        }
        return report;
    }

private:
    /// Tests event-trigger intersection, with project analyzers running on an empty initial batch.
    [[nodiscard]] static bool triggered(const core::contracts::AnalyzerDescriptor& descriptor,
                                        const core::events::EventBatch& events) {
        if (descriptor.scope == core::contracts::AnalysisScope::project && events.events.empty())
            return true;
        return std::ranges::any_of(events.events,
                                   [&](const auto& event) { return descriptor.triggers.contains(event.event_type); });
    }

    std::shared_ptr<AnalyzerRegistry> registry_;
    std::shared_ptr<workers::WorkerPool> workers_;
};

} // namespace ghidra::runtime::analysis
