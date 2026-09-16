module;

#include <gtest/gtest.h>

export module x86_constant_reference_tests;

import analyzer_test_support;
import analyzer_x86_constant_reference;
import std;

namespace ghidra::analyzer::tests {
namespace {

/// Verifies an LEA immediate becomes one DATA reference at the mapped image address.
TEST(X86ConstantReferenceAnalyzerTest, AddsLeaDataReference) {
    auto context = load_fixture("x86_constant_reference");
    const auto executable =
        std::find_if(context.image().memory_regions().begin(), context.image().memory_regions().end(),
                     [](const pe::MemoryRegion& region) { return region.executable; });
    ASSERT_NE(executable, context.image().memory_regions().end());
    const Address instruction_address = executable->start;
    sleigh_runtime::Instruction instruction;
    instruction.address = instruction_address;
    instruction.length = 7;
    instruction.mnemonic = "LEA";
    instruction.operands = {{"rax", sleigh_runtime::OperandKind::register_value, std::nullopt},
                            {"[0x3000]", sleigh_runtime::OperandKind::address, 0x3000U}};
    ASSERT_TRUE(context.define_instruction(std::move(instruction)));
    CancellationToken cancellation;
    X86ConstantReferenceAnalyzer analyzer;
    analyzer.analyze(context, {}, cancellation);

    ASSERT_EQ(context.references().size(), 1U);
    EXPECT_EQ(context.references().front().source, instruction_address);
    EXPECT_EQ(context.references().front().target, context.image().optional_header().image_base + 0x3000U);
    EXPECT_EQ(context.references().front().kind, ReferenceKind::data);
}

/// Verifies small constants and non-LEA instructions are rejected without observable references.
TEST(X86ConstantReferenceAnalyzerTest, RejectsNonPointerCandidates) {
    auto context = load_fixture("x86_constant_reference");
    const auto executable =
        std::find_if(context.image().memory_regions().begin(), context.image().memory_regions().end(),
                     [](const pe::MemoryRegion& region) { return region.executable; });
    ASSERT_NE(executable, context.image().memory_regions().end());
    sleigh_runtime::Instruction instruction;
    instruction.address = executable->start;
    instruction.length = 1;
    instruction.mnemonic = "MOV";
    instruction.operands = {{"eax", sleigh_runtime::OperandKind::register_value, std::nullopt},
                            {"1", sleigh_runtime::OperandKind::immediate, 1U}};
    ASSERT_TRUE(context.define_instruction(std::move(instruction)));
    CancellationToken cancellation;
    X86ConstantReferenceAnalyzer analyzer;
    analyzer.analyze(context, {}, cancellation);
    EXPECT_TRUE(context.references().empty());
}

} // namespace
} // namespace ghidra::analyzer::tests
