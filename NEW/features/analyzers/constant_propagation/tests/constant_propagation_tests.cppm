module;

#include <gtest/gtest.h>

export module constant_propagation_tests;

import analyzer_constant_propagation;
import analyzer_function_body;
import analyzer_subroutine_references;
import analyzer_test_support;
import std;

namespace ghidra::analyzer::tests {
namespace {

/// Verifies p-code COPY and integer addition produce a stable constant fact.
TEST(AnalyzerPipelineTest, PropagatesPcodeArithmetic) {
    auto context = load_fixture("disassemble_entry_points");
    const auto executable =
        std::find_if(context.image().memory_regions().begin(), context.image().memory_regions().end(),
                     [](const pe::MemoryRegion& region) { return region.executable; });
    ASSERT_NE(executable, context.image().memory_regions().end());
    const Address first_address = executable->start;
    context.options().seed_provider_functions = false;
    sleigh_runtime::Instruction first;
    first.address = first_address;
    first.length = 1;
    first.pcode.push_back(sleigh_runtime::PcodeOp{sleigh_runtime::PcodeOpcode::copy,
                                                  sleigh_runtime::Varnode{"register", 0, 8},
                                                  {sleigh_runtime::Varnode{"const", 7, 8}},
                                                  std::nullopt});
    sleigh_runtime::Instruction second;
    second.address = first_address + 1;
    second.length = 1;
    second.pcode.push_back(
        sleigh_runtime::PcodeOp{sleigh_runtime::PcodeOpcode::int_add,
                                sleigh_runtime::Varnode{"register", 0, 8},
                                {sleigh_runtime::Varnode{"register", 0, 8}, sleigh_runtime::Varnode{"const", 3, 8}},
                                std::nullopt});
    sleigh_runtime::Instruction third;
    third.address = first_address + 2;
    third.length = 1;
    third.pcode.push_back(sleigh_runtime::PcodeOp{sleigh_runtime::PcodeOpcode::int_negate,
                                                  sleigh_runtime::Varnode{"register", 8, 8},
                                                  {sleigh_runtime::Varnode{"const", 0x0f, 8}},
                                                  std::nullopt});
    sleigh_runtime::Instruction fourth;
    fourth.address = first_address + 3;
    fourth.length = 1;
    fourth.pcode.push_back(
        sleigh_runtime::PcodeOp{sleigh_runtime::PcodeOpcode::int_left,
                                sleigh_runtime::Varnode{"register", 16, 8},
                                {sleigh_runtime::Varnode{"const", 1, 8}, sleigh_runtime::Varnode{"const", 64, 8}},
                                std::nullopt});
    ASSERT_TRUE(context.define_instruction(std::move(first)));
    ASSERT_TRUE(context.define_instruction(std::move(second)));
    ASSERT_TRUE(context.define_instruction(std::move(third)));
    ASSERT_TRUE(context.define_instruction(std::move(fourth)));
    ASSERT_TRUE(context.create_function(first_address));
    context.options() = {};
    AutoAnalysisManager manager(context);
    manager.register_analyzer(std::make_unique<SubroutineReferencesAnalyzer>());
    manager.register_analyzer(std::make_unique<FunctionBodyAnalyzer>());
    manager.register_analyzer(std::make_unique<ConstantPropagationAnalyzer>());
    const auto result = manager.analyze(std::array<Address, 1>{first_address});
    ASSERT_TRUE(result.completed);
    EXPECT_TRUE(std::any_of(context.constant_facts().begin(), context.constant_facts().end(),
                            [](const ConstantFact& fact) { return fact.value == 10; }));
    EXPECT_TRUE(
        std::any_of(context.constant_facts().begin(), context.constant_facts().end(), [](const ConstantFact& fact) {
            return fact.location.offset == 8 && fact.value == ~std::uint64_t{0x0f};
        }));
    EXPECT_TRUE(std::any_of(context.constant_facts().begin(), context.constant_facts().end(),
                            [](const ConstantFact& fact) { return fact.location.offset == 16 && fact.value == 0; }));
}

} // namespace
} // namespace ghidra::analyzer::tests
