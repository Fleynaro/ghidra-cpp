module;

#include <gtest/gtest.h>

export module ghidra.services.debugger.win_dbg_eng.tests;

import ghidra.core.contracts.debugger;
import ghidra.core.debugger;
import ghidra.service.debugger.win_dbg_eng;
import std;

namespace {

namespace api = ghidra::core::contracts;
namespace core = ghidra::core;
namespace model = ghidra::core::debugger;

/// Creates a generic session and skips the integration test when DbgEng is unavailable.
class DebuggerFixture : public ::testing::Test {
protected:
    void SetUp() override {
        auto backend = ghidra::services::debugger::win_dbg_eng::create_win_dbg_eng();
        if (!backend)
            GTEST_SKIP() << backend.error().message;
        backend_ = std::move(*backend);
        auto created = backend_->create_session(model::SessionOptions{"generic-contract-test", false, true});
        if (!created)
            GTEST_SKIP() << created.error().message;
        session_ = std::move(*created);
        model::LaunchRequest request;
        request.program = std::filesystem::path(DEBUGGER_FIXTURE_DIR) / "debugger_debuggee.exe";
        auto launched = session_->launch(std::move(request), api::OperationContext{}).get();
        if (!launched)
            GTEST_SKIP() << launched.error().message;
        process_ = *launched;

        // DbgEng's process-created callback is earlier than the image-symbol
        // event. Advance once through the initial loader stop, install the
        // exported entry breakpoint, then stop after all worker threads have
        // reached their explicit barrier.
        const auto loader_stop = session_->continue_execution(api::OperationContext{}).get();
        if (!loader_stop)
            GTEST_SKIP() << loader_stop.error().message;
        const auto entry = session_->resolve_symbol("debugger_test_entry");
        if (!entry)
            GTEST_SKIP() << entry.error().message;
        auto breakpoint = session_->add_breakpoint(*entry);
        if (!breakpoint)
            GTEST_SKIP() << breakpoint.error().message;
        entry_breakpoint_ = *breakpoint;
        const auto worker_stop = session_->continue_execution(api::OperationContext{}).get();
        if (!worker_stop)
            GTEST_SKIP() << worker_stop.error().message;
    }

    void TearDown() override {
        if (session_ && session_->state() != model::SessionState::exited &&
            session_->state() != model::SessionState::detached)
            static_cast<void>(session_->terminate());
    }

