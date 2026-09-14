module;

#include <gtest/gtest.h>

export module call_fixup_installer_tests;

import analyzer_call_fixup_installer;
import analyzer_test_support;
import std;

namespace ghidra::analyzer::tests {
namespace {

/// Verifies target, underscore fallback, fixup evidence, and no-return state use the Java rules.
TEST(CallFixupInstallerAnalyzerTest, AppliesCompilerSpecRule) {
    auto context = load_fixture("call_fixup_installer");
    const auto entry = context.image().entry_point_va();
    ASSERT_TRUE(entry.has_value());
    ASSERT_GT(context.disassemble_flow(*entry), 0U);
    ASSERT_TRUE(context.create_function(*entry, "fixture_target"));
    CallFixupInstallerAnalyzer analyzer({CallFixupRule{"__fixture_target", "fixture_fixup", true, false}});
    CancellationToken cancellation;
    analyzer.analyze(context, {}, cancellation);

    ASSERT_NE(context.function_at(*entry), nullptr);
    EXPECT_TRUE(context.function_at(*entry)->no_return);
    ASSERT_EQ(context.symbols().size(), 1U);
    EXPECT_EQ(context.symbols().front().kind, "call_fixup");
    EXPECT_EQ(context.symbols().front().demangled_name, "fixture_fixup");
}

/// Verifies no compiler-spec mappings means no guessed fixup or function state.
TEST(CallFixupInstallerAnalyzerTest, DoesNotGuessMissingCompilerSpecRules) {
    auto context = load_fixture("call_fixup_installer");
    CancellationToken cancellation;
    CallFixupInstallerAnalyzer analyzer;
    analyzer.analyze(context, {}, cancellation);
    EXPECT_TRUE(context.symbols().empty());
    EXPECT_TRUE(context.bookmarks().empty());
}

} // namespace
} // namespace ghidra::analyzer::tests
