module;

#include <gtest/gtest.h>
#ifdef _WIN32
#include <windows.h>
#endif

export module recode.services.debugger.win_dbg_eng.tests;

import recode.core.contracts.debugger;
import recode.core.debugger;
import recode.service.debugger.win_dbg_eng;
import std;

namespace {

namespace api = recode::core::contracts;
namespace core = recode::core;
namespace model = recode::core::debugger;

/// Waits for the detached fixture process so CTest does not retain its captured stdout pipe.
[[nodiscard]] bool wait_for_detached_debuggee(const model::ProcessId& process) {
#ifdef _WIN32
    ULONG native_id{};
    const auto [end, error] =
        std::from_chars(process.value.data(), process.value.data() + process.value.size(), native_id);
    if (error != std::errc{} || end != process.value.data() + process.value.size())
        return false;
    HANDLE handle = OpenProcess(SYNCHRONIZE, FALSE, native_id);
    if (handle == nullptr)
        return true;
    const bool exited = WaitForSingleObject(handle, 5000) == WAIT_OBJECT_0;
    CloseHandle(handle);
    return exited;
#else
    static_cast<void>(process);
    return true;
#endif
}

#if defined(_WIN32)
#define DEBUGGER_SETUP_ERROR(message)                                                                                  \
    do {                                                                                                               \
        FAIL() << message;                                                                                             \
    } while (false)
#else
#define DEBUGGER_SETUP_ERROR(message)                                                                                  \
    do {                                                                                                               \
        GTEST_SKIP() << message;                                                                                       \
    } while (false)
#endif

/// Creates a generic session and fails Windows integration setup instead of silently skipping it.
class DebuggerFixture : public ::testing::Test {
protected:
    void SetUp() override {
        auto backend = recode::services::debugger::win_dbg_eng::create_win_dbg_eng();
        if (!backend)
            DEBUGGER_SETUP_ERROR(backend.error().message);
        backend_ = std::move(*backend);
        auto created = backend_->create_session(
            model::SessionOptions{"generic-contract-test", false, true, false, std::chrono::seconds(3)});
        if (!created)
            DEBUGGER_SETUP_ERROR(created.error().message);
        session_ = std::move(*created);
        model::LaunchRequest request;
        request.program = std::filesystem::path(DEBUGGER_FIXTURE_DIR) / "debugger_debuggee.exe";
        auto launched = session_->launch(std::move(request), api::OperationContext{}).get();
        if (!launched)
            DEBUGGER_SETUP_ERROR(launched.error().message);
        process_ = *launched;

        // DbgEng's process-created callback is earlier than the image-symbol
        // event. Advance through explicit thread-event stops until the image
        // and PDB are loaded, then install the exported entry breakpoint.
        std::optional<core::Address> entry;
        for (unsigned attempt = 0; attempt != 8U && !entry; ++attempt) {
            const auto stop = session_->continue_execution(api::OperationContext{}).get();
            if (!stop)
                DEBUGGER_SETUP_ERROR(stop.error().message);
            static_cast<void>(session_->modules());
            auto candidate = session_->resolve_symbol("debugger_test_entry");
            if (candidate)
                entry = *candidate;
            else if (session_->state() == model::SessionState::exited)
                DEBUGGER_SETUP_ERROR("Debuggee exited before debugger_test_entry symbols became available");
            else if (session_->state() != model::SessionState::exited) {
                const auto retry_stop = session_->pause(api::OperationContext{}).get();
                if (!retry_stop)
                    DEBUGGER_SETUP_ERROR(retry_stop.error().message);
            }
        }
        if (!entry)
            DEBUGGER_SETUP_ERROR("DbgEng did not resolve debugger_test_entry after deterministic warm-up");
        auto breakpoint = session_->add_breakpoint(*entry);
        if (!breakpoint)
            DEBUGGER_SETUP_ERROR(breakpoint.error().message);
        entry_breakpoint_ = *breakpoint;
        const auto worker_stop = session_->continue_execution(api::OperationContext{}).get();
        if (!worker_stop)
            DEBUGGER_SETUP_ERROR(worker_stop.error().message);
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
    const auto selected_process = session_->current_process_id();
    ASSERT_TRUE(selected_process) << selected_process.error().message;
    EXPECT_EQ(*selected_process, process_.id);
    ASSERT_TRUE(session_->select_process(process_.id));
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
    for (const auto name : {"rax", "rbx", "rcx", "rdx", "rsi", "rdi", "rbp", "rflags"}) {
        const auto value = session_->read_register(name);
        ASSERT_TRUE(value) << name << ": " << value.error().message;
        EXPECT_FALSE(value->raw_value.empty()) << name;
    }
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
    ASSERT_GE(frames->size(), 2U);
    EXPECT_NE(frames->front().instruction.offset, 0U);
    EXPECT_TRUE(std::ranges::any_of(
        *modules, [](const model::Module& module) { return module.base.offset != 0U && module.size != 0U; }));
}

/// Verifies code breakpoint installation, continue, callback translation, and stop completion.
TEST_F(DebuggerFixture, CodeBreakpointStopsExecution) {
    const auto address = session_->resolve_symbol("compute_value");
    ASSERT_TRUE(address) << address.error().message;
    const auto breakpoint = session_->add_breakpoint(*address);
    ASSERT_TRUE(breakpoint) << breakpoint.error().message;
    ASSERT_TRUE(session_->enable_breakpoint(entry_breakpoint_.id, false));
    const auto stopped = session_->continue_execution(api::OperationContext{}).get();
    ASSERT_TRUE(stopped) << stopped.error().message;
    EXPECT_EQ(stopped->kind, model::StopReasonKind::breakpoint);
    ASSERT_TRUE(stopped->breakpoint.has_value());
    EXPECT_EQ(*stopped->breakpoint, breakpoint->id);
    const auto instruction = session_->instruction_pointer();
    ASSERT_TRUE(instruction) << instruction.error().message;
    EXPECT_EQ(instruction->offset, address->offset);
    ASSERT_TRUE(session_->enable_breakpoint(breakpoint->id, false));
    ASSERT_TRUE(session_->remove_breakpoint(breakpoint->id));
}

/// Verifies read/write memory semantics against the exported deterministic global.
TEST_F(DebuggerFixture, ReadsAndWritesTargetMemory) {
    const auto address = session_->resolve_symbol("g_debug_value");
    ASSERT_TRUE(address) << address.error().message;
    const auto before = session_->read_memory(*address, sizeof(std::uint64_t));
    ASSERT_TRUE(before) << before.error().message;
    ASSERT_TRUE(before->complete());
    ASSERT_EQ(before->bytes.size(), sizeof(std::uint64_t));
    const std::array<core::Byte, sizeof(std::uint64_t)> replacement{0x10, 0x32, 0x54, 0x76, 0x98, 0xba, 0xdc, 0xfe};
    const auto write = session_->write_memory(*address, core::BytesView{replacement});
    ASSERT_TRUE(write) << write.error().message;
    const auto after = session_->read_memory(*address, replacement.size());
    ASSERT_TRUE(after) << after.error().message;
    ASSERT_TRUE(after->complete());
    EXPECT_EQ(after->bytes.values(), std::vector<core::Byte>(replacement.begin(), replacement.end()));
}

/// Verifies native transfer bounds produce a structured invalid-argument error instead of truncation.
TEST_F(DebuggerFixture, RejectsMemoryReadsBeyondNativeTransferLimit) {
    const auto address = session_->resolve_symbol("g_debug_value");
    ASSERT_TRUE(address) << address.error().message;
    const auto result =
        session_->read_memory(*address, static_cast<std::size_t>((std::numeric_limits<unsigned long>::max)()) + 1U);
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code, core::DiagnosticCode::invalid_argument);
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
    ASSERT_TRUE(session_->enable_breakpoint(entry_breakpoint_.id, false));
    const auto watchpoint = session_->add_watchpoint(*address, sizeof(std::uint64_t), model::WatchpointAccess::write);
    if (!watchpoint)
        GTEST_SKIP() << watchpoint.error().message;
    const auto stopped = session_->continue_execution(api::OperationContext{}).get();
    ASSERT_TRUE(stopped) << stopped.error().message;
    ASSERT_EQ(stopped->kind, model::StopReasonKind::watchpoint);
    ASSERT_TRUE(stopped->watchpoint.has_value());
    EXPECT_EQ(*stopped->watchpoint, watchpoint->id);
    const auto events = session_->poll_events();
    ASSERT_TRUE(events) << events.error().message;
    EXPECT_TRUE(std::ranges::any_of(*events, [&](const model::DebugEvent& event) {
        if (event.kind != model::EventKind::watchpoint_hit)
            return false;
        const auto* hit = std::get_if<model::WatchpointHitEvent>(&event.payload);
        return hit != nullptr && hit->watchpoint.id == watchpoint->id && hit->watchpoint.location == *address;
    }));
}

/// Verifies a first-chance controlled exception is translated without exposing EXCEPTION_RECORD64.
TEST_F(DebuggerFixture, ControlledExceptionProducesGenericException) {
    const auto address = session_->resolve_symbol("g_debugger_trigger_exception");
    ASSERT_TRUE(address) << address.error().message;
    ASSERT_TRUE(session_->enable_breakpoint(entry_breakpoint_.id, false));
    const std::array<core::Byte, sizeof(std::uint32_t)> trigger{1, 0, 0, 0};
    ASSERT_TRUE(session_->write_memory(*address, core::BytesView{trigger}));
    const auto stopped = session_->continue_execution(api::OperationContext{}).get();
    ASSERT_TRUE(stopped) << stopped.error().message;
    ASSERT_EQ(stopped->kind, model::StopReasonKind::exception);
    ASSERT_TRUE(stopped->exception.has_value());
    EXPECT_EQ(stopped->exception->code, 0xE0424242U);
    EXPECT_TRUE(stopped->exception->first_chance);
}

/// Verifies pause interrupts a live continue task and completes both waiters at one stop.
TEST_F(DebuggerFixture, PauseInterruptsRunningExecution) {
    ASSERT_TRUE(session_->enable_breakpoint(entry_breakpoint_.id, false));
    auto running = session_->continue_execution(api::OperationContext{});
    auto paused = session_->pause(api::OperationContext{});
    const auto pause_stop = paused.get();
    ASSERT_TRUE(pause_stop) << pause_stop.error().message;
    EXPECT_EQ(pause_stop->kind, model::StopReasonKind::pause);
    const auto running_stop = running.get();
    ASSERT_TRUE(running_stop) << running_stop.error().message;
    EXPECT_EQ(running_stop->kind, model::StopReasonKind::pause);
}

/// Verifies a second execution waiter is rejected instead of orphaning the first task.
TEST_F(DebuggerFixture, DuplicateExecutionOperationIsRejected) {
    ASSERT_TRUE(session_->enable_breakpoint(entry_breakpoint_.id, false));
    auto first = session_->continue_execution(api::OperationContext{});
    auto second = session_->continue_execution(api::OperationContext{});
    const auto second_result = second.get();
    ASSERT_FALSE(second_result);
    EXPECT_EQ(second_result.error().code, core::DiagnosticCode::conflict);
    const auto pause = session_->pause(api::OperationContext{}).get();
    ASSERT_TRUE(pause) << pause.error().message;
    const auto first_result = first.get();
    ASSERT_TRUE(first_result) << first_result.error().message;
    EXPECT_EQ(first_result->kind, model::StopReasonKind::pause);
}

/// Verifies a live target cannot leave a continue task blocked beyond the configured watchdog.
TEST_F(DebuggerFixture, RunningOperationTimesOutDeterministically) {
    ASSERT_TRUE(session_->remove_breakpoint(entry_breakpoint_.id));
    const auto result = session_->continue_execution(api::OperationContext{}).get();
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code, core::DiagnosticCode::timeout);
}

