module;

#include <gtest/gtest.h>

export module function_start_search_tests;

import analyzer_disassemble_entry_points;
import analyzer_function_start_search;
import analyzer_test_support;
import std;

namespace recode::analyzer::tests {
namespace {

/// Verifies the pattern phase creates both independent positive candidates while
/// preserving the two already-defined compiler-generated negative candidates.
TEST(AnalyzerPipelineTest, PatternSearchCreatesPositiveCandidate) {
    auto context = load_fixture("function_start_search");
    for (const Address existing : {0x140001000ULL, 0x140001080ULL, 0x140001100ULL}) {
        ASSERT_TRUE(context.create_function(existing));
    }
    auto& options = context.options();
    options.disassemble_entry_points = true;
    options.pattern_root = std::filesystem::path(ANALYZER_FIXTURE_DIR) / "function_start_search" / "data" / "patterns";
    options.subroutine_references = false;
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
    // Copied from the function-start Ghidra Delta rows for both raw patterns.
    EXPECT_TRUE(context.functions().contains(0x140005003));
    EXPECT_TRUE(context.functions().contains(0x140005013));
    EXPECT_TRUE(context.functions().contains(0x140001000));
    EXPECT_TRUE(context.functions().contains(0x140001080));
    EXPECT_EQ(context.functions().size(), 5U);
    EXPECT_EQ(std::count_if(context.bookmarks().begin(), context.bookmarks().end(),
                            [](const Bookmark& bookmark) { return bookmark.category == "Function Start Search"; }),
              2U);
    EXPECT_TRUE(std::any_of(context.bookmarks().begin(), context.bookmarks().end(), [](const Bookmark& bookmark) {
        return bookmark.address == 0x140005003 && bookmark.category == "Function Start Search";
    }));
    EXPECT_TRUE(std::any_of(context.bookmarks().begin(), context.bookmarks().end(), [](const Bookmark& bookmark) {
        return bookmark.address == 0x140005013 && bookmark.category == "Function Start Search";
    }));
}

} // namespace
} // namespace recode::analyzer::tests
