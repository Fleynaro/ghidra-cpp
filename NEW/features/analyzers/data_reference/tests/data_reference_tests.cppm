module;

#include <gtest/gtest.h>

export module data_reference_tests;

import analyzer_constant_propagation;
import analyzer_data_reference;
import analyzer_reference;
import analyzer_subroutine_references;
import analyzer_test_support;
import std;

namespace ghidra::analyzer::tests {
namespace {

/// Verifies Data Reference scans pointer-sized PE data cells after its prerequisite event.
TEST(AnalyzerPipelineTest, FollowsDataSectionPointers) {
    auto context = load_fixture("data_reference");
    auto& options = context.options();
    options.disassemble_entry_points = false;
    options.function_start_search = false;
    options.subroutine_references = false;
    options.reference = false;
    options.scalar_operand_references = false;
    options.stack = false;
    options.constant_propagation = false;
    options.non_returning_functions = false;
    AutoAnalysisManager manager(context);
    manager.register_analyzer(std::make_unique<SubroutineReferencesAnalyzer>());
    manager.register_analyzer(std::make_unique<ConstantPropagationAnalyzer>());
    manager.register_analyzer(std::make_unique<ReferenceAnalyzer>());
    manager.register_analyzer(std::make_unique<DataReferenceAnalyzer>());
    const auto result = manager.analyze();
    ASSERT_TRUE(result.completed);
    // Copied from the data-reference Ghidra Delta pointer rows.
    EXPECT_TRUE(std::any_of(context.references().begin(), context.references().end(), [](const Reference& reference) {
        return reference.source == 0x140002058 && reference.target == 0x140002048 &&
               reference.kind == ReferenceKind::data;
    }));
    EXPECT_TRUE(std::any_of(context.references().begin(), context.references().end(), [](const Reference& reference) {
        return reference.source == 0x140002060 && reference.target == 0x140002050 &&
               reference.kind == ReferenceKind::data;
    }));
}

} // namespace
} // namespace ghidra::analyzer::tests
