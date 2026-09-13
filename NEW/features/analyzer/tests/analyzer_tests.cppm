module;

#include <gtest/gtest.h>

export module analyzer_tests;

import analyzer;
import std;

namespace ghidra::analyzer::tests {
namespace {

/// Loads a checked-in x64 fixture through the production PE/Sleigh providers.
[[nodiscard]] AnalysisContext load_fixture(std::string_view fixture) {
    const auto path = std::filesystem::path(ANALYZER_FIXTURE_DIR) / fixture / ("test_" + std::string(fixture) + ".exe");
    auto image = pe::PeLoader::load_file(path);
    if (!image) {
        throw std::runtime_error(image.error().message);
    }
    return AnalysisContext(std::move(*image), "x86-64.sla");
}

/// Describes a function body copied from the fixture's Ghidra Delta report.
struct ExpectedFunctionBody {
    Address entry{};
    std::vector<AddressRange> ranges;
};

/// Verifies complete byte ranges without reading a report at test runtime.
void expect_function_bodies(const AnalysisContext& context, std::span<const ExpectedFunctionBody> expected) {
    for (const auto& row : expected) {
        const auto function = context.function_at(row.entry);
        ASSERT_NE(function, nullptr) << "missing function at 0x" << std::hex << row.entry;
        EXPECT_EQ(function->body_ranges, row.ranges);
    }
}

/// Records scheduler order and emits a data event from the first callback.
class RecordingAnalyzer final : public Analyzer {
public:
    /// Creates a recorder with an externally visible name and priority.
    RecordingAnalyzer(std::string name, std::int32_t priority, std::vector<std::string>& order, bool emit_data = false)
        : name_(std::move(name)), priority_(priority), order_(order), emit_data_(emit_data) {}

    /// Returns the recorder's deterministic scheduling contract.
    [[nodiscard]] AnalyzerDescriptor descriptor() const override {
        return {name_, priority_, {EventKind::memory_added}, {}};
    }

    /// Records execution and optionally emits a new state event.
    void analyze(AnalysisContext& context, std::span<const AnalysisEvent> events, CancellationToken&) override {
        order_.push_back(name_);
        if (emit_data_ && !events.empty()) {
            static_cast<void>(context.add_data(DataObject{events.front().addresses.front(), 1, "test"}));
        }
    }

private:
    std::string name_;
    std::int32_t priority_;
    std::vector<std::string>& order_;
    bool emit_data_;
};

/// Raises a controlled analyzer error so manager failure propagation is tested.
class FailingAnalyzer final : public Analyzer {
public:
    /// Returns a minimal memory-triggered descriptor for the failure test.
    [[nodiscard]] AnalyzerDescriptor descriptor() const override {
        return {"failing", 100, {EventKind::memory_added}, {}};
    }

    /// Throws a deterministic error that must be reported by the manager.
    void analyze(AnalysisContext&, std::span<const AnalysisEvent>, CancellationToken&) override {
        throw std::runtime_error("controlled analyzer failure");
    }
};

/// Observes add/remove lifecycle dispatch and the manager end callback.
class LifecycleAnalyzer final : public Analyzer {
public:
    /// Returns a function-change-triggered lifecycle descriptor.
    [[nodiscard]] AnalyzerDescriptor descriptor() const override {
        return {"lifecycle", 100, {EventKind::function_changed}, {}};
    }

    /// Records ordinary changed-state delivery.
    void analyze(AnalysisContext&, std::span<const AnalysisEvent>, CancellationToken&) override {
        ++added_count;
    }

    /// Records removed-state delivery separately from additions.
    void removed(AnalysisContext&, std::span<const AnalysisEvent>, CancellationToken&) override {
        ++removed_count;
    }

    /// Records that all scheduler work has drained.
    void analysis_ended(AnalysisContext&, bool) override {
        ended = true;
    }

    std::size_t added_count{};
    std::size_t removed_count{};
    bool ended{};
};

/// Cancels after emitting a downstream event so the manager's pending-event cleanup
/// can be verified without losing the mutation already committed to the context.
class CancellingEmitter final : public Analyzer {
public:
    /// Returns a memory-triggered descriptor for the cancellation regression test.
    [[nodiscard]] AnalyzerDescriptor descriptor() const override {
        return {"cancelling-emitter", 100, {EventKind::memory_added}, {}};
    }

