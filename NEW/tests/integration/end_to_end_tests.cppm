module;

#include <gtest/gtest.h>

export module recode.tests.integration;

import recode.tests.integration.report;
import std;

namespace recode::tests::integration {
namespace {

/// Delegates the complete fixture pipeline, bounded report generation, and
/// golden comparison to the reusable integration report module.
TEST(ArchitectureIntegrationTest, FullPeRuntimePipelineProducesProjectionAndReport) {
    const auto root = std::filesystem::path(RECODE_INTEGRATION_ROOT);
    const auto report = std::filesystem::path(RECODE_INTEGRATION_REPORT_PATH);
    const auto golden = std::filesystem::path(RECODE_INTEGRATION_GOLDEN_REPORT_PATH);
    const auto result = report::run_full_pipeline_report(root, report, golden);
    ASSERT_TRUE(result) << (result ? "" : result.error());
}

} // namespace
} // namespace recode::tests::integration
