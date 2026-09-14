module;

#include <gtest/gtest.h>

export module function_body_tests;

import analyzer_test_support;
import std;

namespace ghidra::analyzer::tests {
namespace {

/// Verifies data references do not become control-flow edges during function creation.
TEST(AnalyzerPipelineTest, FunctionBodiesIgnoreDataReferences) {
    auto context = load_shared_fixture("function_body");
    context.options().seed_provider_functions = false;
    const auto executable =
        std::find_if(context.image().memory_regions().begin(), context.image().memory_regions().end(),
                     [](const pe::MemoryRegion& region) { return region.executable; });
    ASSERT_NE(executable, context.image().memory_regions().end());

    const Address entry = executable->start;
    const Address fallthrough = entry + 1;
    const Address data_target = entry + 0x20;
    sleigh_runtime::Instruction first;
    first.address = entry;
    first.length = 1;
    first.flow = {sleigh_runtime::FlowKind::none, std::nullopt, true, false};
    sleigh_runtime::Instruction second;
    second.address = fallthrough;
    second.length = 1;
    second.flow = {sleigh_runtime::FlowKind::return_op, std::nullopt, false, true};
    sleigh_runtime::Instruction pointed_to;
    pointed_to.address = data_target;
    pointed_to.length = 1;
    pointed_to.flow = {sleigh_runtime::FlowKind::return_op, std::nullopt, false, true};
    ASSERT_TRUE(context.define_instruction(std::move(first)));
    ASSERT_TRUE(context.define_instruction(std::move(second)));
    ASSERT_TRUE(context.define_instruction(std::move(pointed_to)));
    ASSERT_TRUE(context.add_reference(
        Reference{entry, data_target, ReferenceKind::data, 0, std::nullopt, FlowOverride::none, true}));
    ASSERT_TRUE(context.create_function(entry));

    const auto& function = context.functions().at(entry);
    EXPECT_TRUE(function.instruction_starts.contains(entry));
    EXPECT_TRUE(function.instruction_starts.contains(fallthrough));
    EXPECT_FALSE(function.instruction_starts.contains(data_target));
}

/// Verifies conditional branches split the block at both the target and fall-through leaders.
TEST(AnalyzerPipelineTest, BuildsConditionalBranchCfg) {
    auto context = load_shared_fixture("function_body");
    context.options().seed_provider_functions = false;
    const auto executable =
        std::find_if(context.image().memory_regions().begin(), context.image().memory_regions().end(),
                     [](const pe::MemoryRegion& region) { return region.executable; });
    ASSERT_NE(executable, context.image().memory_regions().end());
    const Address entry = executable->start;
    const Address fallthrough = entry + 1;
    const Address target = entry + 2;

    sleigh_runtime::Instruction branch;
    branch.address = entry;
    branch.length = 1;
    branch.flow = {sleigh_runtime::FlowKind::conditional_branch, sleigh_runtime::Varnode{"ram", target, 8}, true,
                   false};
    sleigh_runtime::Instruction fallthrough_instruction;
    fallthrough_instruction.address = fallthrough;
    fallthrough_instruction.length = 1;
    fallthrough_instruction.flow = {sleigh_runtime::FlowKind::return_op, std::nullopt, false, true};
    sleigh_runtime::Instruction target_instruction;
    target_instruction.address = target;
    target_instruction.length = 1;
    target_instruction.flow = {sleigh_runtime::FlowKind::return_op, std::nullopt, false, true};
    ASSERT_TRUE(context.define_instruction(std::move(branch)));
    ASSERT_TRUE(context.define_instruction(std::move(fallthrough_instruction)));
    ASSERT_TRUE(context.define_instruction(std::move(target_instruction)));
    ASSERT_TRUE(context.create_function(entry));

    const auto& function = context.functions().at(entry);
    ASSERT_EQ(function.blocks.size(), 3U);
    EXPECT_EQ(function.blocks[0].instructions, std::vector<Address>{entry});
    EXPECT_EQ(function.blocks[1].instructions, std::vector<Address>{fallthrough});
    EXPECT_EQ(function.blocks[2].instructions, std::vector<Address>{target});
    EXPECT_EQ(function.blocks[0].successors, (std::vector<Address>{target, fallthrough}));
    EXPECT_EQ(function.simple_blocks.size(), function.blocks.size());
}

/// Verifies explicitly discovered overlapping entries retain a shared code address.
TEST(AnalyzerPipelineTest, PreservesSharedFunctionBodies) {
    auto context = load_shared_fixture("function_body");
    context.options().seed_provider_functions = false;
    context.options().allow_shared_function_body = true;
    const auto executable =
        std::find_if(context.image().memory_regions().begin(), context.image().memory_regions().end(),
                     [](const pe::MemoryRegion& region) { return region.executable; });
    ASSERT_NE(executable, context.image().memory_regions().end());
    const Address first = executable->start;
    const Address shared = first + 1;
    sleigh_runtime::Instruction first_instruction;
    first_instruction.address = first;
    first_instruction.length = 1;
    sleigh_runtime::Instruction shared_instruction;
    shared_instruction.address = shared;
    shared_instruction.length = 1;
    shared_instruction.flow = {sleigh_runtime::FlowKind::return_op, std::nullopt, false, true};
    ASSERT_TRUE(context.define_instruction(std::move(first_instruction)));
    ASSERT_TRUE(context.define_instruction(std::move(shared_instruction)));
    ASSERT_TRUE(context.create_function(first));
    ASSERT_TRUE(context.create_function(shared));
    EXPECT_TRUE(context.functions().at(first).body.contains(shared));
    EXPECT_TRUE(context.functions().at(shared).body.contains(shared));
}

/// Verifies the default CreateFunctionCmd contract carves a newly discovered
/// entry out of an existing body instead of silently retaining overlapping
/// ownership when shared bodies are not requested.
TEST(AnalyzerPipelineTest, CarvesOverlappingFunctionBodyByDefault) {
    auto context = load_shared_fixture("function_body");
    context.options().seed_provider_functions = false;
    const auto executable =
        std::find_if(context.image().memory_regions().begin(), context.image().memory_regions().end(),
                     [](const pe::MemoryRegion& region) { return region.executable; });
    ASSERT_NE(executable, context.image().memory_regions().end());
    const Address first = executable->start;
    const Address shared = first + 1;

    sleigh_runtime::Instruction first_instruction;
    first_instruction.address = first;
    first_instruction.length = 1;
    first_instruction.flow = {sleigh_runtime::FlowKind::none, std::nullopt, true, false};
    sleigh_runtime::Instruction shared_instruction;
    shared_instruction.address = shared;
    shared_instruction.length = 1;
    shared_instruction.flow = {sleigh_runtime::FlowKind::return_op, std::nullopt, false, true};
    ASSERT_TRUE(context.define_instruction(std::move(first_instruction)));
    ASSERT_TRUE(context.define_instruction(std::move(shared_instruction)));
    ASSERT_TRUE(context.create_function(first));
    ASSERT_TRUE(context.functions().at(first).body.contains(shared));
    ASSERT_TRUE(context.create_function(shared));
    EXPECT_FALSE(context.functions().at(first).body.contains(shared));
    EXPECT_TRUE(context.functions().at(shared).body.contains(shared));
}

/// Verifies that overlap carving remains authoritative when an existing
/// function reaches shared interior code through a branch that does not pass
/// through the newly created function entry. Recomputing the old body after
/// insertion would incorrectly reclaim the shared address.
TEST(AnalyzerPipelineTest, PreservesCarvingAgainstInteriorSharedReachability) {
    auto context = load_shared_fixture("function_body");
    context.options().seed_provider_functions = false;
    const auto executable =
        std::find_if(context.image().memory_regions().begin(), context.image().memory_regions().end(),
                     [](const pe::MemoryRegion& region) { return region.executable; });
    ASSERT_NE(executable, context.image().memory_regions().end());

    const Address first = executable->start;
    const Address existing_branch = first + 1U;
    const Address new_entry = first + 0x10U;
    const Address shared = first + 0x20U;

    sleigh_runtime::Instruction first_instruction;
    first_instruction.address = first;
    first_instruction.length = 1;
    first_instruction.flow = {sleigh_runtime::FlowKind::none, std::nullopt, true, false};
    sleigh_runtime::Instruction existing_branch_instruction;
    existing_branch_instruction.address = existing_branch;
    existing_branch_instruction.length = 1;
    existing_branch_instruction.flow = {sleigh_runtime::FlowKind::conditional_branch,
                                        sleigh_runtime::Varnode{"ram", shared, 8}, false, false};
    sleigh_runtime::Instruction new_entry_instruction;
    new_entry_instruction.address = new_entry;
    new_entry_instruction.length = 1;
    new_entry_instruction.flow = {sleigh_runtime::FlowKind::conditional_branch,
                                  sleigh_runtime::Varnode{"ram", shared, 8}, false, false};
    sleigh_runtime::Instruction shared_instruction;
    shared_instruction.address = shared;
    shared_instruction.length = 1;
    shared_instruction.flow = {sleigh_runtime::FlowKind::return_op, std::nullopt, false, true};

    ASSERT_TRUE(context.define_instruction(std::move(first_instruction)));
    ASSERT_TRUE(context.define_instruction(std::move(existing_branch_instruction)));
    ASSERT_TRUE(context.define_instruction(std::move(new_entry_instruction)));
    ASSERT_TRUE(context.define_instruction(std::move(shared_instruction)));
    static_cast<void>(context.add_reference(Reference{existing_branch, shared, ReferenceKind::conditional_jump,
                                                      std::nullopt, std::nullopt, FlowOverride::none, false}));
    static_cast<void>(context.add_reference(Reference{new_entry, shared, ReferenceKind::conditional_jump, std::nullopt,
                                                      std::nullopt, FlowOverride::none, false}));

    ASSERT_TRUE(context.create_function(first));
    ASSERT_TRUE(context.create_function(new_entry));
    EXPECT_FALSE(context.functions().at(first).body.contains(shared));
    EXPECT_TRUE(context.functions().at(new_entry).body.contains(shared));
}

/// Verifies that a branch with a p-code memory store is not promoted to a
/// thunk. Ghidra's CreateThunkFunctionCmd rejects STORE side effects even when
/// the final control flow has a resolved destination.
TEST(AnalyzerPipelineTest, RejectsThunkWithMemorySideEffect) {
    auto context = load_shared_fixture("function_body");
    context.options().seed_provider_functions = false;
    const auto executable =
        std::find_if(context.image().memory_regions().begin(), context.image().memory_regions().end(),
                     [](const pe::MemoryRegion& region) { return region.executable; });
    ASSERT_NE(executable, context.image().memory_regions().end());

    const Address entry = executable->start;
    const Address target = entry + 0x10U;
    sleigh_runtime::Instruction branch;
    branch.address = entry;
    branch.length = 1;
    branch.flow = {sleigh_runtime::FlowKind::branch, sleigh_runtime::Varnode{"ram", target, 8}, false, true};
    sleigh_runtime::PcodeOp store;
    store.opcode = sleigh_runtime::PcodeOpcode::store;
    branch.pcode.push_back(store);
    sleigh_runtime::Instruction destination;
    destination.address = target;
    destination.length = 1;
    destination.flow = {sleigh_runtime::FlowKind::return_op, std::nullopt, false, true};

    ASSERT_TRUE(context.define_instruction(std::move(branch)));
    ASSERT_TRUE(context.define_instruction(std::move(destination)));
    static_cast<void>(context.add_reference(Reference{entry, target, ReferenceKind::unconditional_jump, std::nullopt,
                                                      std::nullopt, FlowOverride::none, false}));
    ASSERT_TRUE(context.create_function(entry));
    EXPECT_FALSE(context.functions().at(entry).thunk);
    EXPECT_TRUE(context.functions().at(entry).body.contains(target));
}

/// Verifies CreateFunctionCmd rejects an entry that would split an existing
/// instruction, preserving one coherent code-unit ownership boundary.
TEST(AnalyzerPipelineTest, RejectsOffcutFunctionEntry) {
    auto context = load_shared_fixture("function_body");
    const auto executable =
        std::find_if(context.image().memory_regions().begin(), context.image().memory_regions().end(),
                     [](const pe::MemoryRegion& region) { return region.executable; });
    ASSERT_NE(executable, context.image().memory_regions().end());
    sleigh_runtime::Instruction instruction;
    instruction.address = executable->start;
    instruction.length = 2;
    instruction.flow = {sleigh_runtime::FlowKind::return_op, std::nullopt, false, true};
    ASSERT_TRUE(context.define_instruction(std::move(instruction)));
    ASSERT_TRUE(context.create_function(executable->start));
    EXPECT_FALSE(context.create_function(executable->start + 1));
}

/// Verifies call destinations are excluded, recursive conditional flow is
/// visited once, and a CALL_RETURN override removes the post-call path.
TEST(AnalyzerPipelineTest, HandlesCallsLoopsAndNoReturnBodyRepair) {
    auto context = load_shared_fixture("function_body");
    context.options().seed_provider_functions = false;
    const auto executable =
        std::find_if(context.image().memory_regions().begin(), context.image().memory_regions().end(),
                     [](const pe::MemoryRegion& region) { return region.executable; });
    ASSERT_NE(executable, context.image().memory_regions().end());

    const Address entry = executable->start + 0x100U;
    const Address post_call = entry + 1U;
    const Address loop_branch = entry + 2U;
    const Address loop_body = entry + 3U;
    const Address terminal = entry + 4U;
    const Address callee = entry + 0x20U;
    const auto instruction = [](Address address, sleigh_runtime::FlowKind kind, std::optional<Address> target,
                                bool fallthrough, bool is_terminal) {
        sleigh_runtime::Instruction result;
        result.address = address;
        result.length = 1;
        result.flow = {kind, target ? std::optional{sleigh_runtime::Varnode{"ram", *target, 8}} : std::nullopt,
                       fallthrough, is_terminal};
        return result;
    };

    ASSERT_TRUE(context.define_instruction(instruction(entry, sleigh_runtime::FlowKind::call, callee, true, false)));
    ASSERT_TRUE(
        context.define_instruction(instruction(post_call, sleigh_runtime::FlowKind::none, std::nullopt, true, false)));
    ASSERT_TRUE(context.define_instruction(
        instruction(loop_branch, sleigh_runtime::FlowKind::conditional_branch, terminal, true, false)));
    ASSERT_TRUE(context.define_instruction(
        instruction(loop_body, sleigh_runtime::FlowKind::conditional_branch, loop_branch, true, false)));
    ASSERT_TRUE(context.define_instruction(
        instruction(terminal, sleigh_runtime::FlowKind::return_op, std::nullopt, false, true)));
    ASSERT_TRUE(context.define_instruction(
        instruction(callee, sleigh_runtime::FlowKind::return_op, std::nullopt, false, true)));
    ASSERT_TRUE(context.create_function(entry));
    const auto& normal = context.functions().at(entry);
    EXPECT_TRUE(normal.body.contains(post_call));
    EXPECT_TRUE(normal.body.contains(loop_body));
    EXPECT_FALSE(normal.body.contains(callee));
    EXPECT_EQ(normal.instruction_starts.size(), 5U);
    EXPECT_TRUE(context.functions().at(entry).blocks.size() >= 2U);

    ASSERT_TRUE(context.set_flow_override(entry, FlowOverride::call_return, callee));
    ASSERT_TRUE(context.rebuild_function_body(entry));
    const auto& repaired = context.functions().at(entry);
    EXPECT_EQ(repaired.instruction_starts, (std::set<Address>{entry}));
    EXPECT_FALSE(repaired.body.contains(post_call));
    EXPECT_FALSE(repaired.body.contains(loop_branch));
    EXPECT_FALSE(repaired.body.contains(loop_body));
    EXPECT_FALSE(repaired.body.contains(callee));
}

} // namespace
} // namespace ghidra::analyzer::tests