    /// Emits data and then requests cancellation before downstream work can run.
    void analyze(AnalysisContext& context, std::span<const AnalysisEvent> events,
                 CancellationToken& cancellation) override {
        if (!events.empty()) {
            static_cast<void>(context.add_data(DataObject{events.front().addresses.front(), 1, "cancelled"}));
        }
        cancellation.cancel();
    }
};

/// Counts downstream callbacks that must not be replayed after cancellation.
class DataObserver final : public Analyzer {
public:
    /// Returns a data-triggered descriptor for pending-event cleanup verification.
    [[nodiscard]] AnalyzerDescriptor descriptor() const override {
        return {"data-observer", 200, {EventKind::data_added}, {}};
    }

    /// Records every data event dispatched to this analyzer.
    void analyze(AnalysisContext&, std::span<const AnalysisEvent>, CancellationToken&) override {
        ++calls;
    }

    std::size_t calls{};
};

/// Records terminal lifecycle notification when prerequisite validation rejects a run.
class PrerequisiteLifecycleAnalyzer final : public Analyzer {
public:
    /// Returns a descriptor with an intentionally missing prerequisite.
    [[nodiscard]] AnalyzerDescriptor descriptor() const override {
        return {"invalid-prerequisite", 100, {EventKind::memory_added}, {"missing-analyzer"}};
    }

    /// This callback must not run when prerequisite validation fails.
    void analyze(AnalysisContext&, std::span<const AnalysisEvent>, CancellationToken&) override {
        FAIL() << "analyzer ran despite a missing prerequisite";
    }

    /// Records the failed run's terminal lifecycle notification.
    void analysis_ended(AnalysisContext&, bool cancelled) override {
        ended = true;
        ended_as_cancelled = cancelled;
    }

