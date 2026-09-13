module;

#include <gtest/gtest.h>

export module function_body_tests;

import analyzer;
import std;

namespace ghidra::analyzer::tests {
namespace {

/// Loads a checked-in x64 fixture through the production PE/Sleigh providers.
[[nodiscard]] AnalysisContext load_fixture(std::string_view fixture) {
    const auto path = std::filesystem::path(ANALYZER_FIXTURE_DIR) / fixture / "tests" / "data" /
                      ("test_" + std::string(fixture) + ".exe");
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

} // namespace
} // namespace ghidra::analyzer::tests
