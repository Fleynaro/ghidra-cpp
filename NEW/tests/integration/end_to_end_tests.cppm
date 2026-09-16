module;

#include <gtest/gtest.h>

export module ghidra.tests.integration;

import ghidra.core;
import ghidra.runtime.api.project;
import ghidra.runtime.project.config;
import ghidra.runtime.project.runtime_core;
import ghidra.runtime.workers.pool;
import std;

namespace ghidra::tests::integration {
namespace {

namespace api = ghidra::runtime::api;
namespace project = ghidra::runtime::project;
namespace workers = ghidra::runtime::workers;

/// Runs the real executable through the project facade and verifies the cross-service pipeline.
TEST(ArchitectureIntegrationTest, AnalyzerFixtureLoadsAnalyzesAndDecompiles) {
    const auto root = std::filesystem::path(NEW_GHIDRA_INTEGRATION_ROOT);
    const auto fixture = root / "services" / "analyzers" / "tests" / "data" / "test_analyzers_integration.exe";
    const auto sla = root / "services" / "sleigh" / "specifications" / "x86-64.sla";
    const auto directory = std::filesystem::temp_directory_path() / "new-ghidra-architecture-integration";
    std::error_code error;
    std::filesystem::remove_all(directory, error);
    project::ProjectConfig config;
    config.id = core::ProjectId{"architecture-integration"};
    config.directory = directory;
    config.primary_artifact = core::BinaryArtifact{
        core::ArtifactId{"integration-input"}, "fixture.exe", "PE", fixture.string(), 0, "fixture", "x86-64", true};
    config.sleigh_specification = sla;
    auto runtime = std::make_shared<project::RuntimeCore>(project::RuntimeConfig{workers::WorkerPoolConfig{2, 64}});
    auto facade = api::ProjectFacade::open(runtime, config);
    ASSERT_TRUE(facade) << (facade ? "" : facade.error().message);
    const auto loaded = (*facade)->load();
    ASSERT_TRUE(loaded) << (loaded ? "" : loaded.error().message);
    const auto analysis = (*facade)->analyze();
    ASSERT_TRUE(analysis) << (analysis ? "" : analysis.error().message);
    const auto functions = (*facade)->query()->functions();
    ASSERT_FALSE(functions.empty());
    const auto decompilation = (*facade)->decompile(functions.front().key).get();
    ASSERT_TRUE(decompilation) << (decompilation ? "" : decompilation.error().message);
    EXPECT_FALSE(decompilation->c_source.empty());
    (*facade)->close();
    runtime->shutdown();
    std::filesystem::remove_all(directory, error);
}

} // namespace
} // namespace ghidra::tests::integration
