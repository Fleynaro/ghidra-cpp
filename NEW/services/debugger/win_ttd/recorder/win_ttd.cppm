module;

#ifdef _WIN32
#include <windows.h>
#endif

export module recode.service.debugger.win_ttd.recorder;

import std;
import recode.core;

// Porting references:
// * Microsoft Time Travel Debugging documentation for `ttd.exe -launch`.
// * Ghidra/Debug/Debugger-api/src/main/java/ghidra/app/services/DebuggerTraceManagerService.java
//   (trace acquisition is represented as a replayable artifact, not a debugger session).

export namespace recode::services::debugger::win_ttd::recorder {

namespace core = recode::core;
namespace api = recode::core::contracts;

/// Builds a quoted Windows command-line argument according to CommandLineToArgvW rules.
[[nodiscard]] inline std::wstring quote_argument(std::wstring_view value) {
    std::wstring result = L"\"";
    std::size_t slashes = 0;
    for (const wchar_t character : value) {
        if (character == L'\\') {
            ++slashes;
        } else if (character == L'\"') {
            result.append(slashes * 2 + 1, L'\\');
            result.push_back(character);
            slashes = 0;
        } else {
            result.append(slashes, L'\\');
            result.push_back(character);
            slashes = 0;
        }
    }
    result.append(slashes * 2, L'\\');
    result.push_back(L'\"');
    return result;
}

/// Converts a UTF-8 string to a Windows wide string without changing command data.
[[nodiscard]] inline std::wstring wide(std::string_view value) {
    return std::filesystem::path(std::string(value)).wstring();
}

/// Accepts Windows' case-insensitive spelling of the finalized `.run` artifact.
[[nodiscard]] inline bool is_run_path(const std::filesystem::path& value) {
    auto extension = value.extension().string();
    for (char& character : extension)
        if (character >= 'A' && character <= 'Z')
            character = static_cast<char>(character - 'A' + 'a');
    return extension == ".run";
}

/// Implements trace recording through the Windows TTD command-line recorder.
class WinTtdRecorder final : public api::ITraceRecorder {
public:
    /// Creates an idle recorder; TTD is resolved when a recording starts.
    WinTtdRecorder() = default;

    /// Terminates an active TTD process and releases recorder state.
    ~WinTtdRecorder() override {
        (void)cancel();
    }

    /// Prevents copying a recorder that owns a process handle.
    WinTtdRecorder(const WinTtdRecorder&) = delete;

    /// Prevents copying a recorder that owns a process handle.
    WinTtdRecorder& operator=(const WinTtdRecorder&) = delete;

    /// Builds the exact TTD command line, exposed for contract-level tests.
    [[nodiscard]] static std::wstring build_command_line(const core::trace_recording::RecordingRequest& request,
                                                         std::wstring_view ttd_executable) {
        std::wstring command = quote_argument(ttd_executable) + L" -noUI -out " +
                               quote_argument(request.output_trace.wstring()) + L" -accepteula -launch ";
        if (request.record_children)
            command += L"-children ";
        command += quote_argument(request.program.wstring());
        for (const auto& argument : request.arguments)
            command += L" " + quote_argument(wide(argument));
        return command;
    }

