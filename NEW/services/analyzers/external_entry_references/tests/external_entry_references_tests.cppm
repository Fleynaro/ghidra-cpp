module;

#include <gtest/gtest.h>

export module external_entry_references_tests;

import analyzer_external_entry_references;
import analyzer_test_support;
import std;

namespace recode::analyzer::tests {
namespace {

/// Verifies all fixture export entries are marked while a fall-through entry is not made a function.
TEST(ExternalEntryReferencesAnalyzerTest, CreatesOnlyGoodExportFunctionStarts) {
    auto context = load_fixture("external_entry_references");
    for (const auto& exported : context.image().exported_symbols()) {
        if (!exported.forwarded && context.image().is_executable(exported.address_va)) {
            static_cast<void>(context.disassemble_flow(exported.address_va));
        }
    }
    CancellationToken cancellation;
    ExternalEntryReferencesAnalyzer analyzer;
    analyzer.analyze(context, {}, cancellation);

    // The native PE loader exposes three named executable exports; the entry point is one of them.
    EXPECT_EQ(context.external_entries().size(), 3U);
    EXPECT_NE(context.function_at(0x140001000ULL), nullptr);
    EXPECT_NE(context.function_at(0x140001014ULL), nullptr);
    EXPECT_NE(context.function_at(0x140001028ULL), nullptr);
    EXPECT_EQ(context.function_at(0x140001054ULL), nullptr);
}

/// Verifies the original analyzer identity and priority are stable for scheduler integration.
TEST(ExternalEntryReferencesAnalyzerTest, PreservesAnalyzerDescriptor) {
    const auto descriptor = ExternalEntryReferencesAnalyzer{}.descriptor();
    EXPECT_EQ(descriptor.name, "External Entry References");
    EXPECT_EQ(descriptor.priority, 398);
}

} // namespace
} // namespace recode::analyzer::tests
