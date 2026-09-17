module;

#include <gtest/gtest.h>

export module ghidra.services.debugger.win_ttd.tests;

import ghidra.core;
import ghidra.service.debugger.win_ttd;
import std;

namespace {
namespace api = ghidra::core::contracts;
namespace model = ghidra::core::debugger;

/// Verifies that the replay service exposes the specialized contract rather than the live contract.
TEST(WinTtdReplayContractTest, FactoryCreatesReplaySession) {
    const auto service = ghidra::services::debugger::win_ttd::create_win_ttd();
    ASSERT_TRUE(service) << service.error().message;
    const auto session = (*service)->create_session();
    ASSERT_TRUE(session) << session.error().message;
    EXPECT_EQ((*session)->state(), model::SessionState::created);
    static_assert(std::derived_from<api::IReplayDebugSession, api::IBaseDebugSession>);
    static_assert(!std::derived_from<api::IReplayDebugSession, api::ILiveDebugSession>);
}

/// Verifies invalid trace paths fail before native engine initialization and preserve a useful diagnostic.
TEST(WinTtdReplayContractTest, RejectsMissingTrace) {
    const auto service = ghidra::services::debugger::win_ttd::create_win_ttd();
    ASSERT_TRUE(service);
    const auto session = (*service)->create_session();
    ASSERT_TRUE(session);
    const auto opened = (*session)->open_trace(std::filesystem::path{"missing-test-trace.run"});
#if defined(NEW_GHIDRA_HAS_TTD_REPLAY)
    ASSERT_FALSE(opened);
    EXPECT_EQ(opened.error().code, ghidra::core::DiagnosticCode::invalid_argument);
#else
    ASSERT_FALSE(opened);
    EXPECT_EQ(opened.error().code, ghidra::core::DiagnosticCode::unsupported);
#endif
}

/// Verifies replay-only operations remain explicit and never mutate a closed session.
TEST(WinTtdReplayContractTest, ClosedSessionReportsLifecycleConflict) {
    const auto service = ghidra::services::debugger::win_ttd::create_win_ttd();
    ASSERT_TRUE(service);
    const auto session = (*service)->create_session();
    ASSERT_TRUE(session);
    const auto position = (*session)->position();
#if defined(NEW_GHIDRA_HAS_TTD_REPLAY)
    ASSERT_FALSE(position);
    EXPECT_EQ(position.error().code, ghidra::core::DiagnosticCode::conflict);
#else
    ASSERT_FALSE(position);
    EXPECT_EQ(position.error().code, ghidra::core::DiagnosticCode::unsupported);
#endif
}

/// Replays a caller-supplied real trace through contracts and checks the shared state queries.
TEST(WinTtdReplayIntegrationTest, OpensAndNavigatesProvidedTrace) {
#if !defined(NEW_GHIDRA_HAS_TTD_REPLAY)
    GTEST_SKIP() << "Microsoft TTD Replay API package is not available";
#else
    const char* trace_name = std::getenv("TTD_TEST_TRACE");
    const bool required = std::getenv("TTD_TEST_REQUIRED") != nullptr;
    if (trace_name == nullptr || *trace_name == '\0') {
        if (required)
            FAIL() << "TTD_TEST_REQUIRED is set, but TTD_TEST_TRACE is missing. Record a real .run first.";
        GTEST_SKIP() << "Set TTD_TEST_TRACE to a finalized .run file for native integration coverage";
    }
    if (!std::filesystem::is_regular_file(std::filesystem::path{trace_name})) {
        if (required)
            FAIL() << "TTD_TEST_REQUIRED is set, but TTD_TEST_TRACE is not a file: " << trace_name;
        GTEST_SKIP() << "TTD_TEST_TRACE does not name an existing finalized .run file: " << trace_name;
    }
    const auto service = ghidra::services::debugger::win_ttd::create_win_ttd();
    ASSERT_TRUE(service) << service.error().message;
    const auto session = (*service)->create_session();
    ASSERT_TRUE(session) << session.error().message;
    ASSERT_TRUE((*session)->open_trace(std::filesystem::path{trace_name}));
    const auto info = (*session)->trace_info();
    ASSERT_TRUE(info) << info.error().message;
    EXPECT_LE(info->lifetime.first, info->lifetime.last);
    EXPECT_GT(info->thread_count, 0U);
    EXPECT_GT(info->module_count, 0U);
    ASSERT_TRUE((*session)->seek(info->lifetime.first));
    const auto first_position = (*session)->position();
    ASSERT_TRUE(first_position) << first_position.error().message;
    EXPECT_EQ(*first_position, info->lifetime.first);
    const auto threads = (*session)->threads();
    ASSERT_TRUE(threads) << threads.error().message;
    ASSERT_FALSE(threads->empty());
    ASSERT_TRUE((*session)->select_thread(threads->front().id));
    const auto selected_thread = (*session)->current_thread();
    ASSERT_TRUE(selected_thread) << selected_thread.error().message;
    EXPECT_EQ(*selected_thread, threads->front().id);
    const auto registers = (*session)->registers(*selected_thread);
    ASSERT_TRUE(registers) << registers.error().message;
    ASSERT_FALSE(registers->empty());
    const auto register_values = (*session)->read_registers(*selected_thread);
    ASSERT_TRUE(register_values) << register_values.error().message;
    EXPECT_EQ(register_values->size(), registers->size());
    const auto instruction_pointer = (*session)->instruction_pointer(*selected_thread);
    ASSERT_TRUE(instruction_pointer) << instruction_pointer.error().message;
    const auto memory = (*session)->read_memory(*instruction_pointer, 1);
    ASSERT_TRUE(memory) << memory.error().message;
    EXPECT_EQ(memory->transferred_size, 1U);
    const auto execute_watchpoint =
        (*session)->seek_watchpoint(*instruction_pointer, 1, model::WatchpointAccess::execute, true);
    ASSERT_TRUE(execute_watchpoint) << execute_watchpoint.error().message;
    EXPECT_EQ(execute_watchpoint->reason, ghidra::core::replay::StopReason::watchpoint);
    const auto modules = (*session)->modules();
    ASSERT_TRUE(modules) << modules.error().message;
    ASSERT_FALSE(modules->empty());
    const auto regions = (*session)->memory_regions();
    ASSERT_TRUE(regions) << regions.error().message;
    ASSERT_FALSE(regions->empty());
    const auto forward = (*session)->step_forward();
    ASSERT_TRUE(forward) << forward.error().message;
    EXPECT_NE(forward->position, forward->previous_position);
    ASSERT_TRUE((*session)->seek(forward->position));
    const auto repeated_position = (*session)->position();
    ASSERT_TRUE(repeated_position) << repeated_position.error().message;
    EXPECT_EQ(*repeated_position, forward->position);
    ASSERT_TRUE((*session)->seek(info->lifetime.last));
    const auto backward = (*session)->step_backward();
    ASSERT_TRUE(backward) << backward.error().message;
    EXPECT_NE(backward->position, backward->previous_position);
    ASSERT_TRUE((*session)->close_trace());
#endif
}

#if defined(NEW_GHIDRA_HAS_TTD_REPLAY)

/// Opens the one caller-provided trace once per focused test and closes its cursor deterministically.
class ReplayTraceFixture : public ::testing::Test {
protected:
    /// Loads the real trace selected by TTD_TEST_TRACE before each focused contract assertion.
    void SetUp() override {
        const char* trace_name = std::getenv("TTD_TEST_TRACE");
        const bool required = std::getenv("TTD_TEST_REQUIRED") != nullptr;
        if (trace_name == nullptr || *trace_name == '\0') {
            if (required)
                GTEST_FAIL() << "TTD_TEST_REQUIRED is set, but TTD_TEST_TRACE is missing. Record a real .run first.";
            GTEST_SKIP() << "Set TTD_TEST_TRACE to a finalized .run file for native integration coverage";
        }
        if (!std::filesystem::is_regular_file(std::filesystem::path{trace_name})) {
            if (required)
                GTEST_FAIL() << "TTD_TEST_REQUIRED is set, but TTD_TEST_TRACE is not a file: " << trace_name;
            GTEST_SKIP() << "TTD_TEST_TRACE does not name an existing finalized .run file: " << trace_name;
        }
        trace_path_ = trace_name;
        const auto service = ghidra::services::debugger::win_ttd::create_win_ttd();
        ASSERT_TRUE(service) << service.error().message;
        const auto created = (*service)->create_session();
        ASSERT_TRUE(created) << created.error().message;
        session_ = *created;
        ASSERT_TRUE(session_->open_trace(std::filesystem::path{trace_name}));
        const auto metadata = session_->trace_info();
        ASSERT_TRUE(metadata) << metadata.error().message;
        metadata_ = *metadata;
    }