    /// Starts TTD, waits for completion, and reports the produced trace.
    [[nodiscard]] api::Task<core::Result<core::trace_recording::RecordingResult>>
    record(core::trace_recording::RecordingRequest request, api::OperationContext context) override {
        if (request.program.empty() || request.output_trace.empty()) {
            auto promise = std::make_shared<std::promise<core::Result<core::trace_recording::RecordingResult>>>();
            promise->set_value(std::unexpected(core::Error::make(
                core::DiagnosticCode::invalid_argument, "TTD recording requires a program and output trace path.")));
            return api::Task<core::Result<core::trace_recording::RecordingResult>>{
                promise->get_future().share(), std::make_shared<api::OperationControl>()};
        }
        if (!is_run_path(request.output_trace)) {
            auto promise = std::make_shared<std::promise<core::Result<core::trace_recording::RecordingResult>>>();
            promise->set_value(std::unexpected(core::Error::make(
                core::DiagnosticCode::invalid_argument, "Windows TTD recording requires a .run output path.")));
            return api::Task<core::Result<core::trace_recording::RecordingResult>>{
                promise->get_future().share(), std::make_shared<api::OperationControl>()};
        }
        std::error_code path_error;
        if (std::filesystem::exists(request.output_trace, path_error)) {
            auto promise = std::make_shared<std::promise<core::Result<core::trace_recording::RecordingResult>>>();
            promise->set_value(std::unexpected(
                core::Error::make(core::DiagnosticCode::conflict,
                                  "TTD recording output already exists: " + request.output_trace.string())));
            return api::Task<core::Result<core::trace_recording::RecordingResult>>{
                promise->get_future().share(), std::make_shared<api::OperationControl>()};
        }
        const auto output_parent = request.output_trace.parent_path();
        if (!output_parent.empty() && !std::filesystem::is_directory(output_parent, path_error)) {
            auto promise = std::make_shared<std::promise<core::Result<core::trace_recording::RecordingResult>>>();
            promise->set_value(std::unexpected(
                core::Error::make(core::DiagnosticCode::invalid_argument,
                                  "TTD recording output directory does not exist: " + output_parent.string())));
            return api::Task<core::Result<core::trace_recording::RecordingResult>>{
                promise->get_future().share(), std::make_shared<api::OperationControl>()};
        }
        if (!request.options.empty()) {
            auto promise = std::make_shared<std::promise<core::Result<core::trace_recording::RecordingResult>>>();
            promise->set_value(std::unexpected(core::Error::make(
                core::DiagnosticCode::unsupported,
                "This Windows TTD recorder does not expose child-recording or custom recorder options.")));
            return api::Task<core::Result<core::trace_recording::RecordingResult>>{
                promise->get_future().share(), std::make_shared<api::OperationControl>()};
        }
        auto control = std::make_shared<api::OperationControl>();
        const auto task_cancellation = control->cancellation();
        auto promise = std::make_shared<std::promise<core::Result<core::trace_recording::RecordingResult>>>();
        auto future = promise->get_future().share();
        bool accepted = false;
        {
            std::scoped_lock lock(mutex_);
            if (active_)
                promise->set_value(std::unexpected(core::Error::make(
                    core::DiagnosticCode::conflict, "A Windows TTD recording is already active.",
                    "Wait for the current recording to finish or cancel it before starting another.")));
            else {
                active_ = true;
                accepted = true;
            }
        }
        if (!accepted)
            return api::Task<core::Result<core::trace_recording::RecordingResult>>{std::move(future), control};
        control->set_status(api::OperationStatus::running);
        worker_ = std::jthread{[this, request = std::move(request), context = std::move(context), task_cancellation,
                                control, promise](std::stop_token) mutable -> void {
            core::Result<core::trace_recording::RecordingResult> result;
            try {
                result = run(std::move(request), std::move(context), task_cancellation);
            } catch (const std::exception& exception) {
                result = std::unexpected(
                    core::Error::make(core::DiagnosticCode::io_failure,
                                      "Windows TTD recorder worker failed: " + std::string(exception.what())));
            } catch (...) {
                result = std::unexpected(core::Error::make(
                    core::DiagnosticCode::io_failure, "Windows TTD recorder worker failed with an unknown exception."));
            }
            control->set_status(
                !result ? (task_cancellation.stop_requested() ? api::OperationStatus::cancelled
                                                              : api::OperationStatus::failed)
                        : (result->cancelled ? api::OperationStatus::cancelled : api::OperationStatus::completed));
            promise->set_value(std::move(result));
            std::scoped_lock lock(mutex_);
            active_ = false;
#ifdef _WIN32
            process_ = nullptr;
#endif
            cancellation_requested_ = false;
        }};
        return api::Task<core::Result<core::trace_recording::RecordingResult>>{std::move(future), control};
    }

