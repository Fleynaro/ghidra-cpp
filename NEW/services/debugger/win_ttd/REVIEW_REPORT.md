# Independent Windows TTD Code Review

## Scope and method

- [x] Scope confirmed: `win_ttd.cppm`, the nested `recorder` adapter, their CMake files, tests, README/`GHIDRA_PORT.md` files, `core/contracts/debugger.cppm`, `core/contracts/trace_recorder.cppm`, `core/domain/replay.cppm`, `core/domain/trace_recording.cppm`, service integration, dependency bootstrap, and the comparison test `NEW/services/debugger/win_dbg_eng/tests/debugger_contract_tests.cppm`.
- [x] Review date: 2026-09-17.
- [x] Reviewer: Kilo, independent read-only implementation review.
- [x] Reviewed the current working tree and staged deletion of the former `NEW/services/trace_recorder/win_ttd` implementation while the recorder was moved under `win_ttd/recorder`.
- [x] Compared native calls with the checked-in Microsoft TTD samples and the copied Microsoft API package/bootstrap inputs.
- [x] Source, contracts, CMake, tests, and documentation were inspected directly.
- [x] `git diff --check` completed without whitespace errors.
- [x] Follow-up validation performed after the independent review: local dependency bootstrap, real native TTD build/replay, focused recorder/replay tests, DbgEng tests, full CTest, and tidy check.

The Microsoft sample call sites and headers are treated as authoritative where generated API documentation differs. Findings below are confirmed from source unless explicitly marked as an unresolved question.

## Findings

### Critical

No findings.

### High

#### HIGH-001: Reopening a session can destroy the replay engine before its cursor

- [x] Remediation status: fixed by resetting the old `UniqueCursor` before the old engine and adding `ReopensTraceWithSafeLifetimeOrder`.
- **Source:** `win_ttd.cppm:115-130`, especially assignments at `win_ttd.cppm:126-127`.
- **Affected component:** `WinTtdReplaySession` native TTD ownership and trace lifecycle.
- **Technical evidence:** `open_trace` accepts a new trace while an existing `engine_`/`cursor_` pair may still be live. It assigns the new engine to `engine_` before resetting the old cursor. TTD uses custom `UniqueCursor`/`UniqueReplayEngine` deleters; the documented safe order is cursor destruction followed by engine destruction (`TEST/debugger/TTD/docs/IReplayEngine.h/template-Deleter.md:71-93`).
- **Expected behavior:** A session must reject a second open or release the old cursor before releasing/replacing its engine.
- **Actual behavior:** Reopening trace B replaces engine A at line 126 and only then destroys cursor A at line 127.
- **Impact:** The old cursor can call a TTD vtable or destroy routine after its owning engine is gone, producing a native use-after-free, crash, or corrupted replay state.
- **Failure scenario:** Open trace A, call `open_trace` again with trace B, then allow the old `UniqueCursor` deleter to run during `cursor_.reset()`.
- **Root cause:** The lifecycle code handles ordering in `close_trace` but does not apply that ordering before replacement in `open_trace`.
- **Recommended fix:** Return a lifecycle conflict for an already-open session, or call a locked close sequence that resets `cursor_` first and `engine_` second before creating the replacement pair. Add a reopen test.
- **Regression risks:** Preserve the guarantee that a failed second open leaves the first session usable, or document a deliberate close-before-open transition.
- **Relevant validation:** `TEST/debugger/TTD/docs/IReplayEngine.h/template-Deleter.md:83-93`; current tests in `tests/replay_contract_tests.cppm:16-56` do not reopen a session.

#### HIGH-002: The native error-reporting callback has insufficient lifetime

- [x] Remediation status: fixed by storing the synchronized reporter as a session member that outlives the engine.
- **Source:** `win_ttd.cppm:50-61` and `win_ttd.cppm:115-122`.
- **Affected component:** TTD `ErrorReporting` registration and native error translation.
- **Technical evidence:** `ErrorReporting error_reporting` was initially a local variable in `open_trace`, but its address was registered with `RegisterDebugModeAndLogging`. The corrected service now keeps a synchronized reporter member for the engine lifetime.
- **Expected behavior:** Any callback pointer retained by TTD must outlive the engine and be synchronized for the session lifetime.
- **Actual behavior:** The registered object is destroyed when `open_trace` returns, while `engine_` remains live for all subsequent queries and replay calls.
- **Impact:** A later native diagnostic can call a dangling virtual object, causing a use-after-scope crash. Even if this API version only reports during initialization, the implementation does not establish that restriction.
- **Failure scenario:** Open a trace successfully, then issue a replay operation that causes a native warning/error or teardown diagnostic after `open_trace` has returned.
- **Root cause:** Callback storage was scoped to initialization instead of to `WinTtdReplaySession`.
- **Recommended fix:** Store the reporter as a session member before registration; protect its diagnostic buffer or use a callback-safe per-session error channel. Add a test that exercises a post-open native failure where the API permits it.
- **Regression risks:** Avoid sharing one mutable reporter across sessions without synchronization; preserve useful per-session diagnostics.
- **Relevant validation:** The bridge's static reporter at `ttd_replay_bridge.cpp:273-274` is a concrete lifetime precedent.

