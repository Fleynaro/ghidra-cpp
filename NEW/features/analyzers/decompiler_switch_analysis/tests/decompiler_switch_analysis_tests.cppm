module;

#include <gtest/gtest.h>

export module analyzer_decompiler_switch_analysis_tests;

import analyzer_decompiler_switch_analysis;
import analyzer_test_support;
import std;

/// Verifies stable parsing of native control-flow addresses used by switch recovery.
TEST(DecompilerSwitchAnalysis, ParsesControlFlowAddresses) {
    const auto addresses =
        ghidra::analyzer::decompiler_control_flow_addresses("block 0x140001019 -> 0x14000101b; block 0x140001021");
    ASSERT_EQ(addresses.size(), 3U);
    EXPECT_EQ(addresses[0], 0x140001019ULL);
    EXPECT_EQ(addresses[1], 0x14000101bULL);
    EXPECT_EQ(addresses[2], 0x140001021ULL);
}

/// Verifies the real fixture's eight computed-jump destinations are persisted after native switch recovery.
TEST(DecompilerSwitchAnalysis, RecoversFixtureSwitchTargets) {
    auto context = ghidra::analyzer::tests::load_fixture("decompiler_switch_analysis");
    context.options().decompiler_switch_analysis = true;
    ASSERT_TRUE(context.disassemble_flow(0x140001000ULL));
    ASSERT_TRUE(context.create_function(0x140001000ULL, "switch_target"));
    ghidra::analyzer::DecompilerSwitchAnalysisAnalyzer analyzer;
    ghidra::analyzer::CancellationToken cancellation;
    analyzer.analyze(context, {}, cancellation);
    const auto* function = context.function_at(0x140001000ULL);
    ASSERT_NE(function, nullptr);
    EXPECT_TRUE(function->switch_recovered);
    const std::set<ghidra::analyzer::Address> expected{0x14000101bULL, 0x140001021ULL, 0x140001027ULL, 0x14000102dULL,
                                                       0x140001033ULL, 0x140001039ULL, 0x14000103fULL, 0x140001045ULL};
    std::set<ghidra::analyzer::Address> actual;
    for (const auto& reference : context.references())
        if (reference.source == 0x140001019ULL && reference.kind == ghidra::analyzer::ReferenceKind::computed_jump)
            actual.insert(reference.target);
    EXPECT_EQ(actual, expected);
}
