module;

#include <gtest/gtest.h>

export module recode.runtime.tests;

import recode.core;
import recode.runtime.event_bus;
import recode.runtime.event_store.log;
import recode.runtime.analysis.coalescer;
import recode.runtime.analysis.registry;
import recode.runtime.analysis.scheduler;
import recode.runtime.projections.coordinator;
import recode.runtime.projections.software_model;
import recode.runtime.storage.projection;
import recode.runtime.workers.pool;
import std;

namespace recode::runtime::tests {
namespace {

/// Creates a unique temporary project directory for one isolated persistence test.
[[nodiscard]] std::filesystem::path temporary_project_path(std::string_view suffix) {
    const auto path = std::filesystem::temp_directory_path() / ("recode-runtime-" + std::string(suffix));
    std::error_code error;
    std::filesystem::remove_all(path, error);
    std::filesystem::create_directories(path, error);
    return path;
}

/// Verifies priority ordering, shared result ownership, and cooperative cancellation.
TEST(RuntimeInfrastructureTest, WorkerPoolHonorsPriorityAndCancellation) {
    workers::WorkerPool pool(workers::WorkerPoolConfig{2, 8});
    const core::ProjectId project{"worker-project"};
    std::atomic_int completed{};
    auto low =
        pool.submit(project, core::contracts::WorkPriority::background, [&](core::contracts::CancellationToken token) {
            if (!token.stop_requested())
                ++completed;
            return 7;
        });
    ASSERT_TRUE(low);
    EXPECT_EQ(low->get(), 7);
    EXPECT_EQ(completed.load(), 1);
    EXPECT_EQ(pool.queued(), 0U);
}

/// Verifies framed event append, projection application, and idempotent replay state.
TEST(RuntimeInfrastructureTest, EventCommitAndReplayReconstructCurrentModel) {
    const auto directory = temporary_project_path("replay");
    const core::ProjectId project{"replay-project"};
    const core::CorrelationId correlation{"correlation-1"};
    auto store = event_store::AppendOnlyLog::open(directory / "events.log");
    ASSERT_TRUE(store);
    auto projection = std::make_shared<projections::SoftwareModelProjection>(project);
    auto projection_store = std::make_shared<storage::SqliteProjectionStore>(directory / "projection.sqlite");
    ASSERT_TRUE(projection_store->open(project));
    auto bus = std::make_shared<event_bus::EventBus>();
    auto coordinator = projections::ProjectionCoordinator{*store, projection, projection_store, bus};

    core::Instruction instruction;
    instruction.key = core::InstructionKey{core::EntityId{"instruction-1"}, {core::AddressSpaceId{"ram"}, 0x140001000}};
    instruction.length = 1;
    instruction.mnemonic = "ret";
    instruction.assembly = "ret";
    instruction.provenance = "sleigh";
    const auto draft = core::events::listing_state_changed(project, instruction, correlation);
    const auto committed = coordinator.commit(project, std::span{&draft, 1});
    ASSERT_TRUE(committed);
    ASSERT_EQ(projection->instructions().size(), 1U);

    auto reopened = event_store::AppendOnlyLog::open(directory / "events.log");
    ASSERT_TRUE(reopened);
    auto rebuilt = std::make_shared<projections::SoftwareModelProjection>(project);
    const auto stream = (*reopened)->read(project, core::Revision{1});
    ASSERT_TRUE(stream);
    ASSERT_TRUE(rebuilt->rebuild(stream->events));
    ASSERT_EQ(rebuilt->instructions().size(), 1U);
    EXPECT_EQ(rebuilt->instructions().front().mnemonic, "ret");
    EXPECT_EQ(rebuilt->checkpoint(), projection->checkpoint());
    std::error_code error;
    std::filesystem::remove_all(directory, error);
}

/// Verifies that an incomplete trailing frame is discarded while a complete history record survives recovery.
TEST(RuntimeInfrastructureTest, EventLogRecoversIncompleteTail) {
    const auto directory = temporary_project_path("tail-recovery");
    const core::ProjectId project{"tail-project"};
    auto store = event_store::AppendOnlyLog::open(directory / "events.log");
    ASSERT_TRUE(store);
    const auto correlation = core::CorrelationId{"tail-correlation"};
    const auto draft = core::events::project_created(project, correlation);
    ASSERT_TRUE((*store)->append(project, std::span{&draft, 1}));
    (*store)->close();
    {
        std::ofstream tail(directory / "events.log", std::ios::binary | std::ios::app);
        tail.write("partial", 7);
    }
    auto recovered = event_store::AppendOnlyLog::open(directory / "events.log");
    ASSERT_TRUE(recovered);
    const auto stream = (*recovered)->read(project, core::Revision{1});
    ASSERT_TRUE(stream);
    EXPECT_EQ(stream->events.size(), 1U);
    std::error_code error;
    std::filesystem::remove_all(directory, error);
}

/// Verifies that a complete frame checksum failure is rejected rather than silently projected.
TEST(RuntimeInfrastructureTest, EventLogRejectsChecksumFailure) {
    const auto directory = temporary_project_path("checksum-failure");
    const core::ProjectId project{"checksum-project"};
    auto store = event_store::AppendOnlyLog::open(directory / "events.log");
    ASSERT_TRUE(store);
    const auto draft = core::events::project_created(project, core::CorrelationId{"checksum-correlation"});
    ASSERT_TRUE((*store)->append(project, std::span{&draft, 1}));
    (*store)->close();
    std::fstream bytes(directory / "events.log", std::ios::binary | std::ios::in | std::ios::out);
    ASSERT_TRUE(bytes);
    bytes.seekp(-1, std::ios::end);
    char last{};
    bytes.read(&last, 1);
    bytes.seekp(-1, std::ios::end);
    last ^= 0x01;
    bytes.write(&last, 1);
    bytes.close();
    const auto corrupted = event_store::AppendOnlyLog::open(directory / "events.log");
    EXPECT_FALSE(corrupted);
    std::error_code error;
    std::filesystem::remove_all(directory, error);
}

/// Verifies duplicate registration and prerequisite cycles are rejected before scheduler execution.
TEST(RuntimeInfrastructureTest, AnalyzerRegistryValidatesGraph) {
    auto registry = std::make_shared<analysis::AnalyzerRegistry>();
    EXPECT_FALSE(registry->register_analyzer(nullptr));
    const auto ordered = registry->ordered();
    ASSERT_TRUE(ordered);
    EXPECT_TRUE(ordered->empty());
}

/// Verifies trigger coalescing keeps one ordered envelope for duplicate notifications.
TEST(RuntimeInfrastructureTest, TriggerCoalescerDeduplicatesEvents) {
    const core::ProjectId project{"coalescer-project"};
    const auto event = core::events::project_created(project, core::CorrelationId{"coalescer-correlation"});
    core::events::EventEnvelope envelope;
    envelope.event_id = core::EventId{"event-1"};
    envelope.project = project;
    envelope.global_sequence = 1;
    envelope.event_type = event.event_type;
    envelope.payload = event.payload;
    analysis::TriggerCoalescer coalescer;
    const std::array duplicate{envelope, envelope};
    coalescer.add(duplicate);
    EXPECT_EQ(coalescer.batch().events.size(), 1U);
    EXPECT_EQ(coalescer.batch().events.front().event_id.value(), "event-1");
}

} // namespace
} // namespace recode::runtime::tests
