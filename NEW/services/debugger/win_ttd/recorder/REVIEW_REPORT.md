# Independent Trace Recorder Review

This module-local report is the recorder portion of [`../REVIEW_REPORT.md`](../REVIEW_REPORT.md). It supersedes the earlier historical report so that findings describe the current nested adapter rather than the deleted sibling path.

## Scope and method

- [x] Scope confirmed: `win_ttd.cppm`, recorder CMake/tests/docs, `core/contracts/trace_recorder.cppm`, `core/domain/trace_recording.cppm`, service integration, and reference TTD recorder setup.
- [x] Review date: 2026-09-17.
- [x] Reviewer: Kilo, independent read-only implementation review.
- [x] Source and test inspection completed; no production source was edited.
- [x] Follow-up validation performed: focused build/tests, real `ttd.exe` recording, and recorder-to-Replay integration.

## Findings

### Critical

No findings.

### High

#### HIGH-001: `Task::cancel()` is disconnected from the worker

- [x] Remediation status: fixed by passing the returned operation token into the worker and adding opt-in cancellation coverage.
- **Source:** `win_ttd.cppm:87-114,162-167`; `core/contracts/operation.cppm:92-96`.
- **Affected component:** recorder task cancellation.
- **Technical evidence:** The returned `Task` owns a new `OperationControl`, but the worker does not capture it and `run` only checks `context.cancellation`.
- **Expected behavior:** `task.cancel()` requests and observes cancellation.
- **Actual behavior:** `task.cancel()` has no effect; only a separate direct recorder `cancel()` can terminate the process.
- **Impact:** Long-running recordings cannot be stopped through the standard operation API.
- **Failure scenario:** Start a recording, invoke `Task::cancel`, and wait for completion without invoking the recorder object’s `cancel` method.
- **Root cause:** Two cancellation channels were created without being joined.
- **Recommended fix:** Pass the operation control/token into the worker and centralize cancellation/termination.
- **Regression risks:** Keep cancellation idempotent before launch, during launch, and after process exit.
- **Relevant validation:** No test invokes `Task::cancel`.

#### HIGH-002: Environment and inheritance requests are silently ignored

- [x] Remediation status: fixed with explicit UTF-16 environment-block construction and clean-environment coverage.
- **Source:** `win_ttd.cppm:146-155`; `core/domain/trace_recording.cppm:16-18`.
- **Affected component:** child process environment.
- **Technical evidence:** `CreateProcessW` receives a null environment block, so the child always inherits the host environment; `RecordingRequest::environment` and `inherit_environment` are never read.
- **Expected behavior:** Apply the request’s environment policy or reject unsupported fields.
- **Actual behavior:** Custom values disappear and `inherit_environment=false` is ignored.
- **Impact:** Non-reproducible traces and possible leakage of host secrets/configuration.
- **Failure scenario:** Request a clean environment plus a sentinel variable and inspect the target process environment.
- **Root cause:** Only command-line fields were wired to process creation.
- **Recommended fix:** Construct a validated UTF-16 environment block and add child-observer tests, or return an explicit unsupported diagnostic.
- **Regression risks:** Preserve Windows case-insensitive key merge semantics and free the block only after process creation.
- **Relevant validation:** Existing tests never set either field.

#### HIGH-003: Exceptions escaping the worker can terminate the host process

- [x] Remediation status: fixed with exception-to-diagnostic conversion at the worker boundary and terminal operation status updates.
- **Source:** `win_ttd.cppm:104-113,137-193`.
- **Affected component:** asynchronous failure/error semantics.
- **Technical evidence:** The jthread invokes `promise->set_value(run(...))` without a try/catch. `run` performs allocations, path conversion, process setup, and filesystem queries; failures such as an exception from filesystem status or allocation escape the thread function.
- **Expected behavior:** Every realistic failure becomes `Result`/`Error` and the promise is fulfilled.
- **Actual behavior:** An exception escaping a `std::thread` entry function invokes `std::terminate`; callers do not receive a structured diagnostic.
- **Impact:** A malformed/inaccessible output path or resource exhaustion can abort the entire application instead of failing one recording.
- **Failure scenario:** Arrange an exception-producing filesystem status or allocation failure while `run` is active.
- **Root cause:** Native/error handling covers return codes but not C++ exceptions at the task boundary.
- **Recommended fix:** Catch `std::exception` and unknown exceptions in the worker, convert them to `Error`, and always execute state/handle cleanup with RAII.
- **Regression risks:** Do not swallow cancellation or overwrite a result already delivered.
- **Relevant validation:** No failure-injection test exists.

