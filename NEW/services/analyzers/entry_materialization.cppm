export module recode.service.analyzers.entry_materialization;

import std;
import recode.core;

export namespace recode::services::analyzers {

namespace core = recode::core;

/// Verifies that loader/disassembler materialization produced a coherent entry function.
class EntryMaterializationAnalyzer final : public core::contracts::IAnalyzer {
public:
    /// Returns scheduler metadata for the entry materialization invariant.
    [[nodiscard]] core::contracts::AnalyzerDescriptor descriptor() const override {
        return core::contracts::AnalyzerDescriptor{
            "runtime.entry_materialization",
            "Entry Materialization",
            0,
            {"MemoryStateChanged", "ListingStateChanged", "FunctionStateChanged"},
            {},
            core::contracts::ExecutionMode::inline_mode,
            core::contracts::AnalysisScope::project,
            core::contracts::RunPolicy::incremental,
            core::contracts::ExecutionClass::read_only,
            false,
            false};
    }

    /// Checks current functions/instructions without proposing hidden mutations.
    [[nodiscard]] core::contracts::Task<core::Result<core::contracts::AnalyzerResult>>
    analyze(const core::contracts::AnalysisSnapshot& snapshot, core::events::EventBatch,
            core::contracts::OperationContext context) override {
        auto promise = std::make_shared<std::promise<core::Result<core::contracts::AnalyzerResult>>>();
        auto future = promise->get_future().share();
        if (context.cancellation.stop_requested())
            promise->set_value(std::unexpected(
                core::Error::make(core::DiagnosticCode::cancelled, "Entry materialization analysis was cancelled")));
        else {
            core::contracts::AnalyzerResult result{snapshot.revision};
            const auto functions = snapshot.query ? snapshot.query->functions() : std::vector<core::FunctionSnapshot>{};
            const auto instructions =
                snapshot.query ? snapshot.query->instructions() : std::vector<core::Instruction>{};
            if (functions.empty() || instructions.empty())
                result.diagnostics.push_back(
                    core::Diagnostic{core::Severity::warning,
                                     core::DiagnosticCode::parse_failure,
                                     "No materialized entry function or instructions are available",
                                     {},
                                     "Load a primary binary before running analysis."});
            for (const auto& function : functions)
                result.affected_entities.push_back(function.key.entity);
            promise->set_value(std::move(result));
        }
        return core::contracts::Task<core::Result<core::contracts::AnalyzerResult>>{
            std::move(future), std::make_shared<core::contracts::OperationControl>()};
    }
};

} // namespace recode::services::analyzers
