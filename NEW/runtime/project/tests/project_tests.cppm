module;

#include <gtest/gtest.h>

export module ghidra.runtime.project.tests;

import ghidra.core;
import ghidra.runtime.api.project;
import ghidra.runtime.project.config;
import ghidra.runtime.project.runtime_core;
import ghidra.runtime.workers.pool;
import std;

namespace ghidra::runtime::project::tests {
namespace {

/// Creates a deterministic copy-on-test project path under the platform temp directory.
[[nodiscard]] std::filesystem::path test_directory() {
    const auto path = std::filesystem::temp_directory_path() / "new-ghidra-end-to-end-project";
    std::error_code error;
    std::filesystem::remove_all(path, error);
    std::filesystem::create_directories(path, error);
    return path;
}

/// Exercises opening, PE loading, event-backed listing materialization, analysis, and native decompilation.
TEST(ProjectRuntimeTest, RealExecutableCompletesNativePipeline) {
    const auto fixture = std::filesystem::path(NEW_GHIDRA_PROJECT_FIXTURE_DIR) / "services" / "analyzers" / "tests" /
                         "data" / "test_analyzers_integration.exe";
    const auto specification = std::filesystem::path(NEW_GHIDRA_PROJECT_FIXTURE_DIR) / "services" / "sleigh" /
                               "specifications" / "x86-64.sla";
    ProjectConfig config;
    config.id = core::ProjectId{"end-to-end-project"};
    config.directory = test_directory();
    config.primary_artifact = core::BinaryArtifact{
        core::ArtifactId{"primary"}, "test.exe", "PE", fixture.string(), 0, "fixture", "x86-64", true};
    config.sleigh_specification = specification;
    std::shared_ptr<RuntimeCore> runtime;
    try {
        runtime = std::make_shared<RuntimeCore>(RuntimeConfig{ghidra::runtime::workers::WorkerPoolConfig{2, 64}});
    } catch (const std::exception& error) {
        FAIL() << "runtime construction threw: " << error.what();
        return;
    } catch (...) {
        FAIL() << "runtime construction threw an unknown exception";
        return;
    }
    core::Result<std::shared_ptr<ghidra::runtime::api::ProjectFacade>> facade;
    try {
        facade = ghidra::runtime::api::ProjectFacade::open(runtime, config);
    } catch (const std::exception& error) {
        FAIL() << "open threw: " << error.what();
        return;
    } catch (...) {
        FAIL() << "open threw an unknown exception";
        return;
    }
    ASSERT_TRUE(facade) << (facade ? "" : facade.error().message);
    core::Result<LoadSummary> loaded;
    try {
        loaded = (*facade)->load();
    } catch (const std::exception& error) {
        FAIL() << "load threw: " << error.what();
    } catch (...) {
        FAIL() << "load threw an unknown exception";
        return;
    }
    if (!loaded) {
        ADD_FAILURE() << loaded.error().message;
        return;
    }
    EXPECT_GT(loaded->memory_regions, 0U);
    EXPECT_GT(loaded->decoded_instructions, 0U);
    core::Result<AnalysisSummary> analyzed;
    try {
        analyzed = (*facade)->analyze();
    } catch (const std::exception& error) {
        FAIL() << "analyze threw: " << error.what();
        return;
    } catch (...) {
        FAIL() << "analyze threw an unknown exception";
        return;
    }
    if (!analyzed) {
        ADD_FAILURE() << analyzed.error().message;
        return;
    }
    const auto functions = (*facade)->query()->functions();
    ASSERT_FALSE(functions.empty());
    const auto revision_before_rename = (*facade)->state().revision;
    const core::contracts::CommandRequest rename{
        core::CommandId{"rename-command"},
        config.id,
        core::CorrelationId{"rename-correlation"},
        std::nullopt,
        revision_before_rename,
        core::contracts::ExecutionMode::inline_mode,
        core::contracts::WorkPriority::interactive,
        core::contracts::RenameFunction{functions.front().key, "entry_renamed", "user"}};
    const auto renamed = (*facade)->execute(rename);
    ASSERT_TRUE(renamed) << (renamed ? "" : renamed.error().message);
    ASSERT_TRUE((*facade)->query()->function_at(functions.front().key.entry));
    EXPECT_EQ((*facade)->query()->function_at(functions.front().key.entry)->name, "entry_renamed");
    core::contracts::Task<core::Result<core::Decompilation>> decompiled;
    try {
        decompiled = (*facade)->decompile(functions.front().key);
    } catch (const std::exception& error) {
        FAIL() << "decompile submit threw: " << error.what();
        return;
    } catch (...) {
        FAIL() << "decompile submit threw an unknown exception";
        return;
    }
    core::Result<core::Decompilation> result;
    try {
        result = decompiled.get();
    } catch (const std::exception& error) {
        FAIL() << "decompile task threw: " << error.what();
        return;
    } catch (...) {
        FAIL() << "decompile task threw an unknown exception";
        return;
    }
    if (!result) {
        ADD_FAILURE() << result.error().message;
        return;
    }
    EXPECT_EQ(result->status, core::DecompilationStatus::complete);
    EXPECT_FALSE(result->c_source.empty());
    (*facade)->close();
    runtime->shutdown();
    std::error_code error;
    std::filesystem::remove_all(config.directory, error);
}

} // namespace
} // namespace ghidra::runtime::project::tests