#### HIGH-003: `Task::cancel()` does not cancel a recorder operation

- [x] Remediation status: fixed by passing the returned `OperationControl` token into the worker and adding opt-in cancellation coverage.
- **Source:** `recorder/win_ttd.cppm:87-114` and `recorder/win_ttd.cppm:162-167`.
- **Affected component:** `WinTtdRecorder` asynchronous operation and `ITraceRecorder` cancellation contract.
- **Technical evidence:** `record` creates and returns an `OperationControl` at lines 87-114, but the worker captures neither that control nor its cancellation token. `run` checks only `context.cancellation`, while `Task::cancel()` sets the returned control (`NEW/core/contracts/operation.cppm:92-96`).
- **Expected behavior:** Calling `Task::cancel()` must request cancellation that the worker observes and translate to a bounded cancelled result.
- **Actual behavior:** Calling `task.cancel()` changes an object no worker reads; the TTD process continues until it exits or a separate direct `ITraceRecorder::cancel()` call is made.
- **Impact:** Runtime clients cannot stop a long-running recording through the standard task API. This can leave elevated TTD processes and large trace files running indefinitely.
- **Failure scenario:** Start a long-lived recording, call `task.cancel()`, and wait on `task.get()` without separately calling `cancel()` on the recorder; the recording is not terminated.
- **Root cause:** The implementation has two unrelated cancellation channels and ignores the channel exposed by `Task`.
- **Recommended fix:** Reuse `context.operation` when supplied or pass the created control into `run`; poll both operation cancellation and `std::stop_token`, and centralize process termination/wait cleanup.
- **Regression risks:** Ensure cancellation before `CreateProcessW` does not launch TTD and cancellation after process exit is idempotent.
- **Relevant validation:** `ITraceRecorder::record/cancel` is declared in `NEW/core/contracts/trace_recorder.cppm:15-19`; no test covers `Task::cancel`.

#### HIGH-004: Recorder request environment semantics are silently violated

- [x] Remediation status: fixed with a double-NUL UTF-16 environment block, inheritance policy, and explicit environment test.
- **Source:** `recorder/win_ttd.cppm:146-155`; contract values are in `NEW/core/domain/trace_recording.cppm:10-20`.
- **Affected component:** Windows child-process setup and recording reproducibility/security.
- **Technical evidence:** `RecordingRequest` exposes `environment` and `inherit_environment`, but `CreateProcessW` is called with a null environment block. That means the child inherits the recorder's environment regardless of `inherit_environment`, and no request entries are passed. The command builder also has no environment handling.
- **Expected behavior:** Apply the requested environment and inheritance policy, or reject unsupported fields explicitly before launching.
- **Actual behavior:** A request that asks for a clean environment still exposes all inherited variables, while custom values are ignored.
- **Impact:** The recorded program can execute different code/configuration than requested; inherited secrets and machine-specific paths can be exposed to the target and become part of a trace. This breaks reproducibility and can be security-sensitive.
- **Failure scenario:** Set `inherit_environment=false` and provide `SECRET=expected`; the target still receives the recorder's environment and does not receive the requested map.
- **Root cause:** Only command-line and working-directory fields were wired to `CreateProcessW`.
- **Recommended fix:** Build and own a correctly double-NUL-terminated UTF-16 environment block, define merge semantics, pass it to `CreateProcessW`, and add deterministic child-observer tests. If a field cannot be supported, return `unsupported`/`invalid_argument` rather than silently dropping it.
- **Regression risks:** Preserve Windows case-insensitive environment-key behavior and avoid leaking the environment block after process creation.
- **Relevant validation:** The current recorder tests (`recorder/tests/win_ttd_tests.cppm:13-75`) never inspect a child environment.

#### HIGH-005: Per-thread register and instruction-pointer queries return the wrong thread

