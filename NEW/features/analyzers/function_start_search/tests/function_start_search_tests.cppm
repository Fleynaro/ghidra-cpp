module;

#include <gtest/gtest.h>

export module function_start_search_tests;

import analyzer;
import analyzer_test_support;
import std;

namespace ghidra::analyzer::tests {
namespace {

/// Verifies the pattern phase creates the positive function-start candidate from the golden fixture.
TEST(AnalyzerPipelineTest, PatternSearchCreatesPositiveCandidate) {
    auto context = load_fixture("function_start_search");
    for (const Address existing : {0x140001000ULL, 0x140001080ULL, 0x140001100ULL}) {
        ASSERT_TRUE(context.create_function(existing));
    }
    auto& options = context.options();
    options.disassemble_entry_points = true;
    options.pattern_root = std::filesystem::path(ANALYZER_FIXTURE_DIR) / "function_start_search" / "data" / "patterns";
    options.subroutine_references = false;
    options.function_body = false;
    options.reference = false;
    options.data_reference = false;
    options.scalar_operand_references = false;
    options.stack = false;
    options.constant_propagation = false;
    options.non_returning_functions = false;
    AutoAnalysisManager manager(context);
    manager.register_analyzer(std::make_unique<DisassembleEntryPointsAnalyzer>());
    manager.register_analyzer(std::make_unique<FunctionStartPreAnalyzer>());
    manager.register_analyzer(std::make_unique<FunctionStartAnalyzer>());
    manager.register_analyzer(std::make_unique<FunctionStartFunctionAnalyzer>());
    manager.register_analyzer(std::make_unique<FunctionStartPostAnalyzer>());
    manager.register_analyzer(std::make_unique<FunctionStartDataPostAnalyzer>());
    const auto result = manager.analyze();
    ASSERT_TRUE(result.completed);
    // Copied from the function-start Ghidra Delta row for the positive pattern.
    EXPECT_TRUE(context.functions().contains(0x140005003));
    EXPECT_TRUE(std::any_of(context.bookmarks().begin(), context.bookmarks().end(), [](const Bookmark& bookmark) {
        return bookmark.address == 0x140005003 && bookmark.category == "Function Start Search";
    }));
}

} // namespace
} // namespace ghidra::analyzer::tests
