module;

#include <gtest/gtest.h>

export module windows_resource_reference_tests;

import analyzer_disassemble_entry_points;
import analyzer_reference;
import analyzer_windows_resource_reference;
import analyzer_test_support;
import std;

namespace ghidra::analyzer::tests {
namespace {

/// Verifies string-table resource IDs resolve to the individual UTF-16 payload entries.
TEST(WindowsResourceReferenceAnalyzerTest, ResolvesStringTableReferences) {
    auto context = load_fixture("windows_resource_reference");
    context.options().seed_provider_functions = false;
    const auto entry = context.image().entry_point_va();
    ASSERT_TRUE(entry.has_value());
    ASSERT_GT(context.disassemble_flow(*entry), 0U);
    CancellationToken cancellation;
    ReferenceAnalyzer references;
    references.analyze(context, {}, cancellation);
    WindowsResourceReferenceAnalyzer analyzer;
    analyzer.analyze(context, {}, cancellation);

    const auto resource_references =
        std::count_if(context.references().begin(), context.references().end(), [](const Reference& reference) {
            return reference.kind == ReferenceKind::data && reference.analysis_source &&
                   (reference.target == 0x1400051d8ULL || reference.target == 0x1400051daULL);
        });
    EXPECT_EQ(resource_references, 2U);
    EXPECT_TRUE(std::any_of(context.references().begin(), context.references().end(),
                            [](const Reference& reference) { return reference.target == 0x1400051d8ULL; }));
    EXPECT_TRUE(std::any_of(context.references().begin(), context.references().end(),
                            [](const Reference& reference) { return reference.target == 0x1400051daULL; }));
}

/// Verifies the one-time analyzer remains disabled when its option is false.
TEST(WindowsResourceReferenceAnalyzerTest, HonorsDisableOption) {
    auto context = load_fixture("windows_resource_reference");
    context.options().windows_resource_reference = false;
    CancellationToken cancellation;
    WindowsResourceReferenceAnalyzer analyzer;
    analyzer.analyze(context, {}, cancellation);
    EXPECT_TRUE(context.references().empty());
}

} // namespace
} // namespace ghidra::analyzer::tests