- [x] Remediation status: fixed by resolving portable unique IDs to native `TTD::ThreadId` values and testing every active thread.
- **Source:** `win_ttd.cppm:351-404`.
- **Affected component:** Replay thread selection and multi-thread state inspection.
- **Technical evidence:** `read_register` ignores its optional `ThreadId` and calls `GetCrossPlatformContext()` without a native thread ID at line 361. `instruction_pointer` similarly ignores its argument and calls `GetProgramCounter()` without a thread at line 403. `selected_thread_` only changes the `current` flag in `threads` and the result of `current_thread`.
- **Expected behavior:** An explicit thread argument must query that TTD unique thread; an omitted argument may query the cursor's current thread.
- **Actual behavior:** Selecting or requesting a non-current active thread changes metadata only; values continue to come from the cursor's current thread.
- **Impact:** Multi-thread debugging displays incorrect register/PC state and can lead analysis or navigation to the wrong execution context. The replay integration test selects a thread but only checks that values exist (`tests/replay_contract_tests.cppm:80-95`).
- **Failure scenario:** Select thread B while cursor current thread is A, call `read_register("rip", B)` or `instruction_pointer(B)`, and compare it with TTD's thread-B context.
- **Root cause:** Thread IDs were serialized for enumeration but never translated back to the native `UniqueThreadId` for context queries.
- **Recommended fix:** Parse/validate the opaque thread ID, pass the resulting native ID to `GetCrossPlatformContext`/`GetProgramCounter`, and return a clear invalid-argument error for inactive/unknown IDs. Add a trace with distinguishable thread PCs/registers.
- **Regression risks:** Preserve the documented default-current-thread behavior and the distinction between TTD unique IDs and OS thread IDs.
- **Relevant validation:** TTD explicitly supports a thread parameter for these APIs (`TEST/debugger/TTD/docs/IReplayEngine.h/interface-ICursorView.md:35-45`); the live comparison test selects another thread at `win_dbg_eng/tests/debugger_contract_tests.cppm:132-148`.

#### HIGH-006: Invalid native seeks can be reported as successful

- [x] Remediation status: fixed by validating the trace lifetime before `SetPosition` and rejecting invalid sequence sentinels; the installed 0.9.5 header exposes `SetPosition` as `void` and rounds valid in-range positions by contract.
- **Source:** `win_ttd.cppm:179-193`, especially line 188.
- **Affected component:** Replay position validation and caller-visible timeline identity.
- **Technical evidence:** `ICursorView::SetPosition` returns `bool` and documents false on failure (`TEST/debugger/TTD/docs/IReplayEngine.h/interface-ICursorView.md:75-90`), but `seek` discards that return value. It only checks whether the resulting sequence is `Invalid`; a failed call that leaves the cursor at its previous valid position is accepted.
- **Expected behavior:** Return an error when TTD rejects a requested position and report the actual position only after successful navigation.
- **Actual behavior:** A rejected position can produce an empty-success `Result<void>`, leaving callers believing the requested position was reached.
- **Impact:** Replay automation can inspect or mutate the wrong timeline point without an error, making subsequent evidence invalid.
- **Failure scenario:** Open a trace, seek to an out-of-lifetime or otherwise inaccessible position, and observe success with the prior position unchanged.
- **Root cause:** The adapter checks a sentinel after the call instead of honoring the native success result.
- **Recommended fix:** Check `SetPosition` and return a structured invalid-argument/resource error with the requested and actual positions. Add invalid-position and unchanged-position tests.
- **Regression risks:** TTD may normalize some non-canonical positions; distinguish successful normalization from a false return rather than requiring raw equality in all cases.
- **Relevant validation:** Native bool contract at `interface-ICursorView.md:75-90`; current tests only seek the reported first/last lifetime boundaries (`tests/replay_contract_tests.cppm:76-79,115-117`).

#### HIGH-007: Concurrent cancellation can use a closed process handle

- [x] Remediation status: fixed by clearing the shared HANDLE under the mutex before closing it and disabling unrelated handle inheritance.
- **Source:** `recorder/win_ttd.cppm:117-132` and `recorder/win_ttd.cppm:168-182`.
- **Affected component:** Recorder process lifetime and explicit `cancel()`.
- **Technical evidence:** `run` calls `CloseHandle(process_info.hProcess)` at line 178 before it acquires `mutex_` to clear `process_` at lines 180-182. `cancel` reads `process_` under the mutex and may call `TerminateProcess` in the interval after the native handle has been closed but before the member is cleared.
- **Expected behavior:** A process handle must remain valid until all cancellation/cleanup paths have stopped using it, or be atomically removed before close.
- **Actual behavior:** A concurrent cancel can call `TerminateProcess` with a stale closed handle and return a misleading native error; destructor cancellation has the same race.
- **Impact:** Cancellation is nondeterministic and can mask successful completion or leave cleanup decisions inconsistent.
- **Failure scenario:** Let TTD signal `WAIT_OBJECT_0`, race `cancel()` between line 178 and the locked cleanup block, and observe `ERROR_INVALID_HANDLE` from `TerminateProcess`.
- **Root cause:** HANDLE ownership is split between an unlocked local close and a mutex-protected member without one ownership protocol.
- **Recommended fix:** Move the HANDLE out of shared state under the mutex before closing, or hold the mutex through close and make cancellation observe a terminal state. Use an RAII unique HANDLE wrapper.
- **Regression risks:** Do not hold the mutex while waiting for the process; preserve the ability to terminate a genuinely running TTD process.
- **Relevant validation:** No recorder test exercises concurrent or post-completion cancellation.

