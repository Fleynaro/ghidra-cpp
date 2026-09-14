module;

#include <gtest/gtest.h>

export module aggressive_instruction_finder_tests;

import analyzer_aggressive_instruction_finder;
import analyzer_test_support;
import std;

namespace ghidra::analyzer::tests {
namespace {

/// Seeds the twenty-four repeated function starts while leaving the candidate bytes undefined.
void seed_repeated_functions(AnalysisContext& context) {
    for (std::uint64_t index = 0; index < 24U; ++index) {
        const Address entry = 0x140001000ULL + index * 0x20U;
        static_cast<void>(context.disassemble_flow(entry));
        ASSERT_TRUE(context.create_function(entry));
    }
}

/// Verifies the repeated-start hash rediscovers the undefined candidate and emits only its bookmark.
TEST(AggressiveInstructionFinderIntegrationTest, FindsUndefinedRepeatedStartCandidate) {
    auto context = load_fixture("aggressive_instruction_finder");
    context.options().seed_provider_functions = false;
    context.options().aggressive_instruction_finder = true;
    context.options().create_analysis_bookmarks = true;
    seed_repeated_functions(context);
    const auto function_count_before = context.functions().size();

    CancellationToken cancellation;
    AggressiveInstructionFinderAnalyzer analyzer;
    analyzer.analyze(context, {}, cancellation);

    EXPECT_EQ(context.functions().size(), function_count_before);
    EXPECT_TRUE(context.instructions().contains(0x140001300ULL));
    ASSERT_EQ(
        std::count_if(context.bookmarks().begin(), context.bookmarks().end(),
                      [](const Bookmark& bookmark) { return bookmark.category == "Aggressive Instruction Finder"; }),
        1U);
    EXPECT_TRUE(std::any_of(context.bookmarks().begin(), context.bookmarks().end(), [](const Bookmark& bookmark) {
        return bookmark.address == 0x140001300ULL && bookmark.comment == "Found code";
    }));
}

/// Verifies the Java minimum-function safety guard suppresses the heuristic on a small program.
TEST(AggressiveInstructionFinderIntegrationTest, RequiresTwentyKnownFunctions) {
    auto context = load_fixture("aggressive_instruction_finder");
    context.options().seed_provider_functions = false;
    context.options().aggressive_instruction_finder = true;
    for (std::uint64_t index = 0; index < 4U; ++index) {
        const Address entry = 0x140001000ULL + index * 0x20U;
        static_cast<void>(context.disassemble_flow(entry));
        ASSERT_TRUE(context.create_function(entry));
    }
    CancellationToken cancellation;
    AggressiveInstructionFinderAnalyzer analyzer;
    analyzer.analyze(context, {}, cancellation);
    EXPECT_TRUE(context.bookmarks().empty());
    EXPECT_FALSE(context.instructions().contains(0x140001300ULL));
}

} // namespace
} // namespace ghidra::analyzer::tests
