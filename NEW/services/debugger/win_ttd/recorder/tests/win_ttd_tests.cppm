module;

#include <gtest/gtest.h>

export module ghidra.service.debugger.win_ttd.recorder.tests;

import std;
import ghidra.core;
import ghidra.service.debugger.win_ttd.recorder;

namespace recorder = ghidra::services::debugger::win_ttd::recorder;

/// Verifies that TTD receives the required switches and preserves target arguments.
TEST(WinTtdRecorder, BuildsTtdLaunchCommand) {
    ghidra::core::trace_recording::RecordingRequest request{
        .program = L"C:\\Program Files\\Target\\game.exe",
        .arguments = {"--name", "value with spaces"},
        .output_trace = L"C:\\traces\\run01.run",
    };
    const auto command = recorder::WinTtdRecorder::build_command_line(request, L"C:\\Tools\\ttd.exe");
    EXPECT_NE(command.find(L"-noUI"), std::wstring::npos);
    EXPECT_NE(command.find(L"-out \"C:\\traces\\run01.run\""), std::wstring::npos);
    EXPECT_NE(command.find(L"-accepteula -launch"), std::wstring::npos);
    EXPECT_NE(command.find(L"\"value with spaces\""), std::wstring::npos);
}

/// Verifies that unsupported platforms fail clearly without attempting to launch a process.
TEST(WinTtdRecorder, ReportsUnsupportedPlatformWhenRecording) {
    recorder::WinTtdRecorder recorder_instance;
    ghidra::core::contracts::OperationContext context{};
    ghidra::core::trace_recording::RecordingRequest request{.program = "missing-program.exe",
                                                            .output_trace = "missing.run"};
    auto task = recorder_instance.record(std::move(request), context);
    const auto result = task.get();
#ifndef _WIN32
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, ghidra::core::DiagnosticCode::unsupported);
#else
    GTEST_SKIP() << "This test requires a non-Windows build.";
#endif
}

/// Verifies unknown recorder request options fail before launching TTD instead of being silently ignored.
TEST(WinTtdRecorder, RejectsUnsupportedRecordingOptions) {
    recorder::WinTtdRecorder recorder_instance;
    ghidra::core::trace_recording::RecordingRequest request{.program = "target.exe", .output_trace = "target.run"};
    request.options.emplace("unknown-option", "value");
    const auto result = recorder_instance.record(std::move(request), {}).get();
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code, ghidra::core::DiagnosticCode::unsupported);
}

/// Verifies child-process recording maps to Microsoft's documented `-children` switch.
TEST(WinTtdRecorder, BuildsChildRecordingCommand) {
    ghidra::core::trace_recording::RecordingRequest request{
        .program = "target.exe", .output_trace = "target.run", .record_children = true};
    const auto command = recorder::WinTtdRecorder::build_command_line(request, L"ttd.exe");
    EXPECT_NE(command.find(L"-accepteula -launch -children \"target.exe\""), std::wstring::npos);
}

/// Verifies the Windows-specific recorder requires the replay artifact extension used by TTD.
TEST(WinTtdRecorder, RejectsNonRunOutput) {
    recorder::WinTtdRecorder recorder_instance;
    ghidra::core::trace_recording::RecordingRequest request{.program = "target.exe", .output_trace = "target.ttd"};
    const auto result = recorder_instance.record(std::move(request), {}).get();
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code, ghidra::core::DiagnosticCode::invalid_argument);
}

/// Verifies recording never overwrites an existing trace artifact.
TEST(WinTtdRecorder, RejectsExistingOutput) {
    const auto output = std::filesystem::temp_directory_path() / "new-ghidra-existing.run";
    std::ofstream{output} << "existing";
    recorder::WinTtdRecorder recorder_instance;
    ghidra::core::trace_recording::RecordingRequest request{.program = "target.exe", .output_trace = output};
    const auto result = recorder_instance.record(std::move(request), {}).get();
    std::error_code cleanup_error;
    std::filesystem::remove(output, cleanup_error);
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code, ghidra::core::DiagnosticCode::conflict);
}

/// Verifies missing output directories fail before a TTD process can be launched.
TEST(WinTtdRecorder, RejectsMissingOutputDirectory) {
    const auto output = std::filesystem::temp_directory_path() / "new-ghidra-missing-directory" / "trace.run";
    recorder::WinTtdRecorder recorder_instance;
    ghidra::core::trace_recording::RecordingRequest request{.program = "target.exe", .output_trace = output};
    const auto result = recorder_instance.record(std::move(request), {}).get();
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code, ghidra::core::DiagnosticCode::invalid_argument);
}

