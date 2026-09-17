module;

#include <gtest/gtest.h>

#if defined(_WIN32)
#include <windows.h>
#endif

export module recode.service.debugger.win_ttd.analysis.tests;

import std;
import recode.core;
import recode.service.debugger.win_ttd.analysis;

namespace {

namespace api = recode::core::contracts;
namespace model = recode::core;

/// Loads one exported fixture function and converts its ASLR address to a module RVA.
[[nodiscard]] std::uint64_t fixture_rva(const std::filesystem::path& executable, std::string_view name) {
#if defined(_WIN32)
    const auto module = LoadLibraryExW(executable.wstring().c_str(), nullptr, DONT_RESOLVE_DLL_REFERENCES);
    if (module == nullptr)
        return 0;
    const auto symbol = reinterpret_cast<std::uintptr_t>(GetProcAddress(module, std::string{name}.c_str()));
    const auto base = reinterpret_cast<std::uintptr_t>(module);
    FreeLibrary(module);
    return symbol == 0 || symbol < base ? 0 : static_cast<std::uint64_t>(symbol - base);
#else
    static_cast<void>(executable);
    static_cast<void>(name);
    return 0;
#endif
}

/// Creates the stable module-relative catalog used by every real analysis assertion.
[[nodiscard]] std::vector<model::TraceFunction> fixture_catalog(const std::filesystem::path& executable) {
    constexpr std::array names{
        "debugger_main_entry", "worker_thread",       "worker_nested",        "worker_leaf",
        "compute_value",       "debugger_test_entry", "debugger_ready_break", "trigger_controlled_exception"};
    std::vector<model::TraceFunction> catalog;
    for (const auto name : names)
        catalog.push_back({executable.filename().string(), fixture_rva(executable, name), std::string{name}});
    return catalog;
}

/// Verifies one real bulk segment analysis returns deterministic fixture call counts.
TEST(WinTtdFunctionAnalysis, CountsFixtureFunctionEntries) {
#if !defined(RECODE_HAS_TTD_REPLAY)
    GTEST_SKIP() << "Microsoft TTD Replay API is unavailable";
#else
    const char* trace_name = std::getenv("TTD_TEST_TRACE");
    const char* program_name = std::getenv("TTD_ANALYSIS_PROGRAM");
    if (trace_name == nullptr || program_name == nullptr || *trace_name == '\0' || *program_name == '\0')
        GTEST_SKIP() << "Run \build.bat ttd_all or set TTD_TEST_TRACE and TTD_ANALYSIS_PROGRAM";
    const std::filesystem::path trace{trace_name};
    const std::filesystem::path program{program_name};
    ASSERT_TRUE(std::filesystem::is_regular_file(trace));
    ASSERT_TRUE(std::filesystem::is_regular_file(program));
    const auto catalog = fixture_catalog(program);
    for (const auto& function : catalog)
        ASSERT_NE(function.relative_entry, 0U) << function.display_name;
    const auto analyzer = recode::services::debugger::win_ttd::analysis::create_win_ttd_function_call_analyzer();
    ASSERT_TRUE(analyzer) << analyzer.error().message;
    const auto started = std::chrono::steady_clock::now();
    auto task = (*analyzer)->analyze(model::TraceAnalysisRequest{trace, catalog}, {});
    const auto result = task.get();
    const auto elapsed =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - started);
    ASSERT_TRUE(result) << result.error().message;
    std::cout << "analysis_wall_ms=" << elapsed.count() << " trace_open_ns=" << result->timings.trace_open.count()
              << " replay_ns=" << result->timings.replay.count()
              << " aggregation_ns=" << result->timings.aggregation.count() << '\n';
    const auto count_for = [&](std::string_view name) {
        const auto found = std::ranges::find_if(result->functions, [&](const model::FunctionCallCount& count) {
            return count.function.display_name == name;
        });
        return found == result->functions.end() ? std::uint64_t{} : found->call_count;
    };
    EXPECT_EQ(count_for("debugger_main_entry"), 1U);
    EXPECT_EQ(count_for("worker_thread"), 3U);
    EXPECT_EQ(count_for("worker_nested"), 7U);
    EXPECT_EQ(count_for("worker_leaf"), 21U);
    EXPECT_EQ(count_for("compute_value"), 4U);
    EXPECT_EQ(count_for("debugger_test_entry"), 1U);
    // --auto-exit intentionally skips DebugBreak so this live-test marker is not executed.
    EXPECT_EQ(count_for("debugger_ready_break"), 0U);
    EXPECT_EQ(count_for("trigger_controlled_exception"), 0U);
    EXPECT_GT(result->executed_instructions, 0U);
    EXPECT_GT(result->analyzed_segments, 0U);
#endif
}

/// Verifies unlisted fixture functions are not fabricated as call-stat entries.
TEST(WinTtdFunctionAnalysis, DoesNotInventUncataloguedFunctions) {
#if !defined(RECODE_HAS_TTD_REPLAY)
    GTEST_SKIP() << "Microsoft TTD Replay API is unavailable";
#else
    const char* trace_name = std::getenv("TTD_TEST_TRACE");
    const char* program_name = std::getenv("TTD_ANALYSIS_PROGRAM");
    if (trace_name == nullptr || program_name == nullptr || *trace_name == '\0' || *program_name == '\0')
        GTEST_SKIP() << "Run \build.bat ttd_all or set TTD_TEST_TRACE and TTD_ANALYSIS_PROGRAM";
    const auto analyzer = recode::services::debugger::win_ttd::analysis::create_win_ttd_function_call_analyzer();
    ASSERT_TRUE(analyzer);
    const auto catalog = fixture_catalog(std::filesystem::path{program_name});
    const auto result = (*analyzer)->analyze({std::filesystem::path{trace_name}, catalog}, {}).get();
    ASSERT_TRUE(result) << result.error().message;
    EXPECT_TRUE(std::ranges::none_of(result->functions, [](const model::FunctionCallCount& count) {
        return count.function.display_name == "RaiseException";
    }));
#endif
}

} // namespace