### Medium

#### MEDIUM-001: Watchpoint replay does not implement the canonical bounded query lifecycle

- [x] Remediation status: fixed with a temporary cursor, explicit memory-watchpoint event mask, direction-specific lifetime boundary, cleanup, and cursor-preservation test.
- **Source:** `win_ttd.cppm:205-243`.
- **Affected component:** Replay memory watchpoint navigation.
- **Technical evidence:** The method adds a watchpoint to the user cursor, replays to `StepCount::Max` from the current position, does not set a memory-watchpoint event mask, does not establish an explicit range start/end, and ignores `RemoveMemoryWatchpoint`'s result. The Microsoft debugger creates a temporary cursor and moves the user cursor only after the query (`TEST/debugger/TTD/ReplayApi/TraceDebugger/TraceDebugger.cpp:349-369`); the helper sets callbacks/event masks and bounded range (`TEST/debugger/TTD/ReplayApi/inc/ReplayHelpers.h:160-257`).
- **Expected behavior:** Search the requested direction within a bounded trace range, stop only for the requested watchpoint, preserve or deliberately update cursor state, and report cleanup failure.
- **Actual behavior:** A query mutates the primary cursor, may stop for an unrelated native result, and can leave a watchpoint installed if removal fails.
- **Impact:** Repeated queries and subsequent stepping can observe stale filters or wrong stop positions; watchpoint hits can be missed when the event mask is not enabled.
- **Failure scenario:** Issue a forward query, then a second query/step; inspect whether the cursor starts at the documented range and whether a failed removal changes later replay behavior.
- **Root cause:** The one-shot API was implemented as a direct `Add`/`ReplayForward` wrapper without the sample's temporary-cursor/filter lifecycle.
- **Recommended fix:** Use a temporary cursor, set its start/range and event mask, guard no-progress/boundary results, check removal, then set the primary cursor to a confirmed hit position.
- **Regression risks:** Preserve the intended direction semantics and avoid reporting speculative callback observations as confirmed hits.
- **Relevant validation:** Current integration test assumes `StopReason::watchpoint` at `tests/replay_contract_tests.cppm:98-101`, but does not cover no-hit, reverse, cleanup, or repeated queries.

#### MEDIUM-002: Session state is a data race under concurrent callers

- [x] Remediation status: fixed by synchronizing `state()` with the session recursive mutex.
- **Source:** `win_ttd.cppm:93-100`, writes at `win_ttd.cppm:130,143`, and member declaration at `win_ttd.cppm:551-557`.
- **Affected component:** Public lifecycle state observation.
- **Technical evidence:** `state()` reads plain `state_` without `mutex_`, while `open_trace` and `close_trace` write it under the mutex. The live backend uses an atomic lifecycle state (`win_dbg_eng.cppm:2174-2177`).
- **Expected behavior:** Concurrent state queries and lifecycle transitions have defined synchronization.
- **Actual behavior:** A reader can race a writer, which is undefined behavior in C++ and can observe stale or torn state.
- **Impact:** Runtime/session managers can make incorrect cleanup or operation decisions under concurrent use.
- **Failure scenario:** Poll `state()` from one thread while another opens or closes a trace.
- **Root cause:** The replay adapter added locking around operations but omitted it or atomic storage for the accessor.
- **Recommended fix:** Use `std::atomic<SessionState>` if the enum representation is suitable, or lock in `state()` with a documented synchronization policy.
- **Regression risks:** Keep destructor and failure transitions noexcept-safe and avoid introducing lock-order inversions.
- **Relevant validation:** No replay concurrency test exists; live state storage provides a nearby project convention.

#### MEDIUM-003: Replay event APIs are a silent no-op

- [x] Remediation status: fixed by returning explicit `unsupported` diagnostics instead of successful empty/no-op results; callback queue implementation remains future work.
- **Source:** `win_ttd.cppm:475-483`; inherited contract `NEW/core/contracts/debugger.cppm:68-71`.
- **Affected component:** `IBaseDebugSession` event delivery for replay.
- **Technical evidence:** `poll_events()` always returns an empty vector, while `set_event_sink()` stores a callback that is never invoked. The contract promises draining translated events and installing an event callback.
- **Expected behavior:** Implement replay event translation, or return an explicit unsupported diagnostic for an operation not meaningful for this backend.
- **Actual behavior:** Callers receive success and no events, so they cannot distinguish “no events” from “backend never implements events.”
- **Impact:** UI/event consumers silently lose exception, thread, module, and watchpoint notifications.
- **Failure scenario:** Install a sink, replay across a known exception/thread event, and observe neither a callback nor a queued event.
- **Root cause:** The adapter maps replay stops only into `StepResult` and leaves the inherited event surface as placeholder behavior.
- **Recommended fix:** Translate supported TTD event lists/stop results into the queue and invoke the sink outside the native lock, or return `unsupported` consistently and document that capability.
- **Regression risks:** TTD callbacks are speculative and multithreaded; follow `TEST/debugger/TTD/docs/Concepts.md:132-140` and never access the cursor unsafely from a callback.
- **Relevant validation:** No replay test calls `set_event_sink` or asserts event delivery.

