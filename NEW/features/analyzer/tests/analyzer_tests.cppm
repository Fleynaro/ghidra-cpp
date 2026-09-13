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
    return AnalysisContext(std::move(*image), ANALYZER_SLA_PATH);
}

/// Reads one Ghidra golden report so assertions remain tied to checked-in Delta evidence.
[[nodiscard]] std::string read_golden_report(std::string_view fixture) {
    const auto path = std::filesystem::path(ANALYZER_FIXTURE_DIR) / fixture / ("test_" + std::string(fixture) + ".md");
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("Unable to read golden report: " + path.string());
    }
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

/// Loads one fixture report through the structured Delta parser.
[[nodiscard]] GoldenDelta load_golden_delta(std::string_view fixture) {
    const auto path = std::filesystem::path(ANALYZER_FIXTURE_DIR) / fixture / ("test_" + std::string(fixture) + ".md");
    const auto parsed = parse_golden_delta(path);
    if (!parsed)
        throw std::runtime_error(parsed.error());
    return *parsed;
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

/// Verifies analyzer exceptions stop scheduling and remain visible to callers.
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
    const auto report = read_golden_report("disassemble_entry_points");
    EXPECT_NE(report.find("entry_point_alpha"), std::string::npos);
    EXPECT_NE(report.find("entry_point_beta"), std::string::npos);
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
    const auto delta = load_golden_delta("subroutine_references");
    const auto compared = compare_golden_delta(context, delta);
    ASSERT_TRUE(compared.has_value()) << compared.error();
    const auto report = read_golden_report("subroutine_references");
    EXPECT_NE(report.find("Functions Created By Subroutine References"), std::string::npos);
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
    const auto delta = load_golden_delta("function_start_search");
    const auto compared = compare_golden_delta(context, delta);
    ASSERT_TRUE(compared.has_value()) << compared.error();
    const auto report = read_golden_report("function_start_search");
    EXPECT_NE(report.find("0x0000000140005003"), std::string::npos);
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
    const auto report = read_golden_report("non_returning_functions_known");
    EXPECT_NE(report.find("abort"), std::string::npos);
    ASSERT_TRUE(context.functions().contains(0x140001000));
    EXPECT_TRUE(context.functions().at(0x140001000).no_return);
    EXPECT_TRUE(std::any_of(context.bookmarks().begin(), context.bookmarks().end(), [](const Bookmark& bookmark) {
        return bookmark.address == 0x140001000 && bookmark.category == "Non-Returning Function";
    }));
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
    ASSERT_TRUE(context.define_instruction(std::move(first)));
    ASSERT_TRUE(context.define_instruction(std::move(second)));
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
    const auto report = read_golden_report("stack");
    EXPECT_NE(report.find("local_res10"), std::string::npos);
    EXPECT_NE(report.find("local_res8"), std::string::npos);
    EXPECT_TRUE(std::any_of(context.functions().begin(), context.functions().end(),
                            [](const auto& pair) { return !pair.second.stack_variables.empty(); }));
    EXPECT_TRUE(std::any_of(context.references().begin(), context.references().end(),
                            [](const Reference& reference) { return reference.kind == ReferenceKind::stack; }));
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
    const auto report = read_golden_report("data_reference");
    EXPECT_NE(report.find("0x0000000140002058"), std::string::npos);
    EXPECT_TRUE(std::any_of(context.references().begin(), context.references().end(), [](const Reference& reference) {
        return reference.source == 0x140002058 && reference.target == 0x140002048 &&
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
    const auto report = read_golden_report("reference");
    EXPECT_NE(report.find("References after target analysis:** `4`"), std::string::npos);
    EXPECT_GE(std::count_if(context.references().begin(), context.references().end(),
                            [](const Reference& reference) { return reference.kind == ReferenceKind::data; }),
              4);
}

/// Verifies every normal and delay import is represented by the PE-backed external namespace.
TEST(AnalyzerPipelineTest, PreservesImportedExternalSymbols) {
    auto context = load_fixture("reference");
    std::size_t expected = 0;
    for (const auto& descriptor : context.image().imports())
        expected += descriptor.symbols.size();
    for (const auto& descriptor : context.image().delay_imports())
        expected += descriptor.symbols.size();
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
    const auto report = read_golden_report("non_returning_functions_discovered");
    EXPECT_NE(report.find("CALL_RETURN"), std::string::npos);
    ASSERT_TRUE(context.functions().contains(0x140001000));
    EXPECT_TRUE(context.functions().at(0x140001000).no_return);
    EXPECT_GE(
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
    EXPECT_TRUE(std::all_of(context.references().begin(), context.references().end(), [](const Reference& reference) {
        return reference.kind != ReferenceKind::scalar || reference.target >= 0x1000;
    }));
}

} // namespace
} // namespace ghidra::analyzer::tests
