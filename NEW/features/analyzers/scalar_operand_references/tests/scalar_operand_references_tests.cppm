module;

#include <gtest/gtest.h>

export module scalar_operand_references_tests;

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
    manager.register_analyzer(std::make_unique<DisassembleEntryPointsAnalyzer>());
    manager.register_analyzer(std::make_unique<SubroutineReferencesAnalyzer>());
    manager.register_analyzer(std::make_unique<FunctionBodyAnalyzer>());
    manager.register_analyzer(std::make_unique<ConstantPropagationAnalyzer>());
    manager.register_analyzer(std::make_unique<ScalarOperandReferencesAnalyzer>());
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