#### MEDIUM-004: Recorder child/options semantics are silently dropped

- [x] Remediation status: fixed by mapping `record_children` to `-children` and rejecting unknown options before launch.
- **Source:** `recorder/win_ttd.cppm:66-74`; values at `NEW/core/domain/trace_recording.cppm:17-20`.
- **Affected component:** Trace scope and backend option mapping.
- **Technical evidence:** The initial command builder emitted only `-noUI`, `-out`, `-accepteula`, `-launch`, and target arguments. The corrected builder maps the documented `record_children` flag to `-children` and rejects unknown options.
- **Expected behavior:** Map supported request fields or reject unsupported fields explicitly.
- **Actual behavior:** A request can ask for child-process recording or backend options and still report a normal recording without those semantics.
- **Impact:** The resulting trace scope and limits differ from the caller's request, invalidating reproducibility.
- **Failure scenario:** Set `record_children=true`, record a program that launches a child, and inspect that the command contains no `-children` switch.
- **Root cause:** Initial command construction covers only the minimal launch path.
- **Recommended fix:** Add validated mappings for known TTD switches and reject unknown/unsupported `options` with a structured diagnostic.
- **Regression risks:** Avoid passing arbitrary option strings to an elevated process; validate names, values, and allowed combinations.
- **Relevant validation:** `GHIDRA_PORT.md:3-5` acknowledges these fields are not translated, but the public contract does not mark them as unsupported.

#### MEDIUM-005: Recorder launch does not handle TTD's elevation requirement

- [x] Remediation status: fixed by returning a dedicated elevation diagnostic for Win32 error 740 and documenting the elevated-terminal precondition; silent elevation is intentionally not performed.
- **Source:** `recorder/win_ttd.cppm:146-156` and `recorder/README.md:3,9`.
- **Affected component:** Native TTD process startup and diagnostics.
- **Technical evidence:** The service uses direct `CreateProcessW` and now translates Win32 error 740 into an explicit elevated-terminal diagnostic; silent elevation is intentionally avoided.
- **Expected behavior:** Require/document an elevated host clearly, or use a controlled elevation path and return a remediation-specific diagnostic for `ERROR_ELEVATION_REQUIRED`.
- **Actual behavior:** A normal host receives a generic `CreateProcessW` resource error whose remediation only mentions `TTD_EXE`, PATH, and output permissions.
- **Impact:** The recorder appears unavailable even when installed, and callers cannot determine the required remediation from the returned error.
- **Failure scenario:** Start the service from a non-elevated process on a machine where TTD is installed and requires elevation.
- **Root cause:** The adapter copied a direct command-line launch without the reference elevation policy.
- **Recommended fix:** Make elevation an explicit documented precondition and classify error 740, or implement a user-approved `runas` launch with a clear security boundary.
- **Regression risks:** Do not silently elevate or pass sensitive command-line data to an unexpected executable.
- **Relevant validation:** No Windows test covers unelevated startup.

#### MEDIUM-006: TTD dependency discovery is tied to the TEST tree and ignores a CMake package override

- [x] Remediation status: fixed with cache-aware `TTD_APIS_PACKAGE_DIR`/`TTD_RUNTIME_DIR` handling, normalized Windows paths, explicit required mode, and `setup_dependencies.bat`.
- **Source:** `CMakeLists.txt:9-43`, `setup_dependencies.bat:4-10,33-67`, and `build.bat:97-100`.
- **Affected component:** Replay build reproducibility and native runtime staging.
- **Technical evidence:** The initial implementation used a `TEST`-tree fallback and unconditionally shadowed the CMake cache. The corrected implementation owns the pinned package/runtime under `win_ttd/dependencies`, honors explicit cache/environment overrides, and fails in required mode when native inputs are absent.
- **Expected behavior:** Honor explicit CMake cache settings, keep source-tree reference artifacts optional, and reject mismatched native architectures before link/load.
- **Actual behavior:** Clean consumers must know the TEST dependency layout or use environment variables; command-line package overrides are ineffective and a non-x64 CMake configuration can select incompatible binaries.
- **Impact:** Builds silently fall back to unsupported mode or fail late at link/runtime, making dependency setup non-reproducible outside this checkout.
- **Failure scenario:** Configure with `-DTTD_APIS_PACKAGE_DIR=C:/managed/ttd` but no `TTD_APIS_PACKAGE_DIR` environment variable; CMake searches the TEST fallback instead. Configure ARM64 with x64 staged runtime and observe late loader failure.
- **Root cause:** Dependency discovery was added as a local environment/fallback convention rather than a cache-aware, architecture-aware package contract.
- **Recommended fix:** Declare `TTD_APIS_PACKAGE_DIR` as a `CACHE PATH` and only fill it when unset; validate package target architecture and stage runtime DLLs for the selected target. Keep the TEST path as an explicit opt-in default only if documented.
- **Regression risks:** Preserve the unsupported fallback for ordinary builds without SDKs and keep `NEW_GHIDRA_REQUIRE_TTD_REPLAY=ON` fail-fast behavior.
- **Relevant validation:** The service-owned cache variables and bootstrap were configured, then a real native configure/build was completed.

