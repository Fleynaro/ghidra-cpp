module;

#if defined(NEW_GHIDRA_HAS_TTD_REPLAY)
#include <TTD/ErrorReporting.h>
#include <TTD/IReplayEngineRegisters.h>
#include <TTD/IReplayEngineStl.h>
#endif

export module ghidra.service.debugger.win_ttd;

import std;
import ghidra.core;

// Implementation references:
// TEST/debugger/TTD/ReplayApi/TraceDebugger/TraceDebugger.cpp
// TEST/debugger/TTD/docs/IReplayEngine.h/interface-ICursorView.md
// dependencies/Microsoft.TimeTravelDebugging.Apis.0.9.5/CMake/Microsoft.TimeTravelDebugging.ApisConfig.cmake
// The service intentionally keeps all TTD interfaces behind this module.

export namespace ghidra::services::debugger::win_ttd {

namespace core = ghidra::core;
namespace api = ghidra::core::contracts;
namespace model = ghidra::core::debugger;
namespace replay = ghidra::core::replay;

namespace detail {

/// Accepts Windows' case-insensitive spelling of a finalized `.run` trace.
[[nodiscard]] inline bool is_run_path(const std::filesystem::path& value) {
    auto extension = value.extension().string();
    for (char& character : extension)
        if (character >= 'A' && character <= 'Z')
            character = static_cast<char>(character - 'A' + 'a');
    return extension == ".run";
}

/// Creates a diagnostic for an unavailable or failed native TTD operation.
[[nodiscard]] inline core::Error native_error(std::string_view operation, std::string detail) {
    return core::Error::make(core::DiagnosticCode::resource_unavailable,
                             "Windows TTD " + std::string(operation) + " failed: " + std::move(detail),
                             "Install the Microsoft TTD Replay API/runtime and verify the trace path.");
}

/// Creates the stable diagnostic used for non-Windows and unconfigured builds.
[[nodiscard]] inline core::Error unsupported() {
    return core::Error::make(core::DiagnosticCode::unsupported, "Windows TTD Replay is unavailable in this build",
                             "Build on Windows with Microsoft.TimeTravelDebugging.Apis and TTD runtime DLLs.");
}

/// Creates a lifecycle conflict diagnostic for an unopened or closed trace.
[[nodiscard]] inline core::Error not_open() {
    return core::Error::make(core::DiagnosticCode::conflict, "No TTD trace is open in this replay session",
                             "Call open_trace with a finalized .run file before querying replay state.");
}

/// Converts a native position into the portable replay position.
#if defined(NEW_GHIDRA_HAS_TTD_REPLAY)
/// Captures Replay API diagnostics without writing to the process console.
class ErrorReporting final : public TTD::ErrorReporting {
public:
    /// Formats one native diagnostic for inclusion in a later service error.
    void __fastcall VPrintError(char const* const format, va_list arguments) override {
        std::scoped_lock lock(mutex_);
        char buffer[2048]{};
        std::vsnprintf(buffer, sizeof(buffer), format, arguments);
        last_error = buffer;
    }

    /// Returns the most recent diagnostic without racing a native callback.
    [[nodiscard]] std::string message() const {
        std::scoped_lock lock(mutex_);
        return last_error;
    }

private:
    mutable std::mutex mutex_;
    std::string last_error;
};

[[nodiscard]] inline replay::Position position(const TTD::Replay::Position& value) noexcept {
    return {static_cast<std::uint64_t>(value.Sequence), static_cast<std::uint64_t>(value.Steps)};
}

/// Converts a portable position into the native TTD position.
[[nodiscard]] inline TTD::Replay::Position native_position(replay::Position value) noexcept {
    return TTD::Replay::Position(static_cast<TTD::SequenceId>(value.sequence),
                                 static_cast<TTD::Replay::StepCount>(value.steps));
}
#endif

} // namespace detail

/// Implements read-only inspection and timeline navigation over one TTD trace.
class WinTtdReplaySession final : public api::IReplayDebugSession {
public:
    /// Creates an unopened replay session with a stable identity.
    explicit WinTtdReplaySession(model::SessionId id, model::SessionOptions options)
        : id_(id), options_(std::move(options)) {}

    /// Closes the trace before releasing native replay objects.
    ~WinTtdReplaySession() override {
        static_cast<void>(close_trace());
    }