    /// Closes the trace so the API's cursor is destroyed before its engine.
    void TearDown() override {
        if (session_)
            ASSERT_TRUE(session_->close_trace());
    }

    std::shared_ptr<api::IReplayDebugSession> session_;
    ghidra::core::replay::TraceInfo metadata_;
    std::filesystem::path trace_path_;
};

/// Verifies reopening replaces a trace only after the old cursor is destroyed safely.
TEST_F(ReplayTraceFixture, ReopensTraceWithSafeLifetimeOrder) {
    ASSERT_TRUE(session_->open_trace(trace_path_));
    const auto reopened = session_->trace_info();
    ASSERT_TRUE(reopened) << reopened.error().message;
    EXPECT_EQ(reopened->process_id, metadata_.process_id);
}

/// Verifies trace metadata has meaningful process, timeline, thread, and module boundaries.
TEST_F(ReplayTraceFixture, ReportsMetadataAndProcess) {
    EXPECT_GT(metadata_.process_id, 0U);
    EXPECT_GT(metadata_.thread_count, 0U);
    EXPECT_GT(metadata_.module_count, 0U);
    EXPECT_LE(metadata_.lifetime.first, metadata_.lifetime.last);
    const auto process = session_->process();
    ASSERT_TRUE(process) << process.error().message;
    EXPECT_EQ(process->id.value, std::to_string(metadata_.process_id));
    const auto processes = session_->processes();
    ASSERT_TRUE(processes) << processes.error().message;
    ASSERT_EQ(processes->size(), 1U);
}

/// Verifies active thread selection changes the context used by register and instruction queries.
TEST_F(ReplayTraceFixture, EnumeratesAndSelectsThreads) {
    const auto threads = session_->threads();
    ASSERT_TRUE(threads) << threads.error().message;
    ASSERT_FALSE(threads->empty());
    ASSERT_TRUE(session_->select_thread(threads->front().id));
    EXPECT_EQ(session_->current_thread().value(), threads->front().id);
    const auto instruction = session_->instruction_pointer(threads->front().id);
    ASSERT_TRUE(instruction) << instruction.error().message;
    EXPECT_NE(instruction->offset, 0U);
    for (const auto& thread : *threads) {
        const auto thread_instruction = session_->instruction_pointer(thread.id);
        ASSERT_TRUE(thread_instruction) << thread_instruction.error().message;
        const auto thread_rip = session_->read_register("rip", thread.id);
        ASSERT_TRUE(thread_rip) << thread_rip.error().message;
    }
}

/// Verifies scalar register snapshots and memory reads are available at a selected replay position.
TEST_F(ReplayTraceFixture, ReadsRegistersAndMemory) {
    const auto registers = session_->read_registers();
    ASSERT_TRUE(registers) << registers.error().message;
    ASSERT_GE(registers->size(), 10U);
    const auto instruction = session_->instruction_pointer();
    ASSERT_TRUE(instruction) << instruction.error().message;
    const auto memory = session_->read_memory(*instruction, 1);
    ASSERT_TRUE(memory) << memory.error().message;
    EXPECT_EQ(memory->transferred_size, 1U);
    const auto invalid = session_->read_memory(*instruction, 0);
    ASSERT_FALSE(invalid);
    EXPECT_EQ(invalid.error().code, ghidra::core::DiagnosticCode::invalid_argument);
}

/// Verifies module snapshots and conservative memory-region projections are non-empty.
TEST_F(ReplayTraceFixture, EnumeratesModulesAndRegions) {
    const auto modules = session_->modules();
    ASSERT_TRUE(modules) << modules.error().message;
    ASSERT_FALSE(modules->empty());
    EXPECT_TRUE(std::ranges::any_of(
        *modules, [](const model::Module& module) { return module.base.offset != 0U && module.size != 0U; }));
    const auto regions = session_->memory_regions();
    ASSERT_TRUE(regions) << regions.error().message;
    EXPECT_FALSE(regions->empty());
}

/// Verifies position round-tripping and deterministic forward/backward navigation.
TEST_F(ReplayTraceFixture, SeeksAndReplaysBothDirections) {
    ASSERT_TRUE(session_->seek(metadata_.lifetime.first));
    const auto first = session_->position();
    ASSERT_TRUE(first) << first.error().message;
    const auto forward = session_->step_forward();
    ASSERT_TRUE(forward) << forward.error().message;
    EXPECT_NE(forward->position, forward->previous_position);
    ASSERT_TRUE(session_->seek(forward->position));
    EXPECT_EQ(session_->position().value(), forward->position);
    ASSERT_TRUE(session_->seek(metadata_.lifetime.last));
    const auto backward = session_->step_backward();
    ASSERT_TRUE(backward) << backward.error().message;
    EXPECT_NE(backward->position, backward->previous_position);
    const auto zero = session_->step_forward(0);
    ASSERT_FALSE(zero);
    EXPECT_EQ(zero.error().code, ghidra::core::DiagnosticCode::invalid_argument);
}

/// Verifies execute watchpoint navigation uses a temporary cursor and reports its stop reason.
TEST_F(ReplayTraceFixture, FindsExecuteWatchpointWithoutMovingUserCursorFirst) {
    ASSERT_TRUE(session_->seek(metadata_.lifetime.first));
    const auto before = session_->position();
    ASSERT_TRUE(before);
    const auto instruction = session_->instruction_pointer();
    ASSERT_TRUE(instruction);
    const auto hit = session_->seek_watchpoint(*instruction, 1, model::WatchpointAccess::execute, true);
    ASSERT_TRUE(hit) << hit.error().message;
    EXPECT_EQ(hit->reason, ghidra::core::replay::StopReason::watchpoint);
    EXPECT_EQ(session_->position().value(), *before);
}

/// Verifies callback-shaped event APIs do not pretend to provide an empty event stream.
TEST_F(ReplayTraceFixture, ReportsUnsupportedEventPollingExplicitly) {
    const auto events = session_->poll_events();
    ASSERT_FALSE(events);
    EXPECT_EQ(events.error().code, ghidra::core::DiagnosticCode::unsupported);
    const auto sink = session_->set_event_sink([](const model::DebugEvent&) {});
    ASSERT_FALSE(sink);
    EXPECT_EQ(sink.error().code, ghidra::core::DiagnosticCode::unsupported);
}

/// Verifies invalid positions and unsupported symbol/stack operations fail explicitly.
TEST_F(ReplayTraceFixture, RejectsInvalidAndUnsupportedOperations) {
    const auto invalid = session_->seek({std::numeric_limits<std::uint64_t>::max(), 0});
    ASSERT_FALSE(invalid);
    EXPECT_EQ(invalid.error().code, ghidra::core::DiagnosticCode::invalid_argument);
    EXPECT_EQ(session_->resolve_symbol("debugger_test_entry").error().code, ghidra::core::DiagnosticCode::unsupported);
    EXPECT_EQ(session_->stack_trace().error().code, ghidra::core::DiagnosticCode::unsupported);
}

#endif

} // namespace
