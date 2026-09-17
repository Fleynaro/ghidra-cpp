module;

#if defined(RECODE_HAS_TTD_REPLAY)
#include <TTD/ErrorReporting.h>
#include <TTD/IReplayEngineStl.h>
#endif

export module recode.service.debugger.win_ttd.analysis;

import std;
import recode.core;

// Microsoft reference: TEST/debugger/TTD/ReplayApi/TraceAnalysis/TraceAnalysis.cpp.
// The implementation follows its TLS -> continuity queue -> progress merge model.

export namespace recode::services::debugger::win_ttd::analysis {

namespace core = recode::core;
namespace api = recode::core::contracts;
namespace model = recode::core;

namespace detail {

/// Creates a diagnostic for a native bulk-analysis failure.
[[nodiscard]] inline core::Error analysis_error(std::string_view operation, std::string detail) {
    return core::Error::make(core::DiagnosticCode::resource_unavailable,
                             "Windows TTD trace analysis " + std::string(operation) + " failed: " + std::move(detail),
                             "Verify the .run/.idx trace, Replay runtime DLLs, and function catalog.");
}

/// Creates the diagnostic used when native Replay is unavailable in this build.
[[nodiscard]] inline core::Error unsupported() {
    return core::Error::make(core::DiagnosticCode::unsupported,
                             "Windows TTD bulk analysis is unavailable in this build",
                             "Build with Microsoft.TimeTravelDebugging.Apis and TTDReplay runtime DLLs.");
}

/// Normalizes a Windows module path to a case-insensitive file identity.
[[nodiscard]] inline std::string module_identity(std::wstring_view value) {
    auto name = std::filesystem::path{std::wstring(value)}.filename().string();
    for (char& character : name)
        if (character >= 'A' && character <= 'Z')
            character = static_cast<char>(character - 'A' + 'a');
    return name;
}

/// Normalizes a portable module name using the same Windows filename rule.
[[nodiscard]] inline std::string module_identity(std::string value) {
    auto name = std::filesystem::path{std::move(value)}.filename().string();
    for (char& character : name)
        if (character >= 'A' && character <= 'Z')
            character = static_cast<char>(character - 'A' + 'a');
    return name;
}

#if defined(RECODE_HAS_TTD_REPLAY)

/// Stores the native Replay diagnostic for the entire engine lifetime.
class ErrorReporting final : public TTD::ErrorReporting {
public:
    /// Captures one native diagnostic without throwing through a callback boundary.
    void __fastcall VPrintError(char const* const format, va_list arguments) noexcept override {
        char buffer[2048]{};
        std::vsnprintf(buffer, sizeof(buffer), format, arguments);
        message_ = buffer;
    }

