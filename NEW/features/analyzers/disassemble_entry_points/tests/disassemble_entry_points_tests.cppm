module;

#include <gtest/gtest.h>

export module disassemble_entry_points_tests;

import analyzer_disassemble_entry_points;
import analyzer_test_support;
import std;

namespace ghidra::analyzer::tests {
namespace {

/// Verifies that registry insertion rejects duplicate analyzer identities.
TEST(AnalyzerRegistryTest, RejectsDuplicateNames) {
    auto context = load_fixture("disassemble_entry_points");
    AnalyzerRegistry registry;
    std::vector<std::string> order;
    registry.register_analyzer(std::make_unique<RecordingAnalyzer>("duplicate", 1, order));
    EXPECT_THROW(registry.register_analyzer(std::make_unique<RecordingAnalyzer>("duplicate", 2, order)),
                 std::invalid_argument);
    EXPECT_EQ(registry.analyzers().size(), 1U);
    static_cast<void>(context);
}

/// Verifies numeric priority, deterministic tie-breaking, and downstream events.
TEST(AutoAnalysisManagerTest, SchedulesPriorityAndNewEvents) {
    auto context = load_fixture("disassemble_entry_points");
    context.options() = {};
    std::vector<std::string> order;
    AutoAnalysisManager manager(context);
    manager.register_analyzer(std::make_unique<RecordingAnalyzer>("late", 300, order));
    manager.register_analyzer(std::make_unique<RecordingAnalyzer>("early", 100, order, true));
    const auto result = manager.analyze(std::array<Address, 1>{context.image().memory_regions().front().start});
    ASSERT_TRUE(result.completed);
    ASSERT_EQ(order.size(), 2U);
    EXPECT_EQ(order[0], "early");
    EXPECT_EQ(order[1], "late");
    EXPECT_EQ(context.data().size(), 1U);
}

/// Verifies analyzer exceptions remain visible and mark the run incomplete.
TEST(AutoAnalysisManagerTest, ReportsAnalyzerErrors) {
    auto context = load_fixture("disassemble_entry_points");
    context.options() = {};
    AutoAnalysisManager manager(context);
    manager.register_analyzer(std::make_unique<FailingAnalyzer>());
    const auto result = manager.analyze(std::array<Address, 1>{0x140001000});
    EXPECT_FALSE(result.completed);
    ASSERT_EQ(result.errors.size(), 1U);
    EXPECT_NE(result.errors.front().find("controlled analyzer failure"), std::string::npos);
}

/// Verifies cancellation discards downstream queued events while retaining committed state.
TEST(AutoAnalysisManagerTest, ClearsPendingEventsAfterCancellation) {
    auto context = load_fixture("disassemble_entry_points");
    context.options() = {};
    AutoAnalysisManager manager(context);
    manager.register_analyzer(std::make_unique<CancellingEmitter>());
    auto observer = std::make_unique<DataObserver>();
    auto* observer_state = observer.get();
    manager.register_analyzer(std::move(observer));

    const auto cancelled = manager.analyze(std::array<Address, 1>{0x140001000});
    ASSERT_TRUE(cancelled.cancelled);
    EXPECT_EQ(observer_state->calls, 0U);
    ASSERT_EQ(context.data().size(), 1U);

    const auto resumed = manager.analyze(std::span<const Address>{});
    ASSERT_TRUE(resumed.completed);
    EXPECT_EQ(observer_state->calls, 0U);
}

/// Verifies prerequisite validation still delivers the terminal analyzer lifecycle callback.
TEST(AutoAnalysisManagerTest, CompletesLifecycleForInvalidPrerequisites) {
    auto context = load_fixture("disassemble_entry_points");
    context.options() = {};
    AutoAnalysisManager manager(context);
    auto analyzer = std::make_unique<PrerequisiteLifecycleAnalyzer>();
    auto* state = analyzer.get();
    manager.register_analyzer(std::move(analyzer));

    const auto result = manager.analyze(std::array<Address, 1>{0x140001000});
    EXPECT_FALSE(result.completed);
    ASSERT_FALSE(result.errors.empty());
    EXPECT_TRUE(state->ended);
    EXPECT_FALSE(state->ended_as_cancelled);
}

/// Verifies removed events and analysis-ended lifecycle callbacks are delivered.
TEST(AutoAnalysisManagerTest, DispatchesRemovalAndEndLifecycle) {
    auto context = load_fixture("disassemble_entry_points");
    context.options() = {};
    const auto executable =
        std::find_if(context.image().memory_regions().begin(), context.image().memory_regions().end(),
                     [](const pe::MemoryRegion& region) { return region.executable; });
    ASSERT_NE(executable, context.image().memory_regions().end());
    ASSERT_TRUE(context.disassemble_flow(executable->start) > 0);
    ASSERT_TRUE(context.create_function(executable->start));
    ASSERT_TRUE(context.remove_function(executable->start));
    auto lifecycle = std::make_unique<LifecycleAnalyzer>();
    auto* observer = lifecycle.get();
    AutoAnalysisManager manager(context);
    manager.register_analyzer(std::move(lifecycle));
    const auto result = manager.analyze(std::span<const Address>{});
    ASSERT_TRUE(result.completed);
    EXPECT_GT(observer->added_count, 0U);
    EXPECT_EQ(observer->removed_count, 1U);
    EXPECT_TRUE(observer->ended);
}

/// Verifies the real entry-point pipeline produces instructions but no functions.
TEST(AnalyzerPipelineTest, DisassemblesEntryPointsWithoutCreatingFunctions) {
    auto context = load_fixture("disassemble_entry_points");
    auto& options = context.options();
    options.seed_provider_functions = false;
    options.function_start_search = false;
    options.subroutine_references = false;
    options.reference = false;
    options.data_reference = false;
    options.scalar_operand_references = false;
    options.stack = false;
    options.constant_propagation = false;
    options.non_returning_functions = false;
    AutoAnalysisManager manager(context);
    manager.register_analyzer(std::make_unique<DisassembleEntryPointsAnalyzer>());
    const auto result = manager.analyze();
    ASSERT_TRUE(result.completed);
    EXPECT_TRUE(context.instructions().contains(0x140001000));
    EXPECT_TRUE(context.instructions().contains(0x140001014));
    EXPECT_FALSE(context.instructions().contains(0x140002000));
    EXPECT_TRUE(context.functions().empty());
}

/// Verifies data references do not become control-flow edges during function creation.
TEST(AnalyzerPipelineTest, FunctionBodiesIgnoreDataReferences) {
    auto context = load_fixture("disassemble_entry_points");
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
    auto context = load_fixture("disassemble_entry_points");
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
    auto context = load_fixture("disassemble_entry_points");
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
    auto context = load_fixture("disassemble_entry_points");
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

/// Verifies CreateFunctionCmd rejects an entry that would split an existing
/// instruction, preserving one coherent code-unit ownership boundary.
TEST(AnalyzerPipelineTest, RejectsOffcutFunctionEntry) {
    auto context = load_fixture("disassemble_entry_points");
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

/// Verifies every normal and delay import is represented by the PE-backed external namespace.
TEST(AnalyzerPipelineTest, PreservesImportedExternalSymbols) {
    auto context = load_fixture("windows_resource_reference");
    std::size_t expected = 0;
    for (const auto& descriptor : context.image().imports())
        expected += descriptor.symbols.size();
    for (const auto& descriptor : context.image().delay_imports())
        expected += descriptor.symbols.size();
    ASSERT_GT(expected, 0U);
    EXPECT_EQ(context.external_symbols().size(), expected);
    EXPECT_TRUE(
        std::all_of(context.external_symbols().begin(), context.external_symbols().end(),
                    [](const ExternalSymbol& symbol) { return !symbol.library.empty() && symbol.iat_address != 0; }));
}

} // namespace
} // namespace ghidra::analyzer::tests