    /// Prevents copying native cursor ownership.
    WinTtdReplaySession(const WinTtdReplaySession&) = delete;
    /// Prevents copying native cursor ownership.
    WinTtdReplaySession& operator=(const WinTtdReplaySession&) = delete;

    /// Returns the stable session identity.
    [[nodiscard]] model::SessionId identity() const noexcept override {
        return id_;
    }
    /// Returns the cached replay lifecycle state.
    [[nodiscard]] model::SessionState state() const noexcept override {
        std::scoped_lock lock(mutex_);
        return state_;
    }

    /// Opens and initializes a finalized .run trace with Microsoft's Replay API.
    [[nodiscard]] core::Result<void> open_trace(std::filesystem::path trace) override {
        std::scoped_lock lock(mutex_);
#if !defined(NEW_GHIDRA_HAS_TTD_REPLAY)
        static_cast<void>(trace);
        return std::unexpected(detail::unsupported());
#else
        std::error_code filesystem_error;
        if (trace.empty() || !std::filesystem::is_regular_file(trace, filesystem_error))
            return std::unexpected(core::Error::make(core::DiagnosticCode::invalid_argument,
                                                     "TTD trace path must name an existing .run file"));
        if (!detail::is_run_path(trace))
            return std::unexpected(
                core::Error::make(core::DiagnosticCode::invalid_argument, "TTD replay requires a .run trace file"));
        // The Microsoft engine is initialized once; destroy the previous
        // cursor before replacing its owning engine when reopening.
        cursor_.reset();
        engine_.reset();
        selected_thread_.reset();
        selected_thread_native_.reset();
        auto [engine, result] = TTD::Replay::MakeReplayEngine();
        if (result != 0 || engine == nullptr)
            return std::unexpected(detail::native_error("MakeReplayEngine", std::to_string(result)));
        engine->RegisterDebugModeAndLogging(TTD::Replay::DebugModeType::None, &error_reporting_);
        if (!engine->Initialize(trace.wstring().c_str()))
            return std::unexpected(detail::native_error(
                "Initialize", error_reporting_.message().empty() ? trace.string() : error_reporting_.message()));
        auto cursor = engine->NewCursor();
        if (cursor == nullptr)
            return std::unexpected(detail::native_error("NewCursor", "the engine returned a null cursor"));
        engine_ = std::move(engine);
        cursor_.reset(cursor);
        trace_ = std::move(trace);
        cursor_->SetPosition(engine_->GetFirstPosition());
        state_ = model::SessionState::stopped;
        return {};
#endif
    }

    /// Releases the cursor and replay engine and returns to the created state.
    [[nodiscard]] core::Result<void> close_trace() override {
        std::scoped_lock lock(mutex_);
#if defined(NEW_GHIDRA_HAS_TTD_REPLAY)
        cursor_.reset();
        engine_.reset();
#endif
        trace_.clear();
        state_ = model::SessionState::created;
        selected_thread_.reset();
#if defined(NEW_GHIDRA_HAS_TTD_REPLAY)
        selected_thread_native_.reset();
#endif
        return {};
    }

    /// Reports trace lifetime and global metadata.
    [[nodiscard]] core::Result<replay::TraceInfo> trace_info() const override {
        std::scoped_lock lock(mutex_);
#if !defined(NEW_GHIDRA_HAS_TTD_REPLAY)
        return std::unexpected(detail::unsupported());
#else
        if (!engine_)
            return std::unexpected(detail::not_open());
        return replay::TraceInfo{replay::PositionRange{detail::position(engine_->GetFirstPosition()),
                                                       detail::position(engine_->GetLastPosition())},
                                 engine_->GetSystemInfo().ProcessId,
                                 static_cast<std::uint64_t>(engine_->GetPebAddress()),
                                 engine_->GetThreadCount(),
                                 engine_->GetModuleInstanceCount(),
                                 engine_->GetExceptionEventCount(),
                                 engine_->GetKeyframeCount()};
#endif
    }

    /// Returns the cursor's current valid timeline position.
    [[nodiscard]] core::Result<replay::Position> position() const override {
        std::scoped_lock lock(mutex_);
#if !defined(NEW_GHIDRA_HAS_TTD_REPLAY)
        return std::unexpected(detail::unsupported());
#else
        if (!cursor_)
            return std::unexpected(detail::not_open());
        return detail::position(cursor_->GetPosition());
#endif
    }

