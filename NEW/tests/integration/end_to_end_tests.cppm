module;

#include <gtest/gtest.h>

export module recode.tests.integration;

import recode.core;
import recode.runtime.api.project;
import recode.runtime.project.config;
import recode.runtime.project.runtime_core;
import recode.runtime.workers.pool;
import std;

namespace recode::tests::integration {
namespace {

namespace api = recode::runtime::api;
namespace project = recode::runtime::project;
namespace workers = recode::runtime::workers;

/// Runs the real executable through the project facade and verifies the cross-service pipeline.
TEST(ArchitectureIntegrationTest, AnalyzerFixtureLoadsAnalyzesAndDecompiles) {
    const auto root = std::filesystem::path(RECODE_INTEGRATION_ROOT);
    const auto fixture = root / "services" / "analyzers" / "tests" / "data" / "test_analyzers_integration.exe";
    const auto sla = root / "services" / "sleigh" / "specifications" / "x86-64.sla";
    const auto directory = std::filesystem::temp_directory_path() / "recode-architecture-integration";
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
} // namespace recode::tests::integration
