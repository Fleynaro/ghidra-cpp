module;

#include <gtest/gtest.h>

export module recode.tests.replay;

import recode.core;
import recode.runtime.event_bus;
import recode.runtime.event_store.log;
import recode.runtime.projections.coordinator;
import recode.runtime.projections.software_model;
import recode.runtime.storage.projection;
import std;

namespace recode::tests::replay {
namespace {

namespace event_bus = recode::runtime::event_bus;
namespace event_store = recode::runtime::event_store;
namespace projections = recode::runtime::projections;
namespace storage = recode::runtime::storage;

/// Rebuilds a listing projection from a fresh event-log reader and checks its durable checkpoint.
TEST(ReplayArchitectureTest, RebuildRestoresListingAndCheckpoint) {
    const auto directory = std::filesystem::temp_directory_path() / "recode-architecture-replay";
    std::error_code error;
    std::filesystem::remove_all(directory, error);
    std::filesystem::create_directories(directory, error);
    const core::ProjectId project{"replay-integration"};
    auto store = event_store::AppendOnlyLog::open(directory / "events.log");
    ASSERT_TRUE(store);
    auto projection = std::make_shared<projections::SoftwareModelProjection>(project);
    auto sqlite = std::make_shared<storage::SqliteProjectionStore>(directory / "projection.sqlite");
    ASSERT_TRUE(sqlite->open(project));
    auto bus = std::make_shared<event_bus::EventBus>();
    projections::ProjectionCoordinator coordinator{*store, projection, sqlite, bus};
    core::Instruction instruction;
    instruction.key = core::InstructionKey{core::EntityId{"replay-instruction"}, {core::AddressSpaceId{"ram"}, 0x1000}};
    instruction.length = 1;
    instruction.mnemonic = "ret";
    instruction.assembly = "ret";
    const auto draft = core::events::listing_state_changed(project, instruction, core::CorrelationId{"replay"});
    ASSERT_TRUE(coordinator.commit(project, std::span{&draft, 1}));
    ASSERT_EQ(sqlite->checkpoint(project)->value, 1U);
    auto fresh = std::make_shared<projections::SoftwareModelProjection>(project);
    const auto history = (*store)->read(project, core::Revision{1});
    ASSERT_TRUE(history);
    ASSERT_TRUE(fresh->rebuild(history->events));
    EXPECT_EQ(fresh->instruction_at(instruction.key.address)->mnemonic, "ret");
    std::filesystem::remove_all(directory, error);
}

} // namespace
} // namespace recode::tests::replay