    /// Returns the latest captured diagnostic.
    [[nodiscard]] std::string message() const {
        return message_;
    }

private:
    std::string message_;
};

/// Maps one module-relative catalog entry to its loaded trace address range.
struct ResolvedFunction {
    std::size_t catalog_index{};
    std::uint64_t absolute_entry{};
};

/// Describes one loaded module needed to resolve callback instruction addresses.
struct ModuleRange {
    std::string identity;
    std::uint64_t base{};
    std::uint64_t size{};
    std::vector<ResolvedFunction> functions;
};

/// Holds one scheduler segment's local counts before the progress merge.
struct SegmentData {
    std::uint64_t end_sequence{};
    std::uint64_t end_steps{};
    std::uint64_t executed_instructions{};
    std::unordered_map<std::size_t, std::uint64_t> counts;
};

/// Owns all mutable state shared by callbacks during one bulk analysis pass.
struct AnalysisRun {
    std::vector<model::TraceFunction> catalog;
    std::vector<ModuleRange> modules;
    std::mutex completed_mutex;
    std::vector<SegmentData> completed_segments;
    std::unordered_map<std::size_t, std::uint64_t> aggregate;
    std::uint64_t unresolved_entries{};
    std::uint64_t executed_instructions{};
    std::uint64_t analyzed_segments{};
    std::chrono::nanoseconds aggregation_time{};
    std::chrono::nanoseconds trace_open_time{};
    std::chrono::nanoseconds replay_time{};
    api::CancellationToken cancellation;
};

/// Keeps hot callback data local to the native replay worker thread.
struct SegmentLocalState {
    AnalysisRun* owner{};
    std::uint64_t end_sequence{};
    std::uint64_t end_steps{};
    std::uint64_t executed_instructions{};
    std::unordered_map<std::size_t, std::uint64_t> counts;
};

thread_local SegmentLocalState segment_local;

/// Resolves an executed address to a catalog entry without touching the cursor.
[[nodiscard]] inline std::optional<std::size_t> resolve_function(const AnalysisRun& run,
                                                                 std::uint64_t address) noexcept {
    for (const auto& module : run.modules) {
        if (address < module.base || address >= module.base + module.size)
            continue;
        const auto found = std::ranges::find(module.functions, address,
                                             [](const ResolvedFunction& value) { return value.absolute_entry; });
        if (found != module.functions.end())
            return found->catalog_index;
    }
    return std::nullopt;
}

/// Records one executed function entry in TLS and never synchronizes the hot path.
bool __fastcall execution_callback(uintptr_t context, TTD::Replay::ICursorView::MemoryWatchpointResult const& result,
                                   TTD::Replay::IThreadView const* thread_view) noexcept {
    try {
        auto& run = *reinterpret_cast<AnalysisRun*>(context);
        if (thread_view == nullptr)
            return false;
        if (segment_local.owner != &run) {
            segment_local = {};
            segment_local.owner = &run;
        }
        segment_local.end_sequence = static_cast<std::uint64_t>(thread_view->GetPosition().Sequence);
        segment_local.end_steps = static_cast<std::uint64_t>(thread_view->GetPosition().Steps);
        ++segment_local.executed_instructions;
        if (const auto function = resolve_function(run, static_cast<std::uint64_t>(result.Address));
            function.has_value())
            ++segment_local.counts[*function];
        return false;
    } catch (...) {
        return false;
    }
}

/// Flushes one scheduler segment's TLS state into the completed queue.
void __fastcall continuity_callback(uintptr_t context) noexcept {
    try {
        auto& run = *reinterpret_cast<AnalysisRun*>(context);
        if (segment_local.owner != &run)
            return;
        SegmentData completed{segment_local.end_sequence, segment_local.end_steps, segment_local.executed_instructions,
                              std::move(segment_local.counts)};
        segment_local = {};
        std::scoped_lock lock(run.completed_mutex);
        run.completed_segments.push_back(std::move(completed));
    } catch (...) {
    }
}

/// Merges completed segment maps outside the queue lock at a scheduler barrier.
void merge_completed(AnalysisRun& run, TTD::Replay::Position const&) noexcept {
    try {
        const auto started = std::chrono::steady_clock::now();
        std::vector<SegmentData> ready;
        {
            std::scoped_lock lock(run.completed_mutex);
            ready.swap(run.completed_segments);
        }
        for (auto& segment : ready) {
            ++run.analyzed_segments;
            run.executed_instructions += segment.executed_instructions;
            for (const auto& [catalog_index, count] : segment.counts)
                run.aggregate[catalog_index] += count;
        }
        run.aggregation_time +=
            std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - started);
    } catch (...) {
    }
}

/// Applies the progress barrier while allowing cancellation to stop future replay.
void __stdcall progress_callback(uintptr_t context, TTD::Replay::Position const& position) noexcept {
    auto& run = *reinterpret_cast<AnalysisRun*>(context);
    merge_completed(run, position);
}

/// Creates loaded module ranges and validates every requested module identity.
[[nodiscard]] core::Result<void> resolve_catalog(AnalysisRun& run, TTD::Replay::IReplayEngineView& engine) {
    for (std::size_t index = 0; index < engine.GetModuleCount(); ++index) {
        const auto& instance = engine.GetModuleList()[index];
        const auto name = std::wstring(instance.pName, instance.NameLength);
        run.modules.push_back(ModuleRange{module_identity(name),
                                          static_cast<std::uint64_t>(instance.Address),
                                          static_cast<std::uint64_t>(instance.Size),
                                          {}});
    }
    for (std::size_t function_index = 0; function_index < run.catalog.size(); ++function_index) {
        const auto& function = run.catalog[function_index];
        const auto identity = module_identity(function.module);
        auto module = std::ranges::find(run.modules, identity, [](const ModuleRange& value) { return value.identity; });
        if (module == run.modules.end())
            return std::unexpected(
                core::Error::make(core::DiagnosticCode::invalid_argument,
                                  "Function catalog module is not loaded in the trace: " + function.module));
        if (function.relative_entry >= module->size)
            return std::unexpected(
                core::Error::make(core::DiagnosticCode::invalid_argument,
                                  "Function catalog entry is outside its trace module: " + function.display_name));
        module->functions.push_back(ResolvedFunction{function_index, module->base + function.relative_entry});
    }
    return {};
}

/// Runs one native segment-parallel analysis pass and constructs portable output.
[[nodiscard]] core::Result<model::FunctionCallStatistics> run_analysis(const model::TraceAnalysisRequest& request,
                                                                       const api::CancellationToken& cancellation) {
    if (request.trace.empty() || !std::filesystem::is_regular_file(request.trace))
        return std::unexpected(core::Error::make(core::DiagnosticCode::invalid_argument,
                                                 "Trace analysis requires an existing trace path."));
    if (request.functions.empty())
        return std::unexpected(core::Error::make(core::DiagnosticCode::invalid_argument,
                                                 "Trace analysis requires a non-empty function catalog."));

    AnalysisRun run{.catalog = request.functions, .cancellation = cancellation};
    const auto open_started = std::chrono::steady_clock::now();
    ErrorReporting error_reporting;
    auto [engine, result] = TTD::Replay::MakeReplayEngine();
    if (result != 0 || engine == nullptr)
        return std::unexpected(analysis_error("MakeReplayEngine", std::to_string(result)));
    engine->RegisterDebugModeAndLogging(TTD::Replay::DebugModeType::None, &error_reporting);
    engine->Initialize(request.trace.wstring().c_str());
    run.trace_open_time =
        std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - open_started);
    if (!error_reporting.message().empty())
        return std::unexpected(analysis_error("Initialize", error_reporting.message()));
    if (cancellation.stop_requested())
        return std::unexpected(core::Error::make(core::DiagnosticCode::cancelled, "Trace analysis was cancelled."));
    auto resolved = resolve_catalog(run, *engine);
    if (!resolved)
        return std::unexpected(resolved.error());
    TTD::Replay::UniqueCursor cursor{engine->NewCursor()};
    if (!cursor)
        return std::unexpected(analysis_error("NewCursor", "the engine returned a null cursor"));
    cursor->SetThreadContinuityBreakCallback(continuity_callback, reinterpret_cast<uintptr_t>(&run));
    cursor->SetReplayProgressCallback(progress_callback, reinterpret_cast<uintptr_t>(&run));
    cursor->SetReplayFlags(TTD::Replay::ReplayFlags::ReplayAllSegmentsWithoutFiltering);
    if (!cursor->AddMemoryWatchpoint(TTD::Replay::MemoryWatchpointData{
            .Address = TTD::GuestAddress{0}, .Size = UINT64_MAX, .AccessMask = TTD::Replay::DataAccessMask::Execute}))
        return std::unexpected(analysis_error("AddMemoryWatchpoint", "the execute watchpoint was rejected"));
    cursor->SetMemoryWatchpointCallback(execution_callback, reinterpret_cast<uintptr_t>(&run));
    cursor->SetEventMask(TTD::Replay::EventMask::MemoryWatchpoint);
    cursor->SetPosition(TTD::Replay::Position::Min);
    const auto replay_started = std::chrono::steady_clock::now();
    const auto replay_result = cursor->ReplayForward();
    run.replay_time =
        std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - replay_started);
    merge_completed(run, engine->GetLastPosition());
    if (replay_result.StopReason == TTD::Replay::EventType::Error)
        return std::unexpected(analysis_error("ReplayForward", error_reporting.message()));

    model::FunctionCallStatistics statistics;
    statistics.unresolved_entries = run.unresolved_entries;
    statistics.executed_instructions = run.executed_instructions;
    statistics.analyzed_segments = run.analyzed_segments;
    statistics.timings = {run.trace_open_time, run.replay_time, run.aggregation_time};
    for (const auto& [catalog_index, count] : run.aggregate)
        if (count != 0)
            statistics.functions.push_back({run.catalog[catalog_index], count});
    std::ranges::sort(statistics.functions, [](const auto& left, const auto& right) {
        return std::tie(left.function.module, left.function.relative_entry) <
               std::tie(right.function.module, right.function.relative_entry);
    });
    return statistics;
}

