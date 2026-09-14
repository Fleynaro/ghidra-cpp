module;

#include <gtest/gtest.h>

export module reference_tests;

import analyzer_constant_propagation;
import analyzer_disassemble_entry_points;
import analyzer_reference;
import analyzer_subroutine_references;
import analyzer_test_support;
import std;

namespace ghidra::analyzer::tests {
namespace {

/// Verifies p-code LOAD/STORE references match the four keyed rows in the Reference fixture.
TEST(AnalyzerPipelineTest, MaterializesMemoryReferences) {
    auto context = load_fixture("reference");
    auto& options = context.options();
    options.function_start_search = false;
    options.subroutine_references = false;
    options.data_reference = false;
    options.scalar_operand_references = false;
    options.stack = false;
    options.constant_propagation = false;
    options.non_returning_functions = false;
    AutoAnalysisManager manager(context);
    manager.register_analyzer(std::make_unique<DisassembleEntryPointsAnalyzer>());
    manager.register_analyzer(std::make_unique<SubroutineReferencesAnalyzer>());
    manager.register_analyzer(std::make_unique<ConstantPropagationAnalyzer>());
    manager.register_analyzer(std::make_unique<ReferenceAnalyzer>());
    const auto result = manager.analyze();
    ASSERT_TRUE(result.completed);
    // Copied from the reference Ghidra Delta: four data references after analysis.
    EXPECT_EQ(std::count_if(context.references().begin(), context.references().end(),
                            [](const Reference& reference) { return reference.kind == ReferenceKind::data; }),
              4);
}

/// Verifies unrestricted repeat analysis requeues existing listing state and
/// remains idempotent instead of reseeding only the PE entry point.
TEST(AnalyzerPipelineTest, ReAnalyzeAllIsIdempotentForExistingListing) {
    auto context = load_fixture("reference");
    auto& options = context.options();
    options.function_start_search = false;
    options.subroutine_references = false;
    options.data_reference = false;
    options.scalar_operand_references = false;
    options.stack = false;
    options.constant_propagation = false;
    options.non_returning_functions = false;
    AutoAnalysisManager manager(context);
    manager.register_analyzer(std::make_unique<DisassembleEntryPointsAnalyzer>());
    manager.register_analyzer(std::make_unique<SubroutineReferencesAnalyzer>());
    manager.register_analyzer(std::make_unique<ConstantPropagationAnalyzer>());
    manager.register_analyzer(std::make_unique<ReferenceAnalyzer>());
    ASSERT_TRUE(manager.analyze().completed);
    const auto instruction_count = context.instructions().size();
    const auto reference_count = context.references().size();
    const auto repeated = manager.re_analyze_all();
    ASSERT_TRUE(repeated.completed);
    EXPECT_EQ(context.instructions().size(), instruction_count);
    EXPECT_EQ(context.references().size(), reference_count);
}

} // namespace
} // namespace ghidra::analyzer::tests
