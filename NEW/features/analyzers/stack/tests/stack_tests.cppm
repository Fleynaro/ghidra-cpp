module;

#include <gtest/gtest.h>

export module stack_tests;

import analyzer_disassemble_entry_points;
import analyzer_stack;
import analyzer_subroutine_references;
import analyzer_test_support;
import std;

namespace ghidra::analyzer::tests {
namespace {

/// Verifies stack analysis consumes real PE/Sleigh operands and reports golden local names.
TEST(AnalyzerPipelineTest, FindsStackVariablesAndReferences) {
    auto context = load_fixture("stack");
    auto& options = context.options();
    options.function_start_search = false;
    options.reference = false;
    options.data_reference = false;
    options.scalar_operand_references = false;
    options.constant_propagation = false;
    options.non_returning_functions = false;
    AutoAnalysisManager manager(context);
    manager.register_analyzer(std::make_unique<DisassembleEntryPointsAnalyzer>());
    manager.register_analyzer(std::make_unique<SubroutineReferencesAnalyzer>());
    manager.register_analyzer(std::make_unique<StackAnalyzer>());
    const auto result = manager.analyze();
    ASSERT_TRUE(result.completed);
    // Copied from the stack Ghidra Delta stack-variable rows.
    EXPECT_TRUE(std::any_of(context.functions().begin(), context.functions().end(), [](const auto& pair) {
        return std::any_of(pair.second.stack_variables.begin(), pair.second.stack_variables.end(),
                           [](const StackVariable& variable) { return variable.name == "local_res10"; });
    }));
    EXPECT_TRUE(std::any_of(context.functions().begin(), context.functions().end(), [](const auto& pair) {
        return std::any_of(pair.second.stack_variables.begin(), pair.second.stack_variables.end(),
                           [](const StackVariable& variable) { return variable.name == "local_res8"; });
    }));
    const auto stack_variable_count =
        std::accumulate(context.functions().begin(), context.functions().end(), std::size_t{0},
                        [](std::size_t count, const auto& pair) { return count + pair.second.stack_variables.size(); });
    EXPECT_EQ(stack_variable_count, 4U);
    const auto stack_reference_count =
        std::count_if(context.references().begin(), context.references().end(),
                      [](const Reference& reference) { return reference.kind == ReferenceKind::stack; });
    EXPECT_EQ(stack_reference_count, 11);
    const std::array<std::pair<Address, std::int64_t>, 11> expected_stack{{{0x140001003, 0x10},
                                                                           {0x140001007, 0x10},
                                                                           {0x14000100E, 0x18},
                                                                           {0x140001012, 0x8},
                                                                           {0x140001016, 0x10},
                                                                           {0x14000101E, 0x20},
                                                                           {0x140001029, 0x18},
                                                                           {0x140001035, 0x20},
                                                                           {0x14000103A, 0x8},
                                                                           {0x14000103F, 0x18},
                                                                           {0x140001049, 0x10}}};
    for (const auto& [source, offset] : expected_stack) {
        EXPECT_TRUE(std::any_of(context.references().begin(), context.references().end(),
                                [&](const Reference& reference) {
                                    return reference.kind == ReferenceKind::stack && reference.source == source &&
                                           reference.stack_offset == offset;
                                }))
            << "missing stack reference at source 0x" << std::hex << source << " offset 0x" << offset;
    }
    const auto& worker = context.functions().at(0x140001000);
    ASSERT_EQ(worker.stack_variables.size(), 4U);
    for (const auto& variable : worker.stack_variables) {
        const auto expected_size = variable.offset == 0x8 ? 1U : variable.offset == 0x20 ? 8U : 4U;
        EXPECT_EQ(variable.size, expected_size);
        EXPECT_FALSE(variable.parameter);
    }
}

} // namespace
} // namespace ghidra::analyzer::tests