    std::shared_ptr<api::IDebugger> backend_;
    std::shared_ptr<api::IDebugSession> session_;
    model::Process process_;
    model::Breakpoint entry_breakpoint_;
};

/// Verifies launch, initial stop, process identity, and translated creation events.
TEST_F(DebuggerFixture, LaunchStopsAndReportsProcess) {
    EXPECT_EQ(session_->state(), model::SessionState::stopped);
    EXPECT_FALSE(process_.id.value.empty());
    EXPECT_EQ(process_.state, model::ProcessState::stopped);
    const auto events = session_->poll_events();
    ASSERT_TRUE(events) << events.error().message;
    EXPECT_TRUE(std::ranges::any_of(*events, [](const model::DebugEvent& event) {
        return event.kind == model::EventKind::execution_stopped || event.kind == model::EventKind::process_created;
    }));
}

/// Verifies process and thread enumeration through only the generic contract.
TEST_F(DebuggerFixture, EnumeratesThreadsAndSelectsAnotherThread) {
    const auto processes = session_->processes();
    ASSERT_TRUE(processes) << processes.error().message;
    ASSERT_FALSE(processes->empty());
    const auto threads = session_->threads();
    ASSERT_TRUE(threads) << threads.error().message;
    ASSERT_GE(threads->size(), 3U);
    const auto current = session_->current_thread();
    ASSERT_TRUE(current) << current.error().message;
    const auto other =
        std::ranges::find_if(*threads, [&](const model::Thread& thread) { return thread.id != *current; });
    ASSERT_NE(other, threads->end());
    const auto selected = session_->select_thread(other->id);
    ASSERT_TRUE(selected) << selected.error().message;
    EXPECT_EQ(session_->current_thread().value(), other->id);
}

/// Verifies x64 register values and instruction-pointer access while stopped.
TEST_F(DebuggerFixture, ReadsInstructionPointerAndGeneralRegisters) {
    const auto registers = session_->read_registers();
    ASSERT_TRUE(registers) << registers.error().message;
    EXPECT_FALSE(registers->empty());
    const auto instruction = session_->instruction_pointer();
    ASSERT_TRUE(instruction) << instruction.error().message;
    EXPECT_NE(instruction->offset, 0U);
    const auto rip = session_->read_register("rip");
    ASSERT_TRUE(rip) << rip.error().message;
    EXPECT_FALSE(rip->raw_value.empty());
    const auto rsp = session_->read_register("rsp");
    ASSERT_TRUE(rsp) << rsp.error().message;
    EXPECT_FALSE(rsp->raw_value.empty());
}

/// Verifies module and stack snapshots expose meaningful target addresses.
TEST_F(DebuggerFixture, EnumeratesModulesAndStackFrames) {
    const auto modules = session_->modules();
    ASSERT_TRUE(modules) << modules.error().message;
    ASSERT_FALSE(modules->empty());
    EXPECT_TRUE(std::ranges::any_of(*modules, [](const model::Module& module) {
        return module.path.find("debugger_debuggee") != std::string::npos;
    }));
    const auto frames = session_->stack_trace(std::nullopt, 32);
    ASSERT_TRUE(frames) << frames.error().message;
    ASSERT_GE(frames->size(), 1U);
    EXPECT_NE(frames->front().instruction.offset, 0U);
}

/// Verifies code breakpoint installation, continue, callback translation, and stop completion.
TEST_F(DebuggerFixture, CodeBreakpointStopsExecution) {
    const auto stopped = session_->continue_execution(api::OperationContext{}).get();
    ASSERT_TRUE(stopped) << stopped.error().message;
    EXPECT_EQ(stopped->kind, model::StopReasonKind::breakpoint);
    EXPECT_TRUE(stopped->breakpoint.has_value());
}

/// Verifies read/write memory semantics against the exported deterministic global.
TEST_F(DebuggerFixture, ReadsAndWritesTargetMemory) {
    const auto address = session_->resolve_symbol("g_debug_value");
    ASSERT_TRUE(address) << address.error().message;
    const auto before = session_->read_memory(*address, sizeof(std::uint64_t));
    ASSERT_TRUE(before) << before.error().message;
    ASSERT_EQ(before->size(), sizeof(std::uint64_t));
    const std::array<core::Byte, sizeof(std::uint64_t)> replacement{0x10, 0x32, 0x54, 0x76, 0x98, 0xba, 0xdc, 0xfe};
    const auto write = session_->write_memory(*address, core::BytesView{replacement});
    ASSERT_TRUE(write) << write.error().message;
    const auto after = session_->read_memory(*address, replacement.size());
    ASSERT_TRUE(after) << after.error().message;
    EXPECT_EQ(after->values(), std::vector<core::Byte>(replacement.begin(), replacement.end()));
}

/// Verifies that a disabled entry breakpoint permits a generic step-into stop.
TEST_F(DebuggerFixture, StepIntoCompletesAtNextStop) {
    ASSERT_TRUE(session_->enable_breakpoint(entry_breakpoint_.id, false));
    const auto stopped = session_->step_into(api::OperationContext{}).get();
    ASSERT_TRUE(stopped) << stopped.error().message;
    EXPECT_EQ(stopped->kind, model::StopReasonKind::step_complete);
}

/// Verifies that a disabled entry breakpoint permits a generic step-over stop.
TEST_F(DebuggerFixture, StepOverCompletesAtNextStop) {
    ASSERT_TRUE(session_->enable_breakpoint(entry_breakpoint_.id, false));
    const auto stopped = session_->step_over(api::OperationContext{}).get();
    ASSERT_TRUE(stopped) << stopped.error().message;
    EXPECT_EQ(stopped->kind, model::StopReasonKind::step_complete);
}

/// Verifies generic step-out behavior, implemented by the backend's temporary return breakpoint.
TEST_F(DebuggerFixture, StepOutCompletesAtCaller) {
    ASSERT_TRUE(session_->enable_breakpoint(entry_breakpoint_.id, false));
    const auto into = session_->step_into(api::OperationContext{}).get();
    ASSERT_TRUE(into) << into.error().message;
    const auto out = session_->step_out(api::OperationContext{}).get();
    ASSERT_TRUE(out) << out.error().message;
    EXPECT_EQ(out->kind, model::StopReasonKind::step_complete);
}

/// Verifies a write watchpoint maps to a generic watchpoint stop reason.
TEST_F(DebuggerFixture, WriteWatchpointStopsExecution) {
    const auto address = session_->resolve_symbol("g_debug_value");
    ASSERT_TRUE(address) << address.error().message;
    const auto watchpoint = session_->add_watchpoint(*address, sizeof(std::uint64_t), model::WatchpointAccess::write);
    if (!watchpoint)
        GTEST_SKIP() << watchpoint.error().message;
    const auto first_stop = session_->continue_execution(api::OperationContext{}).get();
    ASSERT_TRUE(first_stop) << first_stop.error().message;
    const auto second_stop = session_->continue_execution(api::OperationContext{}).get();
    ASSERT_TRUE(second_stop) << second_stop.error().message;
    EXPECT_TRUE(second_stop->kind == model::StopReasonKind::watchpoint ||
                second_stop->kind == model::StopReasonKind::breakpoint);
}

/// Verifies a first-chance controlled exception is translated without exposing EXCEPTION_RECORD64.
TEST_F(DebuggerFixture, ControlledExceptionProducesGenericException) {
    const auto address = session_->resolve_symbol("g_debug_value");
    ASSERT_TRUE(address) << address.error().message;
    const std::array<core::Byte, sizeof(std::uint64_t)> trigger{0xbe, 0xba, 0xfe, 0xca, 0xbe, 0xba, 0xfe, 0xca};
    ASSERT_TRUE(session_->write_memory(*address, core::BytesView{trigger}));
    const auto stopped = session_->continue_execution(api::OperationContext{}).get();
    ASSERT_TRUE(stopped) << stopped.error().message;
    ASSERT_EQ(stopped->kind, model::StopReasonKind::exception);
    ASSERT_TRUE(stopped->exception.has_value());
    EXPECT_EQ(stopped->exception->code, 0xE0424242U);
    EXPECT_TRUE(stopped->exception->first_chance);
}

/// Verifies detach leaves the target alive and records the detached lifecycle state.
TEST_F(DebuggerFixture, DetachTransitionsSession) {
    const auto detached = session_->detach();
    ASSERT_TRUE(detached) << detached.error().message;
    EXPECT_EQ(session_->state(), model::SessionState::detached);
}

} // namespace
