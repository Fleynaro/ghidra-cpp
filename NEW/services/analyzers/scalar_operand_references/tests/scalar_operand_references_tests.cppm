module;

#include <gtest/gtest.h>

export module scalar_operand_references_tests;

import analyzer_constant_propagation;
import analyzer_disassemble_entry_points;
import analyzer_scalar_operand_references;
import analyzer_subroutine_references;
import analyzer_test_support;
import std;

namespace recode::analyzer::tests {
namespace {

/// Verifies scalar and p-code reference analyzers retain both positive and negative controls.
TEST(AnalyzerPipelineTest, ScalarReferencesOnlyUseMappedLargeValues) {
    auto context = load_fixture("scalar_operand_references");
    auto& options = context.options();
    options.disassemble_entry_points = true;
    options.function_start_search = false;
    options.subroutine_references = false;
    options.reference = false;
    options.data_reference = false;
    options.stack = false;
    options.constant_propagation = false;
    options.non_returning_functions = false;
    AutoAnalysisManager manager(context);
    manager.register_analyzer(std::make_unique<DisassembleEntryPointsAnalyzer>());
    manager.register_analyzer(std::make_unique<SubroutineReferencesAnalyzer>());
    manager.register_analyzer(std::make_unique<ConstantPropagationAnalyzer>());
    manager.register_analyzer(std::make_unique<ScalarOperandReferencesAnalyzer>());
    const auto result = manager.analyze();
    ASSERT_TRUE(result.completed);
    const auto scalar =
        std::count_if(context.references().begin(), context.references().end(),
                      [](const Reference& reference) { return reference.kind == ReferenceKind::scalar; });
    EXPECT_EQ(scalar, 3);
    EXPECT_TRUE(std::any_of(context.references().begin(), context.references().end(), [](const Reference& reference) {
        return reference.kind == ReferenceKind::scalar && reference.source == 0x140001000 &&
               reference.target == 0x140003000 && reference.operand_index == 1U;
    }));
    EXPECT_TRUE(std::any_of(context.references().begin(), context.references().end(), [](const Reference& reference) {
        return reference.kind == ReferenceKind::scalar && reference.source == 0x140001014 &&
               reference.target == 0x140001000 && reference.operand_index == 1U;
    }));
    EXPECT_TRUE(std::any_of(context.references().begin(), context.references().end(), [](const Reference& reference) {
        return reference.kind == ReferenceKind::scalar && reference.source == 0x140001040 &&
               reference.target == 0x140002000 && reference.operand_index == 1U;
    }));
    EXPECT_TRUE(std::none_of(context.references().begin(), context.references().end(), [](const Reference& reference) {
        return reference.kind == ReferenceKind::scalar &&
               (reference.source == 0x140001028 || reference.source == 0x140001034 || reference.source == 0x140001054 ||
                reference.source == 0x1400010D3);
    }));
}

/// Verifies an existing stronger operand reference prevents a speculative
/// scalar reference from being added for the same instruction operand.
TEST(AnalyzerPipelineTest, PreservesExistingOperandReference) {
    auto context = load_fixture("scalar_operand_references");
    context.options() = {};
    context.options().scalar_operand_references = true;
    const auto executable =
        std::find_if(context.image().memory_regions().begin(), context.image().memory_regions().end(),
                     [](const pe::MemoryRegion& region) { return region.executable; });
    ASSERT_NE(executable, context.image().memory_regions().end());
    const Address source = executable->start + 0x300U;
    sleigh_runtime::Instruction instruction;
    instruction.address = source;
    instruction.length = 1;
    instruction.operands.push_back(
        sleigh_runtime::Operand{"0x140003000", sleigh_runtime::OperandKind::immediate, 0x140003000U});
    ASSERT_TRUE(context.define_instruction(std::move(instruction)));
    ASSERT_TRUE(context.add_reference(
        Reference{source, 0x140003000, ReferenceKind::data, 0, std::nullopt, FlowOverride::none, false}));

    AutoAnalysisManager manager(context);
    manager.register_analyzer(std::make_unique<ConstantPropagationAnalyzer>());
    manager.register_analyzer(std::make_unique<ScalarOperandReferencesAnalyzer>());
    const auto result = manager.analyze(std::array<Address, 1>{source});
    ASSERT_TRUE(result.completed);
    EXPECT_TRUE(std::none_of(context.references().begin(), context.references().end(), [&](const Reference& reference) {
        return reference.source == source && reference.kind == ReferenceKind::scalar;
    }));
}

} // namespace
} // namespace recode::analyzer::tests
