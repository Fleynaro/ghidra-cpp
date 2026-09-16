module;

#include <gtest/gtest.h>

export module apply_data_archives_tests;

import analyzer_apply_data_archives;
import analyzer_test_support;
import std;

namespace ghidra::analyzer::tests {
namespace {

/// Verifies valid GDT selection records the exact path and explicit unsupported-application diagnostic.
TEST(ApplyDataArchivesAnalyzerTest, RecordsValidatedArchiveSelection) {
    auto context = load_fixture("apply_data_archives");
    const auto path = std::filesystem::temp_directory_path() / "native_apply_data_archives_test.gdt";
    {
        std::ofstream output(path, std::ios::binary);
        output << "native fixture archive";
    }
    context.options().apply_data_archives = true;
    context.options().source_language = "x86:LE:64:default";
    context.options().data_archive_paths = {path};
    CancellationToken cancellation;
    ApplyDataArchivesAnalyzer analyzer;
    analyzer.analyze(context, {}, cancellation);
    std::filesystem::remove(path);

    ASSERT_EQ(context.data_archives().size(), 1U);
    EXPECT_EQ(context.data_archives().front().path, path);
    EXPECT_EQ(context.data_archives().front().source_language, "x86:LE:64:default");
    EXPECT_FALSE(context.data_archives().front().applied);
    EXPECT_NE(context.data_archives().front().error.find("DataTypeManagerService"), std::string::npos);
}

/// Verifies the original default-disabled analyzer does not select archives without explicit enablement.
TEST(ApplyDataArchivesAnalyzerTest, PreservesDefaultDisabledOption) {
    auto context = load_fixture("apply_data_archives");
    context.options().data_archive_paths = {std::filesystem::path("missing.gdt")};
    CancellationToken cancellation;
    ApplyDataArchivesAnalyzer analyzer;
    analyzer.analyze(context, {}, cancellation);
    EXPECT_TRUE(context.data_archives().empty());
    EXPECT_FALSE(AnalysisOptions{}.apply_data_archives);
}

} // namespace
} // namespace ghidra::analyzer::tests
