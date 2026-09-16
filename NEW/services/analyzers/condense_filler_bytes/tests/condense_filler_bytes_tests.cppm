module;

#include <gtest/gtest.h>

export module condense_filler_bytes_tests;

import analyzer_condense_filler_bytes;
import analyzer_test_support;
import std;

namespace ghidra::analyzer::tests {
namespace {

/// Materializes executable fixture code without making an analyzer responsible for PE/Sleigh setup.
void materialize_executable(AnalysisContext& context) {
    for (const auto& region : context.image().memory_regions()) {
        if (!region.executable || region.size == 0U) {
            continue;
        }
        for (Address address = region.start, end = region.start + region.size - 1U;; ++address) {
            static_cast<void>(context.disassemble(address));
            if (address == end) {
                break;
            }
        }
    }
}

/// Seeds the runtime function whose last byte precedes the fixture's expected one-byte filler.
bool seed_function_ending_at_fixture_boundary(AnalysisContext& context) {
    constexpr Address filler_address = 0x1400002B9ULL;
    const Address last_instruction = filler_address - 1U;
    if (!context.instructions().contains(last_instruction)) {
        sleigh_runtime::Instruction instruction;
        instruction.address = last_instruction;
        instruction.length = 1U;
        instruction.mnemonic = "RET";
        instruction.flow = {sleigh_runtime::FlowKind::return_op, std::nullopt, false, true};
        static_cast<void>(context.define_instruction(std::move(instruction)));
    }
    return context.function_at(last_instruction) != nullptr || context.create_function(last_instruction);
}

/// Verifies Auto mode selects the dominant linker filler and creates the exact fixture alignment row.
TEST(CondenseFillerBytesIntegrationTest, CondensesTheExpectedFixtureBoundary) {
    auto context = load_fixture("condense_filler_bytes");
    ASSERT_TRUE(seed_function_ending_at_fixture_boundary(context));
    context.options().condense_filler_bytes = true;
    context.options().filler_auto_detect = true;
    context.options().filler_minimum_length = 1U;

    CancellationToken cancellation;
    CondenseFillerBytesAnalyzer analyzer;
    analyzer.analyze(context, {}, cancellation);

    const auto alignment = context.data().find(0x1400002B9ULL);
    ASSERT_NE(alignment, context.data().end());
    EXPECT_EQ(alignment->second.type, "alignment");
    EXPECT_EQ(alignment->second.size, 1U);
    EXPECT_TRUE(alignment->second.alignment);
}

/// Verifies a configured explicit filler byte is honored instead of auto-detection.
TEST(CondenseFillerBytesIntegrationTest, HonorsExplicitFillerSelection) {
    auto context = load_fixture("condense_filler_bytes");
    ASSERT_TRUE(seed_function_ending_at_fixture_boundary(context));
    context.options().condense_filler_bytes = true;
    context.options().filler_auto_detect = false;
    context.options().filler_byte = 0x00U;
    context.options().filler_minimum_length = 2U;

    CancellationToken cancellation;
    CondenseFillerBytesAnalyzer analyzer;
    analyzer.analyze(context, {}, cancellation);
    EXPECT_EQ(context.data().find(0x1400002B9ULL), context.data().end());
}

} // namespace
} // namespace ghidra::analyzer::tests
