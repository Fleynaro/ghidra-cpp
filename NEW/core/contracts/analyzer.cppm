export module recode.core.contracts.analyzer;

import std;
import recode.core.architecture;
import recode.core.binary;
import recode.core.diagnostics;
import recode.core.events.event;
import recode.core.identifiers;
import recode.core.contracts.memory_provider;
import recode.core.contracts.operation;
import recode.core.contracts.pcode_decoder;
import recode.core.contracts.project_query;

export namespace recode::core::contracts {

/// Names event kinds that can dirty an analyzer.
using EventTriggerSet = std::set<std::string>;

/// Limits an analyzer to one entity, range, or complete project scope.
enum class AnalysisScope : std::uint8_t { entity, range, project };
/// Describes whether a run is repeatable or one-time.
enum class RunPolicy : std::uint8_t { incremental, one_time, repeatable };
/// Selects whether mutations are serialized by the project lane.
enum class ExecutionClass : std::uint8_t { serial_mutating, read_only };

/// Describes registration, ordering, and dependency semantics for one analyzer.
struct AnalyzerDescriptor {
    std::string stable_id;
    std::string display_name;
    std::int32_t priority{};
    EventTriggerSet triggers;
    std::vector<std::string> prerequisites;
    ExecutionMode preferred_mode{ExecutionMode::queued};
    AnalysisScope scope{AnalysisScope::project};
    RunPolicy policy{RunPolicy::incremental};
    ExecutionClass execution_class{ExecutionClass::serial_mutating};
    bool supports_removals{};
    bool mutates_project{true};
};

/// Owns the read-only providers captured at one project revision.
struct AnalysisSnapshot {
    ProjectId project;
    Revision revision;
    AnalysisScope scope{AnalysisScope::project};
    ResourceSetIdentity resources;
    std::shared_ptr<const ArchitectureDescription> architecture;
    std::shared_ptr<const IProjectQuery> query;
    std::shared_ptr<const IMemoryProvider> memory;
    std::shared_ptr<const IPCodeDecoder> decoder;
};

/// Describes one mutation proposed after read-only analysis.
struct MutationCommand {
    std::string event_type;
    std::string aggregate_kind;
    std::string aggregate_id;
    std::string source_service;
    std::string payload;
};

/// Carries commands and diagnostics back to the runtime commit lane.
struct AnalyzerResult {
    Revision read_revision;
    std::vector<MutationCommand> commands;
    std::vector<Diagnostic> diagnostics;
    std::vector<EntityId> affected_entities;
};

/// Implements one analyzer's business behavior without owning scheduler state.
class IAnalyzer {
public:
    /// Releases an analyzer through its contract.
    virtual ~IAnalyzer() = default;

    /// Returns immutable scheduler metadata.
    [[nodiscard]] virtual AnalyzerDescriptor descriptor() const = 0;

    /// Performs read-only work and returns typed mutation proposals.
    [[nodiscard]] virtual Task<Result<AnalyzerResult>>
    analyze(const AnalysisSnapshot& snapshot, events::EventBatch event_batch, OperationContext context) = 0;
};

} // namespace recode::core::contracts