    bool ended{};
    bool ended_as_cancelled{};
};

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
    options.function_body = false;
    options.reference = false;
    options.data_reference = false;
    options.scalar_operand_references = false;
    options.stack = false;
    options.constant_propagation = false;
    options.non_returning_functions = false;
    AutoAnalysisManager manager(context);
    manager.register_builtin_analyzers();
    const auto result = manager.analyze();
    ASSERT_TRUE(result.completed);
    EXPECT_TRUE(context.instructions().contains(0x140001000));
    EXPECT_TRUE(context.instructions().contains(0x140001014));
    EXPECT_FALSE(context.instructions().contains(0x140002000));
    EXPECT_TRUE(context.functions().empty());
}

/// Verifies direct call references create functions and bodies through the event chain.
TEST(AnalyzerPipelineTest, DirectCallsCreateFunctionsAndCfg) {
    auto context = load_fixture("subroutine_references");
    auto& options = context.options();
    options.function_start_search = false;
    options.reference = false;
    options.data_reference = false;
    options.scalar_operand_references = false;
    options.stack = false;
    options.constant_propagation = false;
    options.non_returning_functions = false;
    AutoAnalysisManager manager(context);
    manager.register_builtin_analyzers();
    const auto result = manager.analyze();
    ASSERT_TRUE(result.completed);
    ASSERT_TRUE(context.instructions().contains(0x140001040));
    ASSERT_TRUE(context.image().is_executable(0x140001000));
    ASSERT_TRUE(context.instructions().at(0x140001040).instruction.flow.target.has_value());
    // Copied from the subroutine-reference Ghidra Delta rows for added/changed bodies.
    const std::array<ExpectedFunctionBody, 5> expected{{{0x140001000, {{0x140001000, 0x14000100A}}},
                                                        {0x140001014, {{0x140001014, 0x14000101E}}},
                                                        {0x140001028, {{0x140001028, 0x140001032}}},
                                                        {0x14000103C, {{0x14000103C, 0x14000106A}}},
                                                        {0x140001074, {{0x140001074, 0x1400010A4}}}}};
    expect_function_bodies(context, expected);
    EXPECT_TRUE(std::any_of(context.references().begin(), context.references().end(), [](const Reference& reference) {
        return reference.source == 0x140001040 && reference.target == 0x140001000 &&
               reference.kind == ReferenceKind::unconditional_call;
    }));
    EXPECT_TRUE(context.functions().contains(0x140001000));
    EXPECT_TRUE(context.functions().contains(0x140001014));
    EXPECT_TRUE(context.functions().contains(0x140001028));
    EXPECT_GE(context.functions().at(0x140001000).body.size(), 2U);
    EXPECT_GE(context.functions().at(0x140001014).body.size(), 2U);
    EXPECT_GE(context.functions().at(0x140001028).body.size(), 2U);
    EXPECT_TRUE(std::all_of(context.functions().begin(), context.functions().end(),
                            [](const auto& pair) { return !pair.second.blocks.empty(); }));
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

/// Verifies the pattern phase creates the positive function-start candidate from the golden fixture.
TEST(AnalyzerPipelineTest, PatternSearchCreatesPositiveCandidate) {
    auto context = load_fixture("function_start_search");
    for (const Address existing : {0x140001000ULL, 0x140001080ULL, 0x140001100ULL}) {
        ASSERT_TRUE(context.create_function(existing));
    }
    auto& options = context.options();
    options.disassemble_entry_points = true;
    options.pattern_root = std::filesystem::path(ANALYZER_FIXTURE_DIR)
                               .lexically_normal()
                               .parent_path()
                               .parent_path()
                               .parent_path()
                               .parent_path() /
                           "Ghidra/Processors/x86/data/patterns";
    options.subroutine_references = false;
    options.function_body = false;
    options.reference = false;
    options.data_reference = false;
    options.scalar_operand_references = false;
    options.stack = false;
    options.constant_propagation = false;
    options.non_returning_functions = false;
    AutoAnalysisManager manager(context);
    manager.register_builtin_analyzers();
    const auto result = manager.analyze();
    ASSERT_TRUE(result.completed);
    // Copied from the function-start Ghidra Delta row for the positive pattern.
    EXPECT_TRUE(context.functions().contains(0x140005003));
    EXPECT_TRUE(std::any_of(context.bookmarks().begin(), context.bookmarks().end(), [](const Bookmark& bookmark) {
        return bookmark.address == 0x140005003 && bookmark.category == "Function Start Search";
    }));
}

/// Verifies known PE no-return names create the target function and bookmark.
TEST(AnalyzerPipelineTest, KnownNoReturnFunctionsAreMarked) {
    auto context = load_fixture("non_returning_functions_known");
    auto& options = context.options();
    options.function_start_search = false;
    options.subroutine_references = false;
    options.function_body = false;
    options.reference = false;
    options.data_reference = false;
    options.scalar_operand_references = false;
    options.stack = false;
    options.constant_propagation = false;
    AutoAnalysisManager manager(context);
    manager.register_builtin_analyzers();
    const auto result = manager.analyze();
    ASSERT_TRUE(result.completed);
    // Copied from the known-no-return Ghidra Delta: abort is the only known row.
    ASSERT_TRUE(context.functions().contains(0x140001000));
    EXPECT_TRUE(context.functions().at(0x140001000).no_return);
    EXPECT_TRUE(std::any_of(context.bookmarks().begin(), context.bookmarks().end(), [](const Bookmark& bookmark) {
        return bookmark.address == 0x140001000 && bookmark.category == "Non-Returning Function";
    }));
    EXPECT_TRUE(std::all_of(context.functions().begin(), context.functions().end(),
                            [](const auto& pair) { return pair.first == 0x140001000 || !pair.second.no_return; }));
}

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

/// Verifies unrestricted repeat analysis requeues existing listing state and
/// remains idempotent instead of reseeding only the PE entry point.
TEST(AnalyzerPipelineTest, ReAnalyzeAllIsIdempotentForExistingListing) {
    auto context = load_fixture("reference");
    auto& options = context.options();
    options.function_start_search = false;
    options.subroutine_references = false;
    options.function_body = false;
    options.data_reference = false;
    options.scalar_operand_references = false;
    options.stack = false;
    options.constant_propagation = false;
    options.non_returning_functions = false;
    AutoAnalysisManager manager(context);
    manager.register_builtin_analyzers();
    ASSERT_TRUE(manager.analyze().completed);
    const auto instruction_count = context.instructions().size();
    const auto reference_count = context.references().size();
    const auto repeated = manager.re_analyze_all();
    ASSERT_TRUE(repeated.completed);
    EXPECT_EQ(context.instructions().size(), instruction_count);
    EXPECT_EQ(context.references().size(), reference_count);
}

/// Verifies stack analysis consumes real PE/Sleigh operands and reports golden local names.
TEST(AnalyzerPipelineTest, FindsStackVariablesAndReferences) {
    auto context = load_fixture("stack");
    auto& options = context.options();
    options.function_start_search = false;
    options.reference = false;
    options.data_reference = false;
    options.scalar_operand_references = false;
    options.constant_propagation = false;
    options.non_returning_functions = false;
    AutoAnalysisManager manager(context);
    manager.register_builtin_analyzers();
    const auto result = manager.analyze();
    ASSERT_TRUE(result.completed);
    // Copied from the stack Ghidra Delta stack-variable rows.
    EXPECT_TRUE(std::any_of(context.functions().begin(), context.functions().end(), [](const auto& pair) {
        return std::any_of(pair.second.stack_variables.begin(), pair.second.stack_variables.end(),
                           [](const StackVariable& variable) { return variable.name == "local_res10"; });
    }));
    EXPECT_TRUE(std::any_of(context.functions().begin(), context.functions().end(), [](const auto& pair) {
        return std::any_of(pair.second.stack_variables.begin(), pair.second.stack_variables.end(),
                           [](const StackVariable& variable) { return variable.name == "local_res8"; });
    }));
    EXPECT_TRUE(std::any_of(context.functions().begin(), context.functions().end(),
                            [](const auto& pair) { return !pair.second.stack_variables.empty(); }));
    EXPECT_TRUE(std::any_of(context.references().begin(), context.references().end(),
                            [](const Reference& reference) { return reference.kind == ReferenceKind::stack; }));
    const std::array<std::pair<Address, std::int64_t>, 6> expected_stack{{{0x140001003, 0x10},
                                                                          {0x140001007, 0x10},
                                                                          {0x14000100E, 0x8},
                                                                          {0x140001018, 0x8},
                                                                          {0x140001024, 0x8},
                                                                          {0x140001028, 0x10}}};
    for (const auto& [source, offset] : expected_stack) {
        EXPECT_TRUE(
            std::any_of(context.references().begin(), context.references().end(), [&](const Reference& reference) {
                return reference.kind == ReferenceKind::stack && reference.source == source &&
                       reference.stack_offset == offset;
            }));
    }
}

/// Verifies Data Reference scans pointer-sized PE data cells after its prerequisite event.
TEST(AnalyzerPipelineTest, FollowsDataSectionPointers) {
    auto context = load_fixture("data_reference");
    auto& options = context.options();
    options.disassemble_entry_points = false;
    options.function_start_search = false;
    options.subroutine_references = false;
    options.function_body = false;
    options.reference = false;
    options.scalar_operand_references = false;
    options.stack = false;
    options.constant_propagation = false;
    options.non_returning_functions = false;
    AutoAnalysisManager manager(context);
    manager.register_builtin_analyzers();
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

/// Verifies p-code LOAD/STORE references match the four keyed rows in the Reference fixture.
TEST(AnalyzerPipelineTest, MaterializesMemoryReferences) {
    auto context = load_fixture("reference");
    auto& options = context.options();
    options.function_start_search = false;
    options.subroutine_references = false;
    options.function_body = false;
    options.data_reference = false;
    options.scalar_operand_references = false;
    options.stack = false;
    options.constant_propagation = false;
    options.non_returning_functions = false;
    AutoAnalysisManager manager(context);
    manager.register_builtin_analyzers();
    const auto result = manager.analyze();
    ASSERT_TRUE(result.completed);
    // Copied from the reference Ghidra Delta: four data references after analysis.
    EXPECT_EQ(std::count_if(context.references().begin(), context.references().end(),
                            [](const Reference& reference) { return reference.kind == ReferenceKind::data; }),
              4);
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

/// Verifies three independent post-call indicators mark a discovered no-return target.
TEST(AnalyzerPipelineTest, DiscoversNoReturnFromCallEvidence) {
    auto context = load_fixture("non_returning_functions_discovered");
    auto& options = context.options();
    options.function_start_search = false;
    options.subroutine_references = true;
    options.function_body = true;
    options.reference = false;
    options.data_reference = false;
    options.scalar_operand_references = false;
    options.stack = false;
    options.constant_propagation = false;
    AutoAnalysisManager manager(context);
    manager.register_builtin_analyzers();
    const auto result = manager.analyze();
    ASSERT_TRUE(result.completed);
    // Copied from the discovered-no-return Ghidra Delta: exactly three CALL_RETURN rows.
    ASSERT_TRUE(context.functions().contains(0x140001000));
    EXPECT_TRUE(context.functions().at(0x140001000).no_return);
    EXPECT_EQ(
        std::count_if(context.references().begin(), context.references().end(),
                      [](const Reference& reference) { return reference.flow_override == FlowOverride::call_return; }),
        3);
}

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
    manager.register_builtin_analyzers();
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