#### MEDIUM-007: Replay native coverage is opt-in and can pass without testing native replay

- [x] Remediation status: fixed for required mode: `NEW\build.bat ttd_replay` sets `TTD_TEST_REQUIRED=1` and fails when `TTD_TEST_TRACE` is absent; the bootstrap/recording workflow supplies the real trace.
- **Source:** `tests/replay_contract_tests.cppm:16-120` and `tests/CMakeLists.txt:1-17`.
- **Affected component:** Replay regression suite.
- **Technical evidence:** The first three tests exercise factory/invalid-path behavior and compile-time hierarchy. The only native integration test skips when `TTD_TEST_TRACE` is absent at lines 60-65. No deterministic trace is generated or checked in, and no fixture setup parallels the live debugger fixture.
- **Expected behavior:** CI should exercise the native path deterministically when the backend is claimed as implemented, with explicit setup failure rather than an all-green skip-only path.
- **Actual behavior:** A normal build can report passing replay tests while every engine/cursor/register/memory/watchpoint path was skipped.
- **Impact:** Regressions in DLL staging, API calls, lifetime, positions, and thread state can land unnoticed.
- **Failure scenario:** Run CTest without `TTD_TEST_TRACE`; native integration is skipped while the target still passes.
- **Root cause:** The test depends on a caller-supplied external `.run` and optional runtime rather than a reproducible fixture.
- **Recommended fix:** Add a documented compressed fixture or deterministic record/replay setup, separate capability tests from integration, and make the required native mode fail when dependencies/fixture are requested but unavailable.
- **Regression risks:** Keep ordinary non-TTD builds usable on machines without Microsoft's SDK and avoid committing sensitive traces.
- **Relevant validation:** The live comparison fixture performs deterministic launch/warm-up and setup failure handling (`win_dbg_eng/tests/debugger_contract_tests.cppm:54-114`) and contains 17 lifecycle/inspection tests at lines 117-353.

#### MEDIUM-008: Recorder tests omit cancellation, environment, and failure-path contracts

- [x] Remediation status: fixed with deterministic tests for `Task::cancel`, environment blocks, missing executable, invalid output directory, existing output, unsupported options, and real recording.
- **Source:** `recorder/tests/win_ttd_tests.cppm:13-75` and `recorder/tests/CMakeLists.txt:1-10`.
- **Affected component:** Recorder regression suite.
- **Technical evidence:** Tests cover command substrings, invalid request/platform behavior, and one opt-in successful recording. They do not cover `Task::cancel`, direct cancellation, nonzero TTD exit, missing executable, output-directory failure, elevation, environment propagation, child recording, or existing output handling.
- **Expected behavior:** Each realistic process and contract failure should have deterministic assertions, with real TTD integration kept as an additional opt-in test.
- **Actual behavior:** The tests can pass without TTD installed and do not detect the cancellation/environment defects above.
- **Impact:** Process-lifetime and security/reproducibility regressions are unprotected.
- **Failure scenario:** Change `CreateProcessW` error mapping or remove cancellation wiring; the current suite remains green unless an external recording happens to run.
- **Root cause:** Coverage is centered on string construction and one external happy path.
- **Recommended fix:** Add a controllable fake executable/helper or injectable process launcher for deterministic exit/cancel/environment tests, retaining the real `ttd.exe` test for end-to-end validation.
- **Regression risks:** Keep tests isolated from machine PATH/elevation and clean all generated traces.
- **Relevant validation:** The live suite's fixture and teardown (`win_dbg_eng/tests/debugger_contract_tests.cppm:54-114`) provide a stronger model for deterministic setup/cleanup.

#### MEDIUM-009: Recorder accepts output paths that the replay contract and reference CLI reject