#### HIGH-004: `cancel()` races with process-handle closure

- [x] Remediation status: fixed by clearing shared HANDLE state under the mutex before closing.
- **Source:** `win_ttd.cppm:117-132,168-182`.
- **Affected component:** HANDLE ownership and cancellation.
- **Technical evidence:** `run` closes the local process handle before clearing shared `process_` under the mutex. `cancel()` can read the non-null member in that interval and call `TerminateProcess` on a closed handle.
- **Expected behavior:** Shared HANDLE ownership is serialized until all users stop accessing it.
- **Actual behavior:** A concurrent cancellation can produce `ERROR_INVALID_HANDLE` and inconsistent cancellation reporting.
- **Impact:** Nondeterministic cancellation and cleanup, including destructor-time races.
- **Failure scenario:** Race direct `cancel()` with the `WAIT_OBJECT_0` path between `CloseHandle` and the locked state reset.
- **Root cause:** Local and shared HANDLE lifetime is not governed by one RAII/lock protocol.
- **Recommended fix:** Move/clear the shared handle under the mutex before closing, or keep close under the same ownership protocol; use a unique HANDLE wrapper.
- **Regression risks:** Preserve termination of a still-running TTD process without holding the mutex while waiting.
- **Relevant validation:** No concurrent cancellation test exists.

### Medium

#### MEDIUM-001: `record_children` and backend options are ignored

- [x] Remediation status: fixed by mapping `record_children` to `-children` and rejecting unknown options.
- **Source:** `win_ttd.cppm:66-74`; `core/domain/trace_recording.cppm:17-20`.
- **Affected component:** trace scope/options.
- **Technical evidence:** The initial command contained no `-children` or option mapping; the corrected command now emits `-children` for `record_children` and rejects unknown options.
- **Expected behavior:** Map supported fields or reject them.
- **Actual behavior:** The request reports success with different trace scope/settings.
- **Impact:** Child activity and requested capture limits can be absent without warning.
- **Failure scenario:** Set `record_children=true` and launch a program that creates a child; inspect the generated command.
- **Root cause:** Minimal command-line mapping.
- **Recommended fix:** Add validated TTD switch mappings and reject unknown options.
- **Regression risks:** Avoid passing arbitrary untrusted options to an elevated TTD process.
- **Relevant validation:** Current command test does not set these fields.

#### MEDIUM-002: TTD elevation is not represented in setup or diagnostics

- [x] Remediation status: fixed with an explicit error-740 diagnostic and elevated-terminal documentation.
- **Source:** `win_ttd.cppm:146-156`, `README.md:3,9`; reference `TEST/debugger/TTD/LiveRecorderApiSample/main.cpp:103-145`.
- **Affected component:** Windows process startup.
- **Technical evidence:** The adapter uses direct `CreateProcessW`, while the reference sample uses `ShellExecuteW` to request elevation and the reference CLI documents an elevated terminal requirement.
- **Expected behavior:** State elevation as a precondition or implement a controlled elevation path; return a clear elevation remediation.
- **Actual behavior:** Normal hosts get a generic resource-unavailable CreateProcess error.
- **Impact:** Installed TTD appears broken and callers cannot diagnose error 740 from the service message.
- **Failure scenario:** Run the recorder from a non-elevated host with TTD installed.
- **Root cause:** Elevation behavior was omitted from the process adapter.
- **Recommended fix:** Detect `ERROR_ELEVATION_REQUIRED` and document/run an explicit user-approved elevation flow.
- **Regression risks:** Never silently elevate with sensitive command-line data.
- **Relevant validation:** No unelevated Windows test exists.

#### MEDIUM-003: Existing/non-`.run` output is not rejected

- [x] Remediation status: fixed with case-insensitive `.run`, parent-directory, and nonexistence validation plus focused tests.
- **Source:** `win_ttd.cppm:80-86`; command test `tests/win_ttd_tests.cppm:15-24`.
- **Affected component:** artifact safety and recorder/replay interoperability.
- **Technical evidence:** Only empty paths are rejected; existing outputs and arbitrary extensions are accepted. The test uses `.ttd`, while sibling replay validation accepts only `.run` (`../win_ttd.cppm:109-114`).
- **Expected behavior:** Validate `.run`, parent directory, and nonexistence or support alternate formats consistently.
- **Actual behavior:** TTD may overwrite an artifact or create an artifact the replay adapter rejects.
- **Impact:** Data loss and broken recorder-to-replay handoff.
- **Failure scenario:** Record to an existing `.run` or use the test’s `.ttd` output and pass it to replay.
- **Root cause:** Recorder and replay artifact policies are not shared.
- **Recommended fix:** Add synchronized output validation and deterministic tests.
- **Regression risks:** Preserve any intentionally supported format only after extending replay support.
- **Relevant validation:** Service-owned `.run`, parent-directory, and nonexistence validation is covered by focused tests.