    /// Moves the cursor to a caller-supplied valid timeline position.
    [[nodiscard]] core::Result<void> seek(replay::Position target) override {
        std::scoped_lock lock(mutex_);
#if !defined(NEW_GHIDRA_HAS_TTD_REPLAY)
        static_cast<void>(target);
        return std::unexpected(detail::unsupported());
#else
        if (!cursor_)
            return std::unexpected(detail::not_open());
        if (target.sequence == std::numeric_limits<std::uint64_t>::max())
            return std::unexpected(
                core::Error::make(core::DiagnosticCode::invalid_argument, "TTD rejected the invalid replay sequence"));
        const replay::Position first = detail::position(engine_->GetFirstPosition());
        const replay::Position last = detail::position(engine_->GetLastPosition());
        if (target < first || target > last)
            return std::unexpected(core::Error::make(core::DiagnosticCode::invalid_argument,
                                                     "TTD replay position is outside the trace lifetime"));
        cursor_->SetPosition(detail::native_position(target));
        if (cursor_->GetPosition().Sequence == TTD::SequenceId::Invalid)
            return std::unexpected(core::Error::make(core::DiagnosticCode::invalid_argument,
                                                     "TTD rejected the requested timeline position"));
        return {};
#endif
    }

    /// Replays forward and translates the native stop result.
    [[nodiscard]] core::Result<replay::StepResult> step_forward(std::uint64_t count) override {
        return step(count, true);
    }
    /// Replays backward and translates the native stop result.
    [[nodiscard]] core::Result<replay::StepResult> step_backward(std::uint64_t count) override {
        return step(count, false);
    }

    /// Replays to the next or previous matching memory access.
    [[nodiscard]] core::Result<replay::StepResult>
    seek_watchpoint(core::Address address, std::size_t size, model::WatchpointAccess access, bool forward) override {
        std::scoped_lock lock(mutex_);
#if !defined(NEW_GHIDRA_HAS_TTD_REPLAY)
        static_cast<void>(address);
        static_cast<void>(size);
        static_cast<void>(access);
        static_cast<void>(forward);
        return std::unexpected(detail::unsupported());
#else
        if (!cursor_)
            return std::unexpected(detail::not_open());
        if (address.space.name() != "ram" || address.offset == 0 || size == 0)
            return std::unexpected(core::Error::make(core::DiagnosticCode::invalid_argument,
                                                     "TTD watchpoints require a non-zero ram address and size"));
        TTD::Replay::DataAccessMask mask = TTD::Replay::DataAccessMask::None;
        switch (access) {
            case model::WatchpointAccess::read:
                mask = TTD::Replay::DataAccessMask::Read;
                break;
            case model::WatchpointAccess::write:
                mask = TTD::Replay::DataAccessMask::Write;
                break;
            case model::WatchpointAccess::read_write:
                mask = TTD::Replay::DataAccessMask::Read | TTD::Replay::DataAccessMask::Write;
                break;
            case model::WatchpointAccess::execute:
                mask = TTD::Replay::DataAccessMask::Execute;
                break;
        }
        auto query_cursor = TTD::Replay::UniqueCursor{cursor_->GetReplayEngine()->NewCursor()};
        if (!query_cursor)
            return std::unexpected(detail::native_error("NewCursor", "the watchpoint query returned a null cursor"));
        query_cursor->SetPosition(cursor_->GetPosition());
        query_cursor->SetEventMask(TTD::Replay::EventMask::MemoryWatchpoint);
        TTD::Replay::MemoryWatchpointData watch{static_cast<TTD::GuestAddress>(address.offset), size, mask};
        if (!query_cursor->AddMemoryWatchpoint(watch))
            return std::unexpected(detail::native_error("AddMemoryWatchpoint", "watchpoint was rejected"));
        const auto result = forward ? query_cursor->ReplayForward(TTD::Replay::Position::Max)
                                    : query_cursor->ReplayBackward(TTD::Replay::Position::Min);
        query_cursor->RemoveMemoryWatchpoint(watch);
        return make_step_result(result, *query_cursor);
#endif
    }

