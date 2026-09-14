module;

#include <gtest/gtest.h>

export module scalar_operand_references_tests;

import analyzer;
import analyzer_test_support;
import std;

namespace ghidra::analyzer::tests {
namespace {

/// Verifies scalar and p-code reference analyzers retain both positive and negative controls.
TEST(AnalyzerPipelineTest, ScalarReferencesOnlyUseMappedLargeValues) {
    auto context = load_fixture("scalar_operand_references");
    auto& options = context.options();
    options.disassemble_entry_points = true;
    options.function_start_search = false;
    options.subroutine_references = false;
    options.function_body = false;
    options.reference = false;
    options.data_reference = false;
    options.stack = false;
    options.constant_propagation = false;
    options.non_returning_functions = false;
    AutoAnalysisManager manager(context);
    manager.register_analyzer(std::make_unique<DisassembleEntryPointsAnalyzer>());
    manager.register_analyzer(std::make_unique<SubroutineReferencesAnalyzer>());
    manager.register_analyzer(std::make_unique<FunctionBodyAnalyzer>());
    manager.register_analyzer(std::make_unique<ConstantPropagationAnalyzer>());
    manager.register_analyzer(std::make_unique<ScalarOperandReferencesAnalyzer>());
    const auto result = manager.analyze();
    ASSERT_TRUE(result.completed);
    const auto scalar =
        std::count_if(context.references().begin(), context.references().end(),
                      [](const Reference& reference) { return reference.kind == ReferenceKind::scalar; });
    EXPECT_EQ(scalar, 2);
    EXPECT_TRUE(std::any_of(context.references().begin(), context.references().end(), [](const Reference& reference) {
        return reference.kind == ReferenceKind::scalar && reference.source == 0x140001000 &&
               reference.target == 0x140003000;
    }));
    EXPECT_TRUE(std::any_of(context.references().begin(), context.references().end(), [](const Reference& reference) {
        return reference.kind == ReferenceKind::scalar && reference.source == 0x140001014 &&
               reference.target == 0x140001000;
    }));
    EXPECT_TRUE(std::none_of(context.references().begin(), context.references().end(), [](const Reference& reference) {
        return reference.kind == ReferenceKind::scalar &&
               (reference.source == 0x140001028 || reference.source == 0x140001034 || reference.source == 0x140001040 ||
                reference.source == 0x1400010A9);
    }));
}

} // namespace
} // namespace ghidra::analyzer::tests