/// Verifies destroying a session resolves a pending task and terminates the held debuggee.
TEST_F(DebuggerFixture, SessionShutdownResolvesPendingTask) {
    ASSERT_TRUE(session_->enable_breakpoint(entry_breakpoint_.id, false));
    auto running = session_->continue_execution(api::OperationContext{});
    auto owned_session = std::move(session_);
    owned_session.reset();
    const auto result = running.get();
    EXPECT_TRUE(result || result.error().code == core::DiagnosticCode::cancelled);
}

/// Verifies detach leaves the target alive and records the detached lifecycle state.
TEST_F(DebuggerFixture, DetachTransitionsSession) {
    const auto hold = session_->resolve_symbol("g_debugger_hold");
    ASSERT_TRUE(hold) << hold.error().message;
    const std::array<core::Byte, sizeof(std::uint32_t)> release{0, 0, 0, 0};
    ASSERT_TRUE(session_->write_memory(*hold, core::BytesView{release}));
    const auto detached = session_->detach();
    ASSERT_TRUE(detached) << detached.error().message;
    EXPECT_EQ(session_->state(), model::SessionState::detached);
    ASSERT_TRUE(wait_for_detached_debuggee(process_.id));
}

#undef DEBUGGER_SETUP_ERROR

} // namespace
