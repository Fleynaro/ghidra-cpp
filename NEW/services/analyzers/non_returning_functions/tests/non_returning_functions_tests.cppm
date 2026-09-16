module;

#include <gtest/gtest.h>

export module non_returning_functions_tests;

import analyzer_disassemble_entry_points;
import analyzer_non_returning_functions;
import analyzer_subroutine_references;
import analyzer_test_support;
import std;

namespace ghidra::analyzer::tests {
namespace {

/// Verifies known PE no-return names create the target function and bookmark.
TEST(AnalyzerPipelineTest, KnownNoReturnFunctionsAreMarked) {
    auto context = load_fixture("non_returning_functions_known");
    auto& options = context.options();
    options.function_start_search = false;
    options.subroutine_references = false;
    options.reference = false;
    options.data_reference = false;
    options.scalar_operand_references = false;
    options.stack = false;
    options.constant_propagation = false;
    options.known_non_returning_functions = true;
    options.discovered_non_returning_functions = false;
    AutoAnalysisManager manager(context);
    manager.register_analyzer(std::make_unique<DisassembleEntryPointsAnalyzer>());
    manager.register_analyzer(std::make_unique<SubroutineReferencesAnalyzer>());
    manager.register_analyzer(std::make_unique<KnownNoReturnFunctionsAnalyzer>());
    const auto result = manager.analyze();
    ASSERT_TRUE(result.completed);
    // Copied from the known-no-return Ghidra Delta: both exact and normalized
    // underscore spellings are marked, while the entry remains returning.
    ASSERT_TRUE(context.functions().contains(0x140001000));
    EXPECT_TRUE(context.functions().at(0x140001000).no_return);
    ASSERT_TRUE(context.functions().contains(0x140001014));
    EXPECT_TRUE(context.functions().at(0x140001014).no_return);
    ASSERT_EQ(std::count_if(context.bookmarks().begin(), context.bookmarks().end(),
                            [](const Bookmark& bookmark) { return bookmark.category == "Non-Returning Function"; }),
              2U);
    EXPECT_TRUE(std::all_of(context.functions().begin(), context.functions().end(), [](const auto& pair) {
        return (pair.first == 0x140001000 || pair.first == 0x140001014) || !pair.second.no_return;
    }));
}

/// Verifies three independent post-call indicators mark a discovered no-return target.
TEST(AnalyzerPipelineTest, DiscoversNoReturnFromCallEvidence) {
    auto context = load_fixture("non_returning_functions_discovered");
    auto& options = context.options();
    options.function_start_search = false;
    options.subroutine_references = true;
    options.reference = false;
    options.data_reference = false;
    options.scalar_operand_references = false;
    options.stack = false;
    options.constant_propagation = false;
    options.known_non_returning_functions = false;
    options.discovered_non_returning_functions = true;
    AutoAnalysisManager manager(context);
    manager.register_analyzer(std::make_unique<DisassembleEntryPointsAnalyzer>());
    manager.register_analyzer(std::make_unique<SubroutineReferencesAnalyzer>());
    manager.register_analyzer(std::make_unique<NonReturningFunctionsAnalyzer>());
    const auto result = manager.analyze();
    ASSERT_TRUE(result.completed);
    // Copied from the discovered-no-return Ghidra Delta: four call sites to
    // the marked target are repaired, although only three carry INT3 evidence.
    ASSERT_TRUE(context.functions().contains(0x140001000));
    EXPECT_TRUE(context.functions().at(0x140001000).no_return);
    EXPECT_FALSE(context.functions().at(0x140001080).no_return);
    EXPECT_EQ(
        std::count_if(context.references().begin(), context.references().end(),
                      [](const Reference& reference) { return reference.flow_override == FlowOverride::call_return; }),
        4);
    EXPECT_TRUE(std::all_of(context.references().begin(), context.references().end(), [](const Reference& reference) {
        return reference.target != 0x140001000 || reference.flow_override == FlowOverride::call_return;
    }));
}

/// Verifies the discovered phase respects the independent known-name option
/// instead of silently marking known symbols when discovery is enabled.
TEST(AnalyzerPipelineTest, KnownNoReturnOptionIsIndependent) {
    auto context = load_fixture("non_returning_functions_known");
    context.options() = {};
    context.options().known_non_returning_functions = false;
    context.options().discovered_non_returning_functions = true;
    AutoAnalysisManager manager(context);
    manager.register_analyzer(std::make_unique<KnownNoReturnFunctionsAnalyzer>());
    manager.register_analyzer(std::make_unique<NonReturningFunctionsAnalyzer>());
    const auto result = manager.analyze();
    ASSERT_TRUE(result.completed);
    EXPECT_FALSE(context.functions().contains(0x140001000));
    EXPECT_TRUE(std::none_of(context.bookmarks().begin(), context.bookmarks().end(),
                             [](const Bookmark& bookmark) { return bookmark.category == "Non-Returning Function"; }));
}

} // namespace
} // namespace ghidra::analyzer::tests