    /// Returns the process represented by the trace.
    [[nodiscard]] core::Result<model::Process> process() const override {
        std::scoped_lock lock(mutex_);
#if !defined(NEW_GHIDRA_HAS_TTD_REPLAY)
        return std::unexpected(detail::unsupported());
#else
        if (!engine_)
            return std::unexpected(detail::not_open());
        const auto& info = engine_->GetSystemInfo();
        return model::Process{model::ProcessId{std::to_string(info.ProcessId)},
                              trace_.stem().string(),
                              {},
                              model::ProcessState::stopped,
                              std::nullopt,
                              options_.target_description};
#endif
    }

    /// Returns a singleton process list for the recorded process.
    [[nodiscard]] core::Result<std::vector<model::Process>> processes() const override {
        auto current = process();
        if (!current)
            return std::unexpected(current.error());
        return std::vector<model::Process>{*current};
    }
    /// Accepts only the process identity encoded in the trace.
    [[nodiscard]] core::Result<void> select_process(model::ProcessId process_id) override {
        std::scoped_lock lock(mutex_);
        auto current = process();
        if (!current)
            return std::unexpected(current.error());
        if (process_id != current->id)
            return std::unexpected(
                core::Error::make(core::DiagnosticCode::invalid_argument, "Process is not in this TTD trace"));
        return {};
    }
    /// Returns the recorded process identity.
    [[nodiscard]] core::Result<model::ProcessId> current_process_id() const override {
        auto current = process();
        if (!current)
            return std::unexpected(current.error());
        return current->id;
    }

    /// Enumerates active threads at the current cursor position.
    [[nodiscard]] core::Result<std::vector<model::Thread>> threads() const override {
        std::scoped_lock lock(mutex_);
#if !defined(NEW_GHIDRA_HAS_TTD_REPLAY)
        return std::unexpected(detail::unsupported());
#else
        if (!cursor_)
            return std::unexpected(detail::not_open());
        std::vector<model::Thread> result;
        for (std::size_t index = 0; index < cursor_->GetThreadCount(); ++index) {
            const auto& active = cursor_->GetThreadList()[index];
            if (active.pThread == nullptr)
                continue;
            const auto id = std::to_string(static_cast<std::uint64_t>(active.pThread->UniqueId));
            result.push_back(model::Thread{
                model::ThreadId{id},
                {},
                model::ThreadState::stopped,
                selected_thread_.value_or(id) == id,
                core::Address{core::AddressSpaceId{"ram"},
                              static_cast<std::uint64_t>(cursor_->GetProgramCounter(active.pThread->Id))}});
        }
        return result;
#endif
    }
    /// Selects a thread by its recorded unique identifier.
    [[nodiscard]] core::Result<void> select_thread(model::ThreadId thread) override {
        std::scoped_lock lock(mutex_);
        auto all = threads();
        if (!all)
            return std::unexpected(all.error());
        if (!std::ranges::any_of(*all, [&](const model::Thread& value) { return value.id == thread; }))
            return std::unexpected(
                core::Error::make(core::DiagnosticCode::invalid_argument, "Thread is not active at this TTD position"));
        selected_thread_ = thread.value;
#if defined(NEW_GHIDRA_HAS_TTD_REPLAY)
        selected_thread_native_.reset();
        for (std::size_t index = 0; index < cursor_->GetThreadCount(); ++index) {
            const auto& active = cursor_->GetThreadList()[index];
            if (active.pThread != nullptr &&
                std::to_string(static_cast<std::uint64_t>(active.pThread->UniqueId)) == thread.value) {
                selected_thread_native_ = active.pThread->Id;
                break;
            }
        }
#endif
        return {};
    }
    /// Returns the selected thread or the cursor's current thread.
    [[nodiscard]] core::Result<model::ThreadId> current_thread() const override {
        std::scoped_lock lock(mutex_);
#if !defined(NEW_GHIDRA_HAS_TTD_REPLAY)
        return std::unexpected(detail::unsupported());
#else
        if (!cursor_)
            return std::unexpected(detail::not_open());
        if (selected_thread_)
            return model::ThreadId{*selected_thread_};
        return model::ThreadId{std::to_string(static_cast<std::uint64_t>(cursor_->GetThreadInfo().UniqueId))};
#endif
    }

    /// Returns the x64 register descriptors exposed by the current API adapter.
    [[nodiscard]] core::Result<std::vector<model::Register>> registers(std::optional<model::ThreadId>) const override {
        static constexpr std::array names{"rax", "rbx", "rcx", "rdx", "rsi", "rdi", "rbp", "rsp", "rip", "rflags"};
        std::vector<model::Register> result;
        for (const auto name : names)
            result.push_back(model::Register{std::string{name}, name == "rflags" ? 32U : 64U, true, name == "rip",
                                             name == "rsp", name == "rbp", name == "rflags", false});
        return result;
    }

