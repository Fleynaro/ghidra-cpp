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
    // Copied from the exact Ghidra operand rows. The set assertion verifies
    // source, target, and operand identity instead of hiding regressions behind
    // a reference count.
    std::set<std::tuple<Address, Address, std::size_t>> actual;
    for (const auto& reference : context.references()) {
        if (reference.kind == ReferenceKind::data) {
            actual.emplace(reference.source, reference.target, reference.operand_index.value_or(99U));
        }
    }
    const std::set<std::tuple<Address, Address, std::size_t>> expected{
        {0x140001000, 0x140003000, 1}, {0x140001015, 0x140003000, 0}, {0x14000101C, 0x140003000, 1},
        {0x140001069, 0x140003000, 0}, {0x140001070, 0x140003000, 1}, {0x14000107C, 0x140003000, 1},
        {0x140001086, 0x140003000, 0}};
    EXPECT_EQ(actual, expected);
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

/// Verifies a register-relative displacement is not mistaken for an absolute
/// image address when no typed operand value is available.
TEST(AnalyzerPipelineTest, IgnoresRegisterRelativeHexDisplacements) {
    auto context = load_fixture("disassemble_entry_points");
    context.options() = {};
    context.options().reference = true;
    context.options().constant_propagation = false;
    const auto executable =
        std::find_if(context.image().memory_regions().begin(), context.image().memory_regions().end(),
                     [](const pe::MemoryRegion& region) { return region.executable; });
    ASSERT_NE(executable, context.image().memory_regions().end());

    sleigh_runtime::Instruction instruction;
    instruction.address = executable->start + 0x300U;
    instruction.length = 1;
    instruction.operands.push_back(
        sleigh_runtime::Operand{"qword ptr [rsp + 0x8]", sleigh_runtime::OperandKind::memory, std::nullopt});
    ASSERT_TRUE(context.define_instruction(std::move(instruction)));

    AutoAnalysisManager manager(context);
    manager.register_analyzer(std::make_unique<ConstantPropagationAnalyzer>());
    manager.register_analyzer(std::make_unique<ReferenceAnalyzer>());
    const auto result = manager.analyze(std::array<Address, 1>{executable->start + 0x300U});
    ASSERT_TRUE(result.completed);
    EXPECT_TRUE(std::none_of(context.references().begin(), context.references().end(), [&](const Reference& reference) {
        return reference.source == executable->start + 0x300U;
    }));
}

} // namespace
} // namespace ghidra::analyzer::tests
