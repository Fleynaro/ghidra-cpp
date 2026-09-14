module;

#include <gtest/gtest.h>

export module stack_tests;

import analyzer_disassemble_entry_points;
import analyzer_function_body;
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
    manager.register_analyzer(std::make_unique<FunctionBodyAnalyzer>());
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
    EXPECT_TRUE(std::any_of(context.functions().begin(), context.functions().end(),
                            [](const auto& pair) { return !pair.second.stack_variables.empty(); }));
    EXPECT_TRUE(std::any_of(context.references().begin(), context.references().end(),
                            [](const Reference& reference) { return reference.kind == ReferenceKind::stack; }));
    const std::array<std::pair<Address, std::int64_t>, 6> expected_stack{{{0x140001003, 0x10},
                                                                          {0x140001007, 0x10},
                                                                          {0x14000100E, 0x8},
                                                                          {0x140001018, 0x8},
                                                                          {0x140001024, 0x8},
                                                                          {0x140001028, 0x10}}};
    for (const auto& [source, offset] : expected_stack) {
        EXPECT_TRUE(
            std::any_of(context.references().begin(), context.references().end(), [&](const Reference& reference) {
                return reference.kind == ReferenceKind::stack && reference.source == source &&
                       reference.stack_offset == offset;
            }));
    }
}

} // namespace
} // namespace ghidra::analyzer::tests