    /// Reads one x64 register from the TTD cross-platform context.
    [[nodiscard]] core::Result<model::RegisterValue>
    read_register(std::string_view name, std::optional<model::ThreadId> thread) const override {
        std::scoped_lock lock(mutex_);
#if !defined(NEW_GHIDRA_HAS_TTD_REPLAY)
        static_cast<void>(name);
        return std::unexpected(detail::unsupported());
#else
        if (!cursor_)
            return std::unexpected(detail::not_open());
        const CROSS_PLATFORM_CONTEXT context = cursor_->GetCrossPlatformContext(resolve_thread_id(thread));
        const auto& value = context.Amd64Context;
        std::size_t byte_count{};
        const auto* bytes = register_bytes(value, name, byte_count);
        if (bytes == nullptr)
            return std::unexpected(
                core::Error::make(core::DiagnosticCode::invalid_argument, "Unknown or unsupported TTD register"));
        model::Register descriptor{std::string{name},
                                   static_cast<std::uint32_t>(byte_count * 8U),
                                   true,
                                   name == "rip",
                                   name == "rsp",
                                   name == "rbp",
                                   name == "rflags",
                                   false};
        return model::RegisterValue{descriptor, core::Bytes{std::span<const core::Byte>(bytes, byte_count)},
                                    model::ByteOrder::little};
#endif
    }
    /// Reads all portable register values exposed by this x64 adapter.
    [[nodiscard]] core::Result<std::vector<model::RegisterValue>>
    read_registers(std::optional<model::ThreadId> thread) const override {
        auto descriptors = registers(thread);
        if (!descriptors)
            return std::unexpected(descriptors.error());
        std::vector<model::RegisterValue> result;
        for (const auto& descriptor : *descriptors) {
            auto value = read_register(descriptor.name, thread);
            if (!value)
                return std::unexpected(value.error());
            result.push_back(std::move(*value));
        }
        return result;
    }
    /// Reads the current program counter as a generic ram address.
    [[nodiscard]] core::Result<core::Address>
    instruction_pointer(std::optional<model::ThreadId> thread) const override {
        std::scoped_lock lock(mutex_);
#if !defined(NEW_GHIDRA_HAS_TTD_REPLAY)
        return std::unexpected(detail::unsupported());
#else
        if (!cursor_)
            return std::unexpected(detail::not_open());
        return core::Address{core::AddressSpaceId{"ram"},
                             static_cast<std::uint64_t>(cursor_->GetProgramCounter(resolve_thread_id(thread)))};
#endif
    }

    /// Reads immutable guest memory at the cursor position.
    [[nodiscard]] core::Result<model::MemoryReadResult> read_memory(core::Address address,
                                                                    std::size_t size) const override {
        std::scoped_lock lock(mutex_);
#if !defined(NEW_GHIDRA_HAS_TTD_REPLAY)
        static_cast<void>(address);
        static_cast<void>(size);
        return std::unexpected(detail::unsupported());
#else
        if (!cursor_)
            return std::unexpected(detail::not_open());
        if (address.space.name() != "ram" || size == 0 || size > std::numeric_limits<std::uint32_t>::max())
            return std::unexpected(
                core::Error::make(core::DiagnosticCode::invalid_argument,
                                  "TTD memory reads require a non-empty ram range within the bridge limit"));
        std::vector<core::Byte> bytes(size);
        const auto memory = cursor_->QueryMemoryBuffer(static_cast<TTD::GuestAddress>(address.offset),
                                                       TTD::BufferView{bytes.data(), bytes.size()});
        const auto copied = std::min<std::size_t>(size, memory.Memory.Size);
        bytes.resize(copied);
        return model::MemoryReadResult{address, size, core::Bytes{std::move(bytes)}, copied};
#endif
    }