#endif

} // namespace detail

/// Implements one native bulk function-entry mining pass over a TTD trace.
class WinTtdFunctionCallAnalyzer final : public api::ITraceAnalyzer {
public:
    /// Creates a stateless analyzer; each request owns its engine/cursor lifetime.
    WinTtdFunctionCallAnalyzer() = default;
    /// Starts one asynchronous analysis operation owned by its returned Task.
    [[nodiscard]] api::Task<core::Result<model::FunctionCallStatistics>>
    analyze(model::TraceAnalysisRequest request, api::OperationContext context) override {
        auto control = std::make_shared<api::OperationControl>();
        const auto cancellation = control->cancellation();
        auto future = std::async(std::launch::async,
                                 [request = std::move(request), context = std::move(context), cancellation,
                                  control]() mutable -> core::Result<model::FunctionCallStatistics> {
                                     control->set_status(api::OperationStatus::running);
#if !defined(RECODE_HAS_TTD_REPLAY)
                                     static_cast<void>(request);
                                     static_cast<void>(context);
                                     control->set_status(api::OperationStatus::failed);
                                     return std::unexpected(detail::unsupported());
#else
                                     if (context.cancellation.stop_requested() || cancellation.stop_requested()) {
                                         control->set_status(api::OperationStatus::cancelled);
                                         return std::unexpected(core::Error::make(core::DiagnosticCode::cancelled,
                                                                                  "Trace analysis was cancelled."));
                                     }
                                     auto result = detail::run_analysis(request, cancellation);
                                     control->set_status(!result ? api::OperationStatus::failed
                                                                  : api::OperationStatus::completed);
                                     return result;
#endif
                                 });
        return api::Task<core::Result<model::FunctionCallStatistics>>{future.share(), std::move(control)};
    }
};

/// Creates the Windows TTD bulk function analyzer behind the portable contract.
[[nodiscard]] inline core::Result<std::shared_ptr<api::ITraceAnalyzer>> create_win_ttd_function_call_analyzer() {
    return std::shared_ptr<api::ITraceAnalyzer>{std::make_shared<WinTtdFunctionCallAnalyzer>()};
}

} // namespace recode::services::debugger::win_ttd::analysis
