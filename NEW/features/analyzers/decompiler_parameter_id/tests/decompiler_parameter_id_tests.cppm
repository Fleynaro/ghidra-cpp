module;

#include <gtest/gtest.h>

export module analyzer_decompiler_parameter_id_tests;

import analyzer_decompiler_parameter_id;
import analyzer_test_support;
import std;

/// Verifies that the analyzer invokes the native decompiler on a real bounded fixture function.
TEST(DecompilerParameterId, CompletesNativeParameterPass) {
    auto context = ghidra::analyzer::tests::load_fixture("decompiler_parameter_id");
    context.options().decompiler_parameter_id = true;
    ASSERT_TRUE(context.disassemble_flow(0x140001000ULL));
    ASSERT_TRUE(context.create_function(0x140001000ULL, "parameter_target"));
    ghidra::analyzer::DecompilerParameterIdAnalyzer analyzer;
    ghidra::analyzer::CancellationToken cancellation;
    analyzer.analyze(context, {}, cancellation);
    const auto* function = context.function_at(0x140001000ULL);
    ASSERT_NE(function, nullptr);
    EXPECT_TRUE(function->parameter_id_complete);
}

/// Verifies that an already completed signature is not rewritten by a later event.
TEST(DecompilerParameterId, PreservesCompletedState) {
    auto context = ghidra::analyzer::tests::load_fixture("decompiler_parameter_id");
    context.options().decompiler_parameter_id = true;
    ASSERT_TRUE(context.disassemble_flow(0x140001000ULL));
    ASSERT_TRUE(context.create_function(0x140001000ULL, "parameter_target"));
    ASSERT_TRUE(context.set_parameter_id_complete(0x140001000ULL));
    const auto before = context.function_at(0x140001000ULL)->parameters;
    ghidra::analyzer::DecompilerParameterIdAnalyzer analyzer;
    ghidra::analyzer::CancellationToken cancellation;
    analyzer.analyze(context, {}, cancellation);
    EXPECT_EQ(context.function_at(0x140001000ULL)->parameters, before);
}