    /// Requests termination of the active TTD process, or reports that none is running.
    [[nodiscard]] core::Result<void> cancel() override {
#ifdef _WIN32
        std::scoped_lock lock(mutex_);
        if (!active_ || process_ == nullptr)
            return std::unexpected(
                core::Error::make(core::DiagnosticCode::conflict, "No Windows TTD recording is active."));
        cancellation_requested_ = true;
        if (!TerminateProcess(process_, 1))
            return std::unexpected(native_error("TerminateProcess"));
        return {};
#else
        return std::unexpected(core::Error::make(core::DiagnosticCode::unsupported,
                                                 "Windows TTD recording is unsupported on this platform.",
                                                 "Run the recorder on Windows with ttd.exe installed."));
#endif
    }

private:
    /// Executes the platform-specific recording and translates failures to core diagnostics.
    [[nodiscard]] core::Result<core::trace_recording::RecordingResult>
    run(core::trace_recording::RecordingRequest request, const api::OperationContext& context,
        const api::CancellationToken& task_cancellation) {
#ifndef _WIN32
        (void)request;
        (void)context;
        (void)task_cancellation;
        return std::unexpected(core::Error::make(core::DiagnosticCode::unsupported,
                                                 "Windows TTD recording is unsupported on this platform.",
                                                 "Run the recorder on Windows with ttd.exe installed."));
#else
        if (context.cancellation.stop_requested() || task_cancellation.stop_requested())
            return core::trace_recording::RecordingResult{request.output_trace, -1, false, true,
                                                          "Windows TTD recording was cancelled before launch."};
        const char* configured = std::getenv("TTD_EXE");
        const std::wstring executable = configured != nullptr && *configured != '\0' ? wide(configured) : L"ttd.exe";
        std::wstring command = build_command_line(request, executable);
        std::vector<wchar_t> mutable_command(command.begin(), command.end());
        mutable_command.push_back(L'\0');
        STARTUPINFOW startup{.cb = sizeof(startup)};
        PROCESS_INFORMATION process_info{};
        const std::wstring directory = request.working_directory.empty() ? L"" : request.working_directory.wstring();
        std::error_code filesystem_error;
        const auto output_parent = request.output_trace.parent_path();
        if (!output_parent.empty() && !std::filesystem::is_directory(output_parent, filesystem_error))
            return std::unexpected(
                core::Error::make(core::DiagnosticCode::invalid_argument,
                                  "TTD recording output directory does not exist: " + output_parent.string()));
        const auto environment = build_environment_block(request);
        if (!CreateProcessW(nullptr, mutable_command.data(), nullptr, nullptr, FALSE, CREATE_UNICODE_ENVIRONMENT,
                            environment.empty() ? nullptr : const_cast<wchar_t*>(environment.data()),
                            directory.empty() ? nullptr : directory.c_str(), &startup, &process_info))
            return std::unexpected(native_error("CreateProcessW"));
        CloseHandle(process_info.hThread);
        {
            std::scoped_lock lock(mutex_);
            process_ = process_info.hProcess;
        }
        bool cancelled = false;
        for (;;) {
            if (context.cancellation.stop_requested() || task_cancellation.stop_requested()) {
                (void)cancel();
                cancelled = true;
            }
            const DWORD status = WaitForSingleObject(process_info.hProcess, 100);
            if (status == WAIT_OBJECT_0)
                break;
            if (status == WAIT_FAILED) {
                {
                    std::scoped_lock lock(mutex_);
                    process_ = nullptr;
                }
                CloseHandle(process_info.hProcess);
                return std::unexpected(native_error("WaitForSingleObject"));
            }
        }
        DWORD exit_code = 1;
        (void)GetExitCodeProcess(process_info.hProcess, &exit_code);
        {
            std::scoped_lock lock(mutex_);
            process_ = nullptr;
        }
        CloseHandle(process_info.hProcess);
        {
            std::scoped_lock lock(mutex_);
            cancelled = cancelled || cancellation_requested_;
        }
        core::trace_recording::RecordingResult result{
            request.output_trace, static_cast<std::int64_t>(exit_code), !cancelled && exit_code == 0, cancelled, {}};
        if (cancelled)
            result.diagnostic = "Windows TTD recording was cancelled.";
        else if (exit_code != 0)
            result.diagnostic = "TTD exited with code " + std::to_string(exit_code) + ".";
        else if (!std::filesystem::is_regular_file(request.output_trace)) {
            result.completed = false;
            result.diagnostic = "TTD exited successfully but did not produce the requested trace file.";
        }
        return result;
#endif
    }

#ifdef _WIN32
public:
    /// Builds a sorted UTF-16 environment block for CreateProcessW.
    [[nodiscard]] static std::vector<wchar_t>
    build_environment_block(const core::trace_recording::RecordingRequest& request) {
        if (request.inherit_environment && request.environment.empty())
            return {};
        std::map<std::wstring, std::wstring> values;
        if (request.inherit_environment) {
            if (LPWCH inherited = GetEnvironmentStringsW(); inherited != nullptr) {
                for (const wchar_t* entry = inherited; *entry != L'\0';) {
                    const std::wstring value(entry);
                    const auto separator = value.find(L'=', 1);
                    if (separator != std::wstring::npos)
                        values[value.substr(0, separator)] = value.substr(separator + 1);
                    entry += value.size() + 1;
                }
                FreeEnvironmentStringsW(inherited);
            }
        }
        for (const auto& [key, value] : request.environment)
            values[wide(key)] = wide(value);
        std::vector<wchar_t> block;
        for (const auto& [key, value] : values) {
            const auto entry = key + L"=" + value;
            block.insert(block.end(), entry.begin(), entry.end());
            block.push_back(L'\0');
        }
        block.push_back(L'\0');
        return block;
    }

private:
#endif

#ifdef _WIN32
    /// Converts the last Win32 error into a clear recorder diagnostic.
    [[nodiscard]] static core::Error native_error(std::string_view operation) {
        const auto code = GetLastError();
        if (code == ERROR_ELEVATION_REQUIRED)
            return core::Error::make(core::DiagnosticCode::resource_unavailable,
                                     "Windows TTD recording requires an elevated process: " +
                                         std::system_category().message(static_cast<int>(code)),
                                     "Run the recording command from an elevated Windows terminal.");
        return core::Error::make(core::DiagnosticCode::resource_unavailable,
                                 "Windows TTD " + std::string(operation) +
                                     " failed: " + std::system_category().message(static_cast<int>(code)),
                                 "Verify TTD_EXE or ttd.exe on PATH and that the output directory is writable.");
    }
#endif

    std::mutex mutex_;
    std::jthread worker_;
    bool active_{};
    bool cancellation_requested_{};
#ifdef _WIN32
    HANDLE process_{};
#endif
};

/// Creates the Windows recorder behind the platform-independent recorder contract.
[[nodiscard]] inline core::Result<std::shared_ptr<api::ITraceRecorder>> create_win_ttd_recorder() {
    return std::shared_ptr<api::ITraceRecorder>{std::make_shared<WinTtdRecorder>()};
}

} // namespace recode::services::debugger::win_ttd::recorder