    /// Reports module-backed memory regions as conservative read/execute mappings.
    [[nodiscard]] core::Result<std::vector<model::MemoryRegion>> memory_regions() const override {
        auto loaded = modules();
        if (!loaded)
            return std::unexpected(loaded.error());
        std::vector<model::MemoryRegion> result;
        for (const auto& module : *loaded)
            result.push_back(model::MemoryRegion{module.base, module.size, true, false, true, module.name});
        return result;
    }
    /// Stack unwinding is not provided by the standalone Replay API contract.
    [[nodiscard]] core::Result<std::vector<model::StackFrame>> stack_trace(std::optional<model::ThreadId>,
                                                                           std::size_t) const override {
        return std::unexpected(core::Error::make(core::DiagnosticCode::unsupported,
                                                 "Standalone TTD Replay does not expose a symbolized stack walker"));
    }
    /// Enumerates module instances currently active at the cursor position.
    [[nodiscard]] core::Result<std::vector<model::Module>> modules() const override {
        std::scoped_lock lock(mutex_);
#if !defined(NEW_GHIDRA_HAS_TTD_REPLAY)
        return std::unexpected(detail::unsupported());
#else
        if (!cursor_)
            return std::unexpected(detail::not_open());
        std::vector<model::Module> result;
        for (std::size_t index = 0; index < cursor_->GetModuleCount(); ++index) {
            const auto& instance = cursor_->GetModuleList()[index];
            if (instance.pModule == nullptr)
                continue;
            const auto* module = instance.pModule;
            const std::wstring name(module->pName, module->NameLength);
            result.push_back(
                model::Module{std::filesystem::path{name}.filename().string(), std::filesystem::path{name}.string(),
                              core::Address{core::AddressSpaceId{"ram"}, static_cast<std::uint64_t>(module->Address)},
                              module->Size, false});
        }
        return result;
#endif
    }
    /// The standalone Replay API does not resolve symbols by name.
    [[nodiscard]] core::Result<core::Address> resolve_symbol(std::string_view) const override {
        return std::unexpected(core::Error::make(core::DiagnosticCode::unsupported,
                                                 "TTD Replay symbol resolution requires an external symbol provider"));
    }
    /// Replay events are returned as step results; no asynchronous event queue exists.
    [[nodiscard]] core::Result<std::vector<model::DebugEvent>> poll_events() override {
        return std::unexpected(core::Error::make(
            core::DiagnosticCode::unsupported, "Standalone TTD Replay event polling is not exposed by this adapter",
            "Use replay step results; callback-backed event delivery requires a dedicated event queue."));
    }
    /// Stores a sink for future translated replay-event delivery.
    [[nodiscard]] core::Result<void> set_event_sink(api::DebugEventSink sink) override {
        static_cast<void>(sink);
        return std::unexpected(core::Error::make(
            core::DiagnosticCode::unsupported, "Standalone TTD Replay event sinks are not available in this adapter",
            "Use replay step results until callback-backed event delivery is enabled."));
    }

private:
    /// Executes one bounded replay operation and maps its result.
    [[nodiscard]] core::Result<replay::StepResult> step(std::uint64_t count, bool forward) {
        std::scoped_lock lock(mutex_);
#if !defined(NEW_GHIDRA_HAS_TTD_REPLAY)
        static_cast<void>(count);
        static_cast<void>(forward);
        return std::unexpected(detail::unsupported());
#else
        if (!cursor_)
            return std::unexpected(detail::not_open());
        if (count == 0 || count > static_cast<std::uint64_t>(TTD::Replay::StepCount::Max))
            return std::unexpected(core::Error::make(core::DiagnosticCode::invalid_argument,
                                                     "Replay step count must be non-zero and fit TTD StepCount"));
        const auto result = forward ? cursor_->ReplayForward(static_cast<TTD::Replay::StepCount>(count))
                                    : cursor_->ReplayBackward(static_cast<TTD::Replay::StepCount>(count));
        return make_step_result(result, *cursor_);
#endif
    }

#if defined(NEW_GHIDRA_HAS_TTD_REPLAY)
    /// Maps a native replay result without leaking its event enumeration.
    template <class NativeResult>
    [[nodiscard]] replay::StepResult make_step_result(const NativeResult& result,
                                                      const TTD::Replay::ICursorView& cursor) const {
        replay::StopReason reason = replay::StopReason::unknown;
        if (result.StopReason == TTD::Replay::EventType::MemoryWatchpoint)
            reason = replay::StopReason::watchpoint;
        else if (result.StopReason == TTD::Replay::EventType::Exception)
            reason = replay::StopReason::exception;
        else if (result.StopReason == TTD::Replay::EventType::Thread)
            reason = replay::StopReason::thread_event;
        else if (result.StopReason == TTD::Replay::EventType::StepCount ||
                 result.StopReason == TTD::Replay::EventType::Position ||
                 result.StopReason == TTD::Replay::EventType::Process)
            reason = replay::StopReason::boundary;
        else if (result.StopReason == TTD::Replay::EventType::Interrupted)
            reason = replay::StopReason::error;
        else if (result.StopReason == TTD::Replay::EventType::Error)
            reason = replay::StopReason::error;
        return {detail::position(cursor.GetPosition()), detail::position(cursor.GetPreviousPosition()), reason,
                static_cast<std::uint64_t>(result.StepsExecuted),
                static_cast<std::uint64_t>(result.InstructionsExecuted)};
    }

