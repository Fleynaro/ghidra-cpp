module;

#include <gtest/gtest.h>

export module shared_return_calls_tests;

import analyzer_shared_return_calls;
import analyzer_test_support;
import std;

namespace ghidra::analyzer::tests {
namespace {

/// Builds the one-byte synthetic instructions used to isolate SharedReturnAnalysisCmd flow rules.
[[nodiscard]] sleigh_runtime::Instruction synthetic_instruction(Address address, sleigh_runtime::FlowKind kind,
                                                                std::optional<Address> target, bool fallthrough,
                                                                bool terminal) {
    sleigh_runtime::Instruction instruction;
    instruction.address = address;
    instruction.length = 1U;
    instruction.flow = {kind, target ? std::optional{sleigh_runtime::Varnode{"ram", *target, 8U}} : std::nullopt,
                        fallthrough, terminal};
    return instruction;
}

/// Verifies the fixture's seeded destination-function scenario changes only the jump flow.
TEST(SharedReturnCallsIntegrationTest, ConvertsExistingFunctionJumpToCallReturn) {
    auto context = load_fixture("shared_return_calls");
    context.options().seed_provider_functions = false;
    context.options().shared_return_calls = true;
    constexpr Address target = 0x140001000ULL;
    constexpr Address source = 0x140001020ULL;
    constexpr Address entry = 0x140001030ULL;
    ASSERT_TRUE(context.define_instruction(
        synthetic_instruction(target, sleigh_runtime::FlowKind::return_op, std::nullopt, false, true)));
    ASSERT_TRUE(context.define_instruction(
        synthetic_instruction(source, sleigh_runtime::FlowKind::branch, target, false, true)));
    ASSERT_TRUE(context.define_instruction(
        synthetic_instruction(entry, sleigh_runtime::FlowKind::return_op, std::nullopt, false, true)));
    ASSERT_TRUE(context.create_function(target));
    ASSERT_TRUE(context.create_function(entry));

    AutoAnalysisManager manager(context);
    manager.register_analyzer(std::make_unique<SharedReturnCallsAnalyzer>());
    ASSERT_TRUE(manager.analyze().completed);

    ASSERT_EQ(context.functions().size(), 2U);
    const auto converted = std::find_if(context.references().begin(), context.references().end(),
                                        [](const Reference& reference) { return reference.source == 0x140001020ULL; });
    ASSERT_NE(converted, context.references().end());
    EXPECT_EQ(converted->target, target);
    EXPECT_EQ(converted->kind, ReferenceKind::unconditional_call);
    EXPECT_EQ(converted->flow_override, FlowOverride::call_return);
}

/// Verifies the Java default rejects conditional branches as shared returns.
TEST(SharedReturnCallsIntegrationTest, IgnoresConditionalFunctionJumpByDefault) {
    auto context = load_fixture("shared_return_calls");
    context.options().seed_provider_functions = false;
    constexpr Address target = 0x140001100ULL;
    constexpr Address source = 0x140001120ULL;
    ASSERT_TRUE(context.define_instruction(
        synthetic_instruction(target, sleigh_runtime::FlowKind::return_op, std::nullopt, false, true)));
    ASSERT_TRUE(context.define_instruction(
        synthetic_instruction(source, sleigh_runtime::FlowKind::conditional_branch, target, true, false)));
    ASSERT_TRUE(context.create_function(target));

    AutoAnalysisManager manager(context);
    manager.register_analyzer(std::make_unique<SharedReturnCallsAnalyzer>());
    ASSERT_TRUE(manager.analyze().completed);
    const auto jump = std::find_if(context.references().begin(), context.references().end(),
                                   [](const Reference& reference) { return reference.source == 0x140001120ULL; });
    ASSERT_NE(jump, context.references().end());
    EXPECT_EQ(jump->kind, ReferenceKind::conditional_jump);
    EXPECT_EQ(jump->flow_override, FlowOverride::none);
}

} // namespace
} // namespace ghidra::analyzer::tests