#### MEDIUM-004: Recorder tests do not cover process lifecycle failures

- [x] Remediation status: substantially fixed with missing-executable, invalid-directory, existing-output, environment, cancellation, and real-recording tests; nonzero-exit/elevation remain environment-dependent.
- **Source:** `tests/win_ttd_tests.cppm:13-75`.
- **Affected component:** regression coverage.
- **Technical evidence:** Tests cover command substrings, invalid input/platform behavior, and one environment-dependent success. Missing executable, nonzero exit, cancellation, environment, elevation, and cleanup are untested.
- **Expected behavior:** Deterministic tests assert every native result path while keeping real TTD integration additive.
- **Actual behavior:** The suite passes without TTD and cannot detect the high/medium defects above.
- **Impact:** Lifecycle regressions can ship unnoticed.
- **Failure scenario:** Break exit-code mapping or cancellation wiring; existing tests remain green unless external integration is enabled.
- **Root cause:** Coverage is concentrated on command construction and opt-in happy path.
- **Recommended fix:** Inject a controlled process launcher/fake executable and add focused cancellation/environment/failure tests.
- **Regression risks:** Keep real `ttd.exe` coverage separate and cleanup-safe.
- **Relevant validation:** `tests/CMakeLists.txt:1-10` registers only one small executable.

#### MEDIUM-005: All inheritable host handles are passed to TTD

- [x] Remediation status: fixed by passing `FALSE` for `bInheritHandles`.
- **Source:** `win_ttd.cppm:154-155`.
- **Affected component:** process isolation.
- **Technical evidence:** `CreateProcessW` uses `bInheritHandles=TRUE` without an explicit handle allow-list.
- **Expected behavior:** Child processes inherit no unrelated service handles.
- **Actual behavior:** Any inheritable handle can leak into TTD and the launched target.
- **Impact:** Resource retention and possible security exposure.
- **Failure scenario:** Create an inheritable service IPC/file handle before recording and inspect cleanup or child handle state.
- **Root cause:** Permissive process-launch flag with no documented requirement.
- **Recommended fix:** Use `FALSE` or `STARTUPINFOEXW` with an explicit handle list after verifying TTD requirements.
- **Regression risks:** Confirm TTD does not require inherited standard handles.
- **Relevant validation:** No handle-isolation test exists.

### Low

No findings.

## Verified strengths

- [x] The adapter exposes a portable `ITraceRecorder` rather than TTD types.
- [x] It uses `CreateProcessW`, closes the thread handle, waits for process completion, and reports nonzero exit/missing-artifact outcomes.
- [x] Non-Windows calls return an explicit unsupported diagnostic.
- [x] The recorder has an independent target/alias and focused CTest registration.

## Reviewed areas with no confirmed findings

- The command-line quoting routine handles spaces, embedded quotes, and trailing backslashes according to the intended Windows quoting shape; broader Unicode behavior remains an unresolved risk because `wide()` uses filesystem locale conversion.
- The factory returns the independent recorder contract and does not add live-debugger methods.

## Validation results

- [x] Direct source, contract, CMake, test, and reference inspection completed.
- [x] Read-only `git diff --check` completed in the repository review.
- [x] Focused build/CTest passed after recorder relocation and lifecycle test expansion.
- [x] Real `ttd.exe` recording produced a `.run` from the multi-thread debuggee.
- [x] The unified dependency bootstrap reran successfully after the recorder move.
- [x] Environment-block and task-cancellation paths are covered by focused tests; opt-in cancellation remains environment-dependent.

## Unresolved questions and residual risks

- TTD's exact elevation behavior can vary by installation/manifest; confirm whether the deployed `ttd.exe` requires `runas` for this launch mode.
- Define whether `RecordingRequest::options` is intentionally backend-specific or must be rejected when unknown.
- `wide(std::string_view)` claims UTF-8 conversion but uses `std::filesystem::path` locale conversion (`win_ttd.cppm:44-47`); add a Unicode test before relying on non-ASCII target arguments.

## Final follow-up decision

- [x] HIGH-001 through HIGH-004 remediation is complete.
- [x] MEDIUM-001 through MEDIUM-005 remediation is complete, with nonzero-exit/elevation residual cases documented.
