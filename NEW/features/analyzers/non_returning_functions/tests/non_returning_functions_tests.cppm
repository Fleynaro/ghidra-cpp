module;

#include <gtest/gtest.h>

export module non_returning_functions_tests;

import analyzer_disassemble_entry_points;
import analyzer_function_body;
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
    options.function_body = false;
    options.reference = false;
    options.data_reference = false;
    options.scalar_operand_references = false;
    options.stack = false;
    options.constant_propagation = false;
    AutoAnalysisManager manager(context);
    manager.register_analyzer(std::make_unique<DisassembleEntryPointsAnalyzer>());
    manager.register_analyzer(std::make_unique<SubroutineReferencesAnalyzer>());
    manager.register_analyzer(std::make_unique<FunctionBodyAnalyzer>());
    manager.register_analyzer(std::make_unique<NonReturningFunctionsAnalyzer>());
    const auto result = manager.analyze();
    ASSERT_TRUE(result.completed);
    // Copied from the known-no-return Ghidra Delta: abort is the only known row.
    ASSERT_TRUE(context.functions().contains(0x140001000));
    EXPECT_TRUE(context.functions().at(0x140001000).no_return);
    EXPECT_TRUE(std::any_of(context.bookmarks().begin(), context.bookmarks().end(), [](const Bookmark& bookmark) {
        return bookmark.address == 0x140001000 && bookmark.category == "Non-Returning Function";
    }));
    EXPECT_TRUE(std::all_of(context.functions().begin(), context.functions().end(),
                            [](const auto& pair) { return pair.first == 0x140001000 || !pair.second.no_return; }));
}

/// Verifies three independent post-call indicators mark a discovered no-return target.
TEST(AnalyzerPipelineTest, DiscoversNoReturnFromCallEvidence) {
    auto context = load_fixture("non_returning_functions_discovered");
    auto& options = context.options();
    options.function_start_search = false;
    options.subroutine_references = true;
    options.function_body = true;
    options.reference = false;
    options.data_reference = false;
    options.scalar_operand_references = false;
    options.stack = false;
    options.constant_propagation = false;
    AutoAnalysisManager manager(context);
    manager.register_analyzer(std::make_unique<DisassembleEntryPointsAnalyzer>());
    manager.register_analyzer(std::make_unique<SubroutineReferencesAnalyzer>());
    manager.register_analyzer(std::make_unique<FunctionBodyAnalyzer>());
    manager.register_analyzer(std::make_unique<NonReturningFunctionsAnalyzer>());
    const auto result = manager.analyze();
    ASSERT_TRUE(result.completed);
    // Copied from the discovered-no-return Ghidra Delta: exactly three CALL_RETURN rows.
    ASSERT_TRUE(context.functions().contains(0x140001000));
    EXPECT_TRUE(context.functions().at(0x140001000).no_return);
    EXPECT_EQ(
        std::count_if(context.references().begin(), context.references().end(),
                      [](const Reference& reference) { return reference.flow_override == FlowOverride::call_return; }),
        3);
}

} // namespace
} // namespace ghidra::analyzer::tests
