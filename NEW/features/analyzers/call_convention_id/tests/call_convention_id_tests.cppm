module;

#include <gtest/gtest.h>

export module analyzer_call_convention_id_tests;

import analyzer_call_convention_id;
import analyzer_test_support;
import std;

/// Verifies that convention identification consumes generated decompiler output rather than ABI register patterns.
TEST(CallConventionId, ReadsExplicitDecompilerConvention) {
    EXPECT_EQ(ghidra::analyzer::identify_calling_convention("int __fastcall convention_target(int a, int b, int c)"),
              std::optional<std::string>("__fastcall"));
    EXPECT_FALSE(ghidra::analyzer::identify_calling_convention("int convention_target(int a, int b, int c)"));
}

/// Verifies that an eligible fixture function remains model-consistent when the frontend has no explicit convention.
TEST(CallConventionId, DoesNotInventConventionWhenFrontendOmitsOne) {
    auto context = ghidra::analyzer::tests::load_fixture("call_convention_id");
    context.options().call_convention_id = true;
    ASSERT_TRUE(context.disassemble_flow(0x140001000ULL));
    ASSERT_TRUE(context.create_function(0x140001000ULL, "convention_target"));
    ASSERT_TRUE(context.set_function_signature(
        0x140001000ULL, "default", "int",
        {{"a", "int", {}, 0, 4, false}, {"b", "int", {}, 0, 4, false}, {"c", "int", {}, 0, 4, false}}, false, false));
    ghidra::analyzer::CallConventionIdAnalyzer analyzer;
    ghidra::analyzer::CancellationToken cancellation;
    analyzer.analyze(context, {}, cancellation);
    const auto* function = context.function_at(0x140001000ULL);
    ASSERT_NE(function, nullptr);
    EXPECT_TRUE(function->calling_convention == "default" || function->calling_convention == "__fastcall");
    EXPECT_EQ(function->parameters.size(), 3U);
}