    /// Resolves an optional portable unique-thread selection to a native thread id.
    [[nodiscard]] TTD::ThreadId resolve_thread_id(std::optional<model::ThreadId> requested) const noexcept {
        if (requested.has_value()) {
            for (std::size_t index = 0; index < cursor_->GetThreadCount(); ++index) {
                const auto& active = cursor_->GetThreadList()[index];
                if (active.pThread != nullptr &&
                    std::to_string(static_cast<std::uint64_t>(active.pThread->UniqueId)) == requested->value)
                    return active.pThread->Id;
            }
        }
        return selected_thread_native_.value_or(TTD::ThreadId::Invalid);
    }

    /// Returns the address of a supported x64 scalar register in the native context.
    [[nodiscard]] static const std::uint8_t* register_bytes(const AMD64_CONTEXT& context, std::string_view name,
                                                            std::size_t& byte_count) noexcept {
        byte_count = sizeof(std::uint64_t);
        if (name == "rax")
            return reinterpret_cast<const std::uint8_t*>(&context.Rax);
        if (name == "rbx")
            return reinterpret_cast<const std::uint8_t*>(&context.Rbx);
        if (name == "rcx")
            return reinterpret_cast<const std::uint8_t*>(&context.Rcx);
        if (name == "rdx")
            return reinterpret_cast<const std::uint8_t*>(&context.Rdx);
        if (name == "rsi")
            return reinterpret_cast<const std::uint8_t*>(&context.Rsi);
        if (name == "rdi")
            return reinterpret_cast<const std::uint8_t*>(&context.Rdi);
        if (name == "rbp")
            return reinterpret_cast<const std::uint8_t*>(&context.Rbp);
        if (name == "rsp")
            return reinterpret_cast<const std::uint8_t*>(&context.Rsp);
        if (name == "rip")
            return reinterpret_cast<const std::uint8_t*>(&context.Rip);
        if (name == "rflags") {
            byte_count = sizeof(context.EFlags);
            return reinterpret_cast<const std::uint8_t*>(&context.EFlags);
        }
        return nullptr;
    }
#endif

    model::SessionId id_;
    model::SessionOptions options_;
    model::SessionState state_{model::SessionState::created};
    std::filesystem::path trace_;
    std::optional<std::string> selected_thread_;
    mutable std::recursive_mutex mutex_;
#if defined(NEW_GHIDRA_HAS_TTD_REPLAY)
    std::optional<TTD::ThreadId> selected_thread_native_;
    detail::ErrorReporting error_reporting_;
    TTD::Replay::UniqueReplayEngine engine_;
    TTD::Replay::UniqueCursor cursor_;
#endif
};

/// Owns replay-session identity allocation and creates independent sessions.
class WinTtdReplayDebugger final : public api::IReplayDebugger {
public:
    /// Creates an empty replay service.
    WinTtdReplayDebugger() = default;
    /// Creates one isolated replay session.
    [[nodiscard]] core::Result<std::shared_ptr<api::IReplayDebugSession>>
    create_session(model::SessionOptions options = {}) override {
        return std::shared_ptr<api::IReplayDebugSession>{
            std::make_shared<WinTtdReplaySession>(model::SessionId{next_id_++}, std::move(options))};
    }

private:
    inline static std::atomic_uint64_t next_id_{1};
};

/// Creates the Windows TTD replay service without exposing native setup.
[[nodiscard]] inline core::Result<std::shared_ptr<api::IReplayDebugger>> create_win_ttd() {
    return std::shared_ptr<api::IReplayDebugger>{std::make_shared<WinTtdReplayDebugger>()};
}

} // namespace ghidra::services::debugger::win_ttd
