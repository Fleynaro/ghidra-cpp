module;

#include <gtest/gtest.h>

export module subroutine_references_tests;

import analyzer_disassemble_entry_points;
import analyzer_function_body;
import analyzer_subroutine_references;
import analyzer_test_support;
import std;

namespace ghidra::analyzer::tests {
namespace {

/// Verifies direct call references create functions and bodies through the event chain.
TEST(AnalyzerPipelineTest, DirectCallsCreateFunctionsAndCfg) {
    auto context = load_fixture("subroutine_references");
    auto& options = context.options();
    options.function_start_search = false;
    options.reference = false;
    options.data_reference = false;
    options.scalar_operand_references = false;
    options.stack = false;
    options.constant_propagation = false;
    options.non_returning_functions = false;
    AutoAnalysisManager manager(context);
    manager.register_analyzer(std::make_unique<DisassembleEntryPointsAnalyzer>());
    manager.register_analyzer(std::make_unique<SubroutineReferencesAnalyzer>());
    manager.register_analyzer(std::make_unique<FunctionBodyAnalyzer>());
    const auto result = manager.analyze();
    ASSERT_TRUE(result.completed);
    ASSERT_TRUE(context.instructions().contains(0x140001040));
    ASSERT_TRUE(context.image().is_executable(0x140001000));
    ASSERT_TRUE(context.instructions().at(0x140001040).instruction.flow.target.has_value());
    // Copied from the subroutine-reference Ghidra Delta rows for added/changed bodies.
    const std::array<ExpectedFunctionBody, 5> expected{{{0x140001000, {{0x140001000, 0x14000100A}}},
                                                        {0x140001014, {{0x140001014, 0x14000101E}}},
                                                        {0x140001028, {{0x140001028, 0x140001032}}},
                                                        {0x14000103C, {{0x14000103C, 0x14000106A}}},
                                                        {0x140001074, {{0x140001074, 0x1400010A4}}}}};
    expect_function_bodies(context, expected);
    EXPECT_TRUE(std::any_of(context.references().begin(), context.references().end(), [](const Reference& reference) {
        return reference.source == 0x140001040 && reference.target == 0x140001000 &&
               reference.kind == ReferenceKind::unconditional_call;
    }));
    EXPECT_TRUE(context.functions().contains(0x140001000));
    EXPECT_TRUE(context.functions().contains(0x140001014));
    EXPECT_TRUE(context.functions().contains(0x140001028));
    EXPECT_GE(context.functions().at(0x140001000).body.size(), 2U);
    EXPECT_GE(context.functions().at(0x140001014).body.size(), 2U);
    EXPECT_GE(context.functions().at(0x140001028).body.size(), 2U);
    EXPECT_TRUE(std::all_of(context.functions().begin(), context.functions().end(),
                            [](const auto& pair) { return !pair.second.blocks.empty(); }));
}

} // namespace
} // namespace ghidra::analyzer::tests