/// Verifies a missing TTD executable produces a structured launch diagnostic before any trace is claimed.
TEST(WinTtdRecorder, ReportsMissingExecutable) {
#ifndef _WIN32
    GTEST_SKIP() << "Windows TTD process diagnostics require Windows.";
#else
    char* previous = nullptr;
    size_t previous_length = 0;
    _dupenv_s(&previous, &previous_length, "TTD_EXE");
    _putenv_s("TTD_EXE", "definitely-missing-ttd.exe");
    const auto output = std::filesystem::temp_directory_path() / "new-ghidra-missing-ttd.run";
    recorder::WinTtdRecorder recorder_instance;
    ghidra::core::trace_recording::RecordingRequest request{.program = "target.exe", .output_trace = output};
    const auto result = recorder_instance.record(std::move(request), {}).get();
    if (previous != nullptr)
        _putenv_s("TTD_EXE", previous);
    else
        _putenv_s("TTD_EXE", "");
    free(previous);
    std::error_code cleanup_error;
    std::filesystem::remove(output, cleanup_error);
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code, ghidra::core::DiagnosticCode::resource_unavailable);
#endif
}

/// Verifies clean-environment recording requests produce only the explicitly supplied variables.
TEST(WinTtdRecorder, BuildsExplicitEnvironmentBlock) {
#ifdef _WIN32
    ghidra::core::trace_recording::RecordingRequest request;
    request.environment = {{"TTD_TEST_VALUE", "expected"}};
    request.inherit_environment = false;
    const auto block = recorder::WinTtdRecorder::build_environment_block(request);
    const std::wstring text(block.data(), block.size());
    EXPECT_NE(text.find(L"TTD_TEST_VALUE=expected"), std::wstring::npos);
    EXPECT_EQ(text.find(L"PATH="), std::wstring::npos);
#else
    GTEST_SKIP() << "Windows environment blocks are unavailable on this platform.";
#endif
}

/// Verifies Task::cancel reaches a running recorder without requiring a second direct cancel call.
TEST(WinTtdRecorder, CancelsOptedInLongRecording) {
#ifndef _WIN32
    GTEST_SKIP() << "Windows TTD cancellation requires Windows.";
#else
    const char* program_name = std::getenv("TTD_TEST_CANCEL_PROGRAM");
    if (program_name == nullptr || *program_name == '\0')
        GTEST_SKIP() << "Set TTD_TEST_CANCEL_PROGRAM to an executable that remains alive for cancellation coverage.";
    const auto output = std::filesystem::temp_directory_path() / "new-ghidra-ttd-cancel-test.run";
    std::error_code cleanup_error;
    std::filesystem::remove(output, cleanup_error);
    auto recorder_service = recorder::create_win_ttd_recorder();
    ASSERT_TRUE(recorder_service) << recorder_service.error().message;
    ghidra::core::trace_recording::RecordingRequest request;
    request.program = program_name;
    request.output_trace = output;
    auto task = (*recorder_service)->record(std::move(request), {});
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    task.cancel();
    const auto result = task.get();
    ASSERT_TRUE(result) << result.error().message;
    EXPECT_TRUE(result->cancelled);
    std::filesystem::remove(output, cleanup_error);
#endif
}

/// Verifies the concrete implementation is constructible through the independent recorder contract.
TEST(WinTtdRecorder, FactoryReturnsIndependentContract) {
    const auto recorder_service = recorder::create_win_ttd_recorder();
    ASSERT_TRUE(recorder_service) << recorder_service.error().message;
    ghidra::core::contracts::OperationContext context{};
    auto task = (*recorder_service)->record({}, context);
    const auto result = task.get();
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code, ghidra::core::DiagnosticCode::invalid_argument);
}

/// Records an opted-in real executable through the contract and verifies TTD produced the requested artifact.
TEST(WinTtdRecorder, RecordsProvidedProgram) {
#if !defined(_WIN32)
    GTEST_SKIP() << "Windows TTD recording requires Windows";
#else
    const char* program_name = std::getenv("TTD_TEST_PROGRAM");
    if (program_name == nullptr || *program_name == '\0')
        GTEST_SKIP() << "Set TTD_TEST_PROGRAM to run the real ttd.exe integration test";
    const auto output = std::filesystem::temp_directory_path() / "new-ghidra-ttd-recorder-test.run";
    std::error_code cleanup_error;
    std::filesystem::remove(output, cleanup_error);
    const auto recorder_service = recorder::create_win_ttd_recorder();
    ASSERT_TRUE(recorder_service) << recorder_service.error().message;
    ghidra::core::trace_recording::RecordingRequest request;
    request.program = program_name;
    request.output_trace = output;
    if (std::getenv("TTD_TEST_AUTO_EXIT") != nullptr)
        request.arguments.emplace_back("--auto-exit");
    const auto recording_started = std::chrono::steady_clock::now();
    const auto result = (*recorder_service)->record(std::move(request), {}).get();
    const auto recording_elapsed =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - recording_started);
    ASSERT_TRUE(result) << result.error().message;
    ASSERT_TRUE(result->completed) << result->diagnostic;
    EXPECT_TRUE(std::filesystem::is_regular_file(result->trace));
    std::cout << "recording_wall_ms=" << recording_elapsed.count() << '\n';
    if (std::getenv("TTD_KEEP_TRACE") == nullptr)
        std::filesystem::remove(output, cleanup_error);
#endif
}
