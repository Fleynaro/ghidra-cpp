module;

#include <gtest/gtest.h>

export module constant_propagation_tests;

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