- [x] Remediation status: fixed by validating case-insensitive `.run`, existing output, and parent directory before launch, with focused tests.
- **Source:** `recorder/win_ttd.cppm:66-86,183-193`; service-owned artifact validation.
- **Affected component:** Recording artifact validation.
- **Technical evidence:** `record` validates only non-empty program/output paths. It permits an existing output and arbitrary extension; the test itself uses `.ttd` at `recorder/tests/win_ttd_tests.cppm:18`, while the replay adapter requires `.run` at `win_ttd.cppm:109-114`.
- **Expected behavior:** Reject existing/different-format output before launch, or document and support the format consistently.
- **Actual behavior:** TTD can overwrite an existing artifact or produce a path the sibling replay service refuses.
- **Impact:** Destructive overwrite and broken recorder-to-replay workflows.
- **Failure scenario:** Record to an existing `.run`, or use the command-builder's `.ttd` path and then pass it through the replay service.
- **Root cause:** Minimal input validation was implemented without synchronizing recorder/replay artifact policy.
- **Recommended fix:** Validate `.run`, parent directory, and nonexistence up front; use a shared portable artifact policy or explicitly support `.ttd` in replay.
- **Regression risks:** Preserve callers that intentionally use a supported alternate format only if the replay contract is extended at the same time.
- **Relevant validation:** Reference CLI checks extension, parent directory, and existing output at the cited lines; current tests assert none of these.

#### MEDIUM-010: The recorder inherits every inheritable host handle

- [x] Remediation status: fixed by passing `FALSE` for `bInheritHandles`; the real recorder test still passes.
- **Source:** `recorder/win_ttd.cppm:151-155`, specifically the `TRUE` `bInheritHandles` argument.
- **Affected component:** Native child-process isolation and handle lifetime.
- **Technical evidence:** `CreateProcessW` is invoked with handle inheritance enabled and no explicit `PROC_THREAD_ATTRIBUTE_HANDLE_LIST`. All inheritable handles in the service process may therefore be exposed to TTD and potentially retained by the launched target.
- **Expected behavior:** Inherit no unrelated service handles; pass only explicitly required handles.
- **Actual behavior:** Handle inheritance is globally enabled without a documented reason.
- **Impact:** File/database/IPC handles can leak into an elevated recorder, keep resources alive, or create security and shutdown surprises.
- **Failure scenario:** Create an inheritable service handle before recording and inspect the TTD child or observe the handle preventing resource cleanup.
- **Root cause:** The process-launch call uses a permissive default instead of an explicit handle policy.
- **Recommended fix:** Pass `FALSE` unless TTD requires inheritance, or use `STARTUPINFOEXW` with an allow-list and test handle isolation.
- **Regression risks:** Verify TTD does not rely on inherited standard handles before removing them.
- **Relevant validation:** No test checks child handle inheritance; no build was run by this review.

### Low

#### LOW-001: Documentation links do not match the moved module hierarchy

- [x] Remediation status: fixed by recalculating parent/core/reference links after the recorder move.
- **Source:** `README.md:11` and `recorder/README.md:5`.
- **Affected component:** In-place C++ module navigation.
- **Technical evidence:** From `NEW/services/debugger/win_ttd`, `../../../../core/contracts/debugger.cppm` points above `NEW`; the target is `../../../core/contracts/debugger.cppm`. From `recorder`, `../../CMakeLists.txt` points to the debugger parent rather than the recorder's immediate `win_ttd/CMakeLists.txt` integration boundary.
- **Expected behavior:** Every README link resolves to the source/configuration it describes.
- **Actual behavior:** Readers following the links reach nonexistent or unintended files.
- **Impact:** Porting evidence and module navigation are less reliable, especially after moving the recorder.
- **Failure scenario:** Follow the links in a Markdown viewer or resolve them from the documented file paths.
- **Root cause:** Relative links were not recalculated after the hierarchy change.
- **Recommended fix:** Correct the relative paths and add a link check to documentation review.
- **Regression risks:** Keep links relative to the C++ module tree rather than to machine-specific build paths.
- **Relevant validation:** Direct path inspection during this review; no documentation link checker was available.

#### LOW-002: `--no-test` build modes still select TTD test targets

- [x] Remediation status: fixed by mapping both focused modes to their library targets in the no-test branch.
- **Source:** `NEW/build.bat:92-100` and the `--no-test` target remapping block at `NEW/build.bat:274-320`.
- **Affected component:** Focused build workflow.
- **Technical evidence:** This was present in the initial review; the no-test remapping now selects `new_ghidra_ttd_recorder` and `new_ghidra_win_ttd_replay` explicitly.
- **Expected behavior:** `--no-test` should build the recorder/replay libraries without test executables or test execution.
- **Actual behavior:** The selected test target remains when tests are disabled, so the focused command can still compile test sources or fail because the target graph differs.
- **Impact:** Slow or misleading dependency-only builds and a broken documented incremental workflow.
- **Failure scenario:** Run `NEW\build.bat ttd_replay --no-test` or `trace_recorder --no-test` and inspect the requested target.
- **Root cause:** New mode selectors were added without updating the existing target remapping table.
- **Recommended fix:** Map the modes to `new_ghidra_win_ttd_replay` and `new_ghidra_ttd_recorder` in the no-test branch and add a script smoke check.
- **Regression risks:** Keep test-enabled mode target names unchanged.
- **Relevant validation:** Static inspection only; no build wrapper was executed.

