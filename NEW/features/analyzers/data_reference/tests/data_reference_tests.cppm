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
    std::set<std::pair<Address, Address>> actual;
    for (const auto& reference : context.references()) {
        if (reference.kind == ReferenceKind::data && reference.source >= 0x140002000 &&
            reference.source < 0x140002100) {
            actual.emplace(reference.source, reference.target);
        }
    }
    EXPECT_EQ(actual, (std::set<std::pair<Address, Address>>{{0x140002058, 0x140002048}, {0x140002060, 0x140002050}}));
    // Ghidra defines pointer data at the referenced pointer target, not at the
    // relocation cell that stores the pointer. This distinguishes the data
    // object contract from the reference source/target relation.
    ASSERT_TRUE(context.data().contains(0x140002048));
    ASSERT_TRUE(context.data().contains(0x140002050));
    EXPECT_EQ(context.data().at(0x140002048).size, 8U);
    EXPECT_EQ(context.data().at(0x140002050).size, 8U);
    EXPECT_EQ(context.data().at(0x140002048).type, "relocated pointer");
    EXPECT_EQ(context.data().at(0x140002050).type, "relocated pointer");
    EXPECT_FALSE(context.functions().contains(0x140002040));
    EXPECT_FALSE(context.functions().contains(0x140002048));
    EXPECT_FALSE(context.functions().contains(0x140002050));
}

} // namespace
} // namespace ghidra::analyzer::tests