#### LOW-003: Windows `.RUN` paths are rejected despite case-insensitive path conventions

- [x] Remediation status: fixed with case-insensitive `.run` validation in recorder and replay adapters.
- **Source:** `win_ttd.cppm:109-114`.
- **Affected component:** Replay trace path validation.
- **Technical evidence:** The extension is compared with `trace.extension() != ".run"` using case-sensitive `std::filesystem::path` string comparison.
- **Expected behavior:** Accept `.run` in case-insensitive Windows spelling, or clearly require lower-case names.
- **Actual behavior:** A valid `capture.RUN` is rejected before the native API is called.
- **Impact:** Minor interoperability failure for user-provided traces.
- **Failure scenario:** Pass an existing `capture.RUN` to `open_trace` on Windows.
- **Root cause:** Portable case-sensitive comparison was used for a Windows file-format extension.
- **Recommended fix:** Compare a lower-cased extension on Windows or use a case-insensitive helper and retain explicit format validation.
- **Regression risks:** Do not broaden acceptance to unrelated extensions.
- **Relevant validation:** No test covers extension casing.

## Verified strengths

- [x] Native replay objects use `UniqueReplayEngine` and `UniqueCursor` rather than raw `delete` (`win_ttd.cppm:558-560`).
- [x] Replay and live debugger contracts are separated: `IReplayDebugSession` does not derive from `ILiveDebugSession` (`NEW/core/contracts/debugger.cppm:130-161`), and the replay test has compile-time assertions (`tests/replay_contract_tests.cppm:21-24`).
- [x] The service provides structured unsupported/resource/invalid-argument diagnostics for common missing-dependency and invalid-input paths.
- [x] The recorder uses a mutable command line with `CreateProcessW` and closes the thread handle, rather than relying on a shell command string.
- [x] CMake target names are distinct (`NewGhidra::WinTtdReplay` and `NewGhidra::TtdRecorder`) and both are included in `new_ghidra_services` (`NEW/services/CMakeLists.txt:14-27`).
- [x] Both focused test executables are registered with CTest (`tests/CMakeLists.txt:16` and `recorder/tests/CMakeLists.txt:9`).

## Reviewed areas with no confirmed findings

- No target-name collision remains in the current replay/recorder CMake files.
- The core replay value types keep TTD native types out of `NEW/core/domain` and treat positions as opaque pairs (`NEW/core/domain/replay.cppm:5-17`).
- The normal no-package replay fallback is explicit rather than pretending that native replay is available (`CMakeLists.txt:39-44`).
- No critical-severity defect was confirmed from the inspected source.

## Validation results

- [x] Direct source and line-level inspection completed.
- [x] Microsoft sample/API comparison completed for engine initialization, custom destruction order, cursor thread parameters, watchpoint setup, recorder elevation, and `-children`.
- [x] Read-only `git diff --check` completed without reported whitespace errors.
- [x] CMake configure/build completed with the real Microsoft API package and runtime DLLs.
- [x] Focused recorder/replay CTest and full CTest completed; the required replay mode correctly fails when its trace fixture is absent.
- [x] Formatting completed; tidy remains limited by the repository's MSVC module `.ifc` consumer limitation.
- [x] Native TTD runtime/API behavior exercised against a real recorder-generated `.run`.
- [x] `setup_dependencies.bat` reran the reference NuGet restore and runtime staging successfully.
- [x] The real replay executable ran 13 tests, including the focused multi-thread fixture cases.

## Unresolved questions and residual risks

- The installed 0.9.5 API does not expose a complete portable event queue; replay event polling/sinks therefore return explicit `unsupported` diagnostics rather than pretending to deliver events.
- The current working tree intentionally moves the recorder from the former sibling path `NEW/services/trace_recorder/win_ttd` to `NEW/services/debugger/win_ttd/recorder`; the portable `ITraceRecorder` contract remains source-compatible.
- The real trace fixture is generated in-process by `NEW\build.bat ttd_all` and removed after a successful run; CI can use that one command instead of managing `TTD_TEST_TRACE` manually.

## Final follow-up decision

- [x] HIGH-001 through HIGH-007 remediation is complete.
- [x] MEDIUM-001 through MEDIUM-010 are addressed or explicitly documented as unsupported/residual environment-dependent cases.
- [x] LOW-001 through LOW-003 remediation is complete.
