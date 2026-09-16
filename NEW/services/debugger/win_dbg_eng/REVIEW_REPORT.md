# WinDbgEng Service Review

## Review Metadata

- **Date:** 2026-09-16
- **Reviewer:** Kilo, independent review pass
- **Scope:** [`win_dbg_eng.cppm`](win_dbg_eng.cppm), DbgEng thread affinity, execution/event flow, native lifetime, process/thread/register/memory/stack/module support, breakpoint/watchpoint mapping, and documented limitations.
- **Reviewed state:** Current working-tree implementation; no source or test implementation changes were made during this review.
- **Assumptions:** A debugger service must remain usable under concurrent caller activity, must not leave pending `Task` objects unresolved, and must distinguish a real stop reason from a merely observed callback.

## Review Status

### Post-review implementation update

The follow-up implementation addressed the active-wait and safety findings: running pause now uses the documented cross-thread `SetInterrupt` path, duplicate waits are rejected, pending tasks are resolved during shutdown, partial initialization is cleaned up, thread-event stopping is policy-controlled, and execution waits have a configurable watchdog timeout. Regression coverage was added for pause, duplicate execution, shutdown, timeout, exact breakpoint/watchpoint identity, and memory/register/process behavior.

- [x] Scope confirmed against the required DbgEng backend behavior.
- [x] Engine-thread queue, `WaitForEvent`, callback classes, native RAII, state transitions, and public contract adapters inspected.
- [x] Native process/thread/register/memory/stack/module/breakpoint/watchpoint paths inspected.
- [x] Integration tests and debuggee inspected separately in [`tests/REVIEW_REPORT.md`](tests/REVIEW_REPORT.md).
- [x] No implementation changes made during review.

## critical

No findings.

## high

### HIGH-WDBG-001: `pause()` cannot interrupt a pending continue operation

- **Status:** [x] Remediated and covered by `PauseInterruptsRunningExecution`
- **Source:** [`win_dbg_eng.cppm:224-238`](win_dbg_eng.cppm#L224-L238), [`win_dbg_eng.cppm:438-475`](win_dbg_eng.cppm#L438-L475), and [`win_dbg_eng.cppm:644-659`](win_dbg_eng.cppm#L644-L659).
- **Component:** Engine command queue and asynchronous execution operations.
- **Technical evidence:** `continue_execution()` installs one `pending_` wait and sets `state_` to `running`. A later `pause()` is queued, but `begin_execution()` accepts only `SessionState::stopped`; when the pause command reaches the engine thread it returns an invalid-state error instead of calling `SetExecutionStatus(DEBUG_STATUS_BREAK)` against the active wait.
- **Expected behavior:** A caller must be able to request a break while a continue/step operation is running. The pause task should complete only after `WaitForEvent()` returns with the stopped target.
- **Actual behavior:** The pause task fails while the original continue task remains pending and the target continues running until another stop.
- **Impact:** The mandatory pause/break capability is not functional for the normal asynchronous usage pattern. A UI or controller cannot stop a running target through this service.
- **Reproduction/failure scenario:** Start `auto run = session->continue_execution(ctx);`, immediately call `session->pause(ctx).get()`, and observe an invalid-state result from pause rather than a stop.
- **Root cause:** The queue models only one completion slot and treats pause as another execution operation instead of an interrupt request that can target an existing wait.
- **Recommended fix:** Separate the active wait completion from interrupt commands. Let pause enqueue an interrupt action that sets `DEBUG_STATUS_BREAK` without replacing the active wait, or expose a dedicated engine-thread interrupt path and complete a pause waiter from the same stop event.
- **Regression risks:** Concurrent pause/continue and cancellation need explicit ordering and exactly-once promise completion.
- **Relevant tests/validation:** No test starts a running task and requests pause; the current fixture remains stopped at a breakpoint for every test.

### HIGH-WDBG-002: Concurrent wait operations overwrite the only pending completion

- **Status:** [x] Remediated and covered by `DuplicateExecutionOperationIsRejected`
- **Source:** [`win_dbg_eng.cppm:438-475`](win_dbg_eng.cppm#L438-L475), especially assignment to `pending_` at line 459.
- **Component:** `enqueue_wait()` pending-operation state.
- **Technical evidence:** Every launch/attach/continue/pause/step operation assigns `pending_` without checking whether another wait is already active. The previous promise remains captured by its old lambda but no longer has a path to completion.
- **Expected behavior:** The service should reject a conflicting operation deterministically or maintain a well-defined set of waiters; no returned `Task` may remain unresolved forever.
- **Actual behavior:** A second operation can replace the first pending completion. The first task can wait indefinitely even after the target stops.
- **Impact:** Deadlocks/hangs in clients, leaked operation state, and non-deterministic execution control under ordinary concurrent callers.
- **Reproduction/failure scenario:** Call `continue_execution()` twice before the first task completes, or queue `step_into()` while a continue is waiting. The second command replaces `pending_` and the first task is never fulfilled.
- **Root cause:** The engine has a single pending slot but no admission control or operation identity.
- **Recommended fix:** Reject a second conflicting wait before enqueueing, or maintain a typed operation state with one active execution waiter and separate interrupt/cancellation requests. Add exactly-once completion assertions.
- **Regression risks:** Existing callers may rely on queued commands; they should receive a completed conflict error rather than a hang.
- **Relevant tests/validation:** No concurrent-task, duplicate-command, or operation-ordering test exists.

### HIGH-WDBG-003: Shutdown can abandon queued and pending tasks

- **Status:** [x] Remediated and covered by `SessionShutdownResolvesPendingTask`
- **Source:** [`win_dbg_eng.cppm:182-190`](win_dbg_eng.cppm#L182-L190), [`win_dbg_eng.cppm:531-577`](win_dbg_eng.cppm#L531-L577), and [`win_dbg_eng.cppm:516-528`](win_dbg_eng.cppm#L516-L528).
- **Component:** Session destruction and engine-loop termination.
- **Technical evidence:** Destruction sets `stopping_` and joins the thread, but the engine loop exits without draining `commands_`, resolving `pending_`, or cancelling promises. Only commands submitted after `stopping_` is observed receive a `project_closed` error.
- **Expected behavior:** Every returned `Task` must resolve with cancellation/shutdown error, including tasks already queued or waiting for a stop.
- **Actual behavior:** A caller holding a task when the session is destroyed can block forever on `get()`.
- **Impact:** Application shutdown hangs and resource ownership/lifetime guarantees are violated.
- **Reproduction/failure scenario:** Queue a launch or continue, destroy the session before the engine reaches the command, then wait on the returned task.
- **Root cause:** Shutdown is implemented as thread stop/join rather than a promise-draining state transition.
- **Recommended fix:** Atomically transition to shutting down, reject/drain queued commands, resolve `pending_`, and make engine-loop exit complete all outstanding controls before releasing native interfaces.
- **Regression risks:** Shutdown ordering must remain safe with callback delivery and native DbgEng teardown.
- **Relevant tests/validation:** No task-lifetime/shutdown test exists; fixture teardown ignores outstanding task handles.

### HIGH-WDBG-004: Native initialization failure can leave callbacks/client resources in an unsafe partial state

- **Status:** [x] Remediated through the shared partial-init cleanup path; fault injection remains pending
- **Source:** [`win_dbg_eng.cppm:531-541`](win_dbg_eng.cppm#L531-L541) and [`win_dbg_eng.cppm:717-755`](win_dbg_eng.cppm#L717-L755).
- **Component:** DbgEng initialization failure and cleanup.
- **Technical evidence:** `engine_loop()` returns immediately when `initialize_native()` fails, so `shutdown_native()` is not called. Initialization can fail after `DebugCreate`, interface queries, or callback registration have already succeeded.
- **Expected behavior:** Partial initialization must unregister callbacks and release every acquired interface on the engine thread before the session construction failure is reported.
- **Actual behavior:** Cleanup depends on member destruction after construction throws, while callback registration may still reference member callback objects and COM initialization may need matching cleanup.
- **Impact:** Failure paths can leak COM state, release callbacks in the wrong order, or crash during construction/destruction when DbgEng is unavailable or partially initialized.
- **Reproduction/failure scenario:** Make a later `QueryInterface` or `SetOutputCallbacks` fail after event callbacks have been registered, then construct the session. The early return skips the explicit callback/client teardown path.
- **Root cause:** Normal-loop shutdown is not shared with the initialization-error path.
- **Recommended fix:** Run a `shutdown_native()`/partial-cleanup guard on every initialization failure before publishing the failed readiness result, with callback lifetime kept valid until the client releases references.
- **Regression risks:** Cleanup must tolerate null/partially initialized interfaces and preserve the dedicated-thread rule.
- **Relevant tests/validation:** Only successful initialization is exercised; no fault-injection test covers each initialization stage.

### HIGH-WDBG-005: Process/thread snapshots do not provide the required runtime state and exit information

- **Status:** [ ] Partially remediated; process selection and structured memory transfer are now present, but cached process/thread state remains incomplete
- **Source:** [`win_dbg_eng.cppm:1063-1089`](win_dbg_eng.cppm#L1063-L1089) and [`win_dbg_eng.cppm:1123-1145`](win_dbg_eng.cppm#L1123-L1145).
- **Component:** Process and thread inspection.
- **Technical evidence:** `query_current_process()` always leaves `command_line` empty, never stores an exit code, and derives only a coarse current session state. `threads_native()` returns `ThreadState::unknown` for every thread, leaves names empty, and gives an instruction pointer only nowhere in the result.
- **Expected behavior:** Process/thread APIs should return usable identity, state, exit information, current context, and distinguish running/stopped/blocked/exited threads where the backend permits it.
- **Actual behavior:** Thread enumeration proves only IDs and current flag; process exit events do not update a retained process snapshot.
- **Impact:** Required process/thread state inspection and post-exit diagnosis are incomplete; consumers cannot reliably display or act on thread state.
- **Reproduction/failure scenario:** Stop a process after a thread exits, call `threads()` or `process()`, and observe unknown thread states/empty exit metadata.
- **Root cause:** Snapshot methods were implemented as minimal ID enumeration and do not maintain a target model/cache.
- **Recommended fix:** Maintain process/thread snapshots from callbacks plus explicit refresh, populate state/name/IP where supported, retain exit info, and add process scope.
- **Regression risks:** Callback refresh ordering must avoid stale snapshots around process/thread exit events.
- **Relevant tests/validation:** The current test checks only thread count, current selection, and a non-empty process ID.

### HIGH-WDBG-006: Memory reads silently return partial data and cannot safely handle large requests

- **Status:** [x] Remediated for bounded native transfers and covered by `RejectsMemoryReadsBeyondNativeTransferLimit`
- **Source:** [`win_dbg_eng.cppm:1262-1273`](win_dbg_eng.cppm#L1262-L1273) and [`win_dbg_eng.cppm:1276-1284`](win_dbg_eng.cppm#L1276-L1284).
- **Component:** DbgEng virtual memory adapter.
- **Technical evidence:** `size_t` is cast directly to `ULONG`; a large request can truncate. If `ReadVirtual` returns a failure with nonzero `transferred`, the code returns the shortened byte vector as a successful `Result<Bytes>` with no partial-read marker. Writes similarly cast size and reject partial writes without preserving transfer detail.
- **Expected behavior:** The backend must validate request size, preserve transfer count/known bytes, and distinguish complete success from partial/unmapped memory.
- **Actual behavior:** A caller can receive apparently successful bytes that do not cover the requested range, and oversized requests can target a different native length than requested.
- **Impact:** Memory analysis can use incomplete or incorrectly sized data; remote/guarded-page behavior cannot be diagnosed.
- **Reproduction/failure scenario:** Read across an unmapped page or pass a size larger than `ULONG_MAX`; observe truncation/partial success with no generic diagnostic.
- **Root cause:** The backend maps DbgEng's transfer API directly to `Bytes` instead of a structured memory transfer result.
- **Recommended fix:** Reject sizes beyond native limits, return a result with requested/returned lengths and known mask, and preserve native error details.
- **Regression risks:** Existing exact-page reads should remain unchanged while partial reads become explicit.
- **Relevant tests/validation:** Only an exact eight-byte global read/write is tested.

## medium

### MEDIUM-WDBG-001: Thread creation/exit callbacks force stops unrelated to the requested operation

- **Status:** [ ] Remediation required
- **Source:** [`win_dbg_eng.cppm:1593-1604`](win_dbg_eng.cppm#L1593-L1604) and [`win_dbg_eng.cppm:1614-1623`](win_dbg_eng.cppm#L1614-L1623).
- **Component:** `IDebugEventCallbacks` execution status returns.
- **Technical evidence:** `CreateThread`, `ExitThread`, and `ExitProcess` return `DEBUG_STATUS_BREAK` unconditionally. `on_thread_created()` only records a flag and does not set `last_stop_reason_` to `thread_event`.
- **Expected behavior:** Thread/process notifications should be captured as events without stopping execution unless session policy explicitly requests a stop. When a stop is requested, its stop reason must identify the event.
- **Actual behavior:** Normal continue can stop at every thread creation/exit, and the resulting stop reason may be `unknown`. The fixture must “continue” through loader/thread events before reaching its intended breakpoint.
- **Impact:** Continue semantics are surprising and make breakpoint/step tests ambiguous; event handling is coupled to accidental callback return values.
- **Reproduction/failure scenario:** Launch a target that creates several threads without a code breakpoint and call continue. The service can complete on a thread callback rather than a user-visible stop condition.
- **Root cause:** Callback return values were used as a generic stop mechanism instead of separating event capture from stop policy.
- **Recommended fix:** Return `DEBUG_STATUS_NO_CHANGE` for notifications unless an explicit policy requires a stop, set a precise `thread_event`/`process_exit` reason when stopping, and test event-only continuation.
- **Regression risks:** Initial-stop policy and callback ordering need explicit tests.
- **Relevant tests/validation:** Fixture setup intentionally advances through loader/thread stops but never asserts their reasons.

### MEDIUM-WDBG-002: Register mapping does not provide reliable architecture/register semantics

- **Status:** [ ] Remediation required
- **Source:** [`win_dbg_eng.cppm:1007-1051`](win_dbg_eng.cppm#L1007-L1051) and [`win_dbg_eng.cppm:1188-1230`](win_dbg_eng.cppm#L1188-L1230).
- **Component:** `IDebugRegisters` adapter.
- **Technical evidence:** Bit width is inferred as 32 only for `DEBUG_REGISTER_SUB_REGISTER`, otherwise 64; vector/FPU/system register widths and roles are not modeled. `read_registers_native()` silently skips values that fail to read. `register_bytes()` returns empty for unsupported value types.
- **Expected behavior:** All supported register classes should have correct widths/roles and explicit unavailable/error state, including `RFLAGS` and SIMD/FPU values.
- **Actual behavior:** A successful vector register enumeration can produce an empty value; a failed register read disappears from the result; `RFLAGS` is classified as general-purpose by name heuristic.
- **Impact:** Register context is incomplete and silently lossy, especially for SIMD/FPU and non-x64 targets.
- **Reproduction/failure scenario:** Request all registers on x64 and compare descriptor/value counts; unsupported native types are absent without an error or known mask.
- **Root cause:** Register conversion is a scalar x64 name heuristic rather than a capability-aware architecture mapping.
- **Recommended fix:** Use native register descriptions/types to populate explicit roles, widths, aliases, and unavailable values; do not silently drop failures.
- **Regression risks:** Existing lowercase-name queries should remain case-insensitive while preserving canonical names.
- **Relevant tests/validation:** Tests check only non-empty results plus `rip`/`rsp`; they do not validate `RAX-R15`, `RBP`, `RFLAGS`, or vector registers.

### MEDIUM-WDBG-003: Stack/module queries return only partial symbol and ownership information

- **Status:** [ ] Remediation required
- **Source:** [`win_dbg_eng.cppm:1315-1347`](win_dbg_eng.cppm#L1315-L1347) and [`win_dbg_eng.cppm:1349-1389`](win_dbg_eng.cppm#L1349-L1389).
- **Component:** Stack and module inspection.
- **Technical evidence:** Stack frames resolve only a function name; module and source fields are always empty. Module snapshots use a boolean `parameters.Flags != 0` as `symbols_loaded`, with no actual symbol-load query or process association.
- **Expected behavior:** The contract advertises symbol information when available, and the backend should preserve module/source/frame metadata or explicitly report it unavailable.
- **Actual behavior:** Consumers receive empty module/source fields and a potentially false symbol-loaded flag.
- **Impact:** Stack inspection is useful only for raw addresses/function names; source navigation and reliable symbol state are unavailable.
- **Reproduction/failure scenario:** Inspect a PDB-backed frame and module; source/module fields remain empty despite symbols being present.
- **Root cause:** The adapter stops after `GetNameByOffset` and does not query source/module names or symbol state.
- **Recommended fix:** Resolve module/source/line metadata best-effort, add explicit symbol state, and preserve unavailable-vs-empty distinctions.
- **Regression risks:** Symbol loading can be expensive; keep it out of callbacks and make refresh policy explicit.
- **Relevant tests/validation:** The stack test asserts only one frame and a nonzero instruction address; module test checks only a path substring.

### MEDIUM-WDBG-004: Detach/terminate update state without completing or synchronizing pending execution

- **Status:** [ ] Remediation required
- **Source:** [`win_dbg_eng.cppm:683-714`](win_dbg_eng.cppm#L683-L714).
- **Component:** Lifecycle operations.
- **Technical evidence:** `detach_native()` and `terminate_native()` set `waiting_for_event_` false and update state immediately, but do not resolve an active `pending_` operation or wait for the corresponding process/thread exit event.
- **Expected behavior:** Detach/termination should have an explicit asynchronous completion point and resolve all affected execution tasks exactly once.
- **Actual behavior:** A pending continue can remain unresolved while the session reports `detached`/`exited`; termination state is asserted before event translation completes.
- **Impact:** Lifecycle races can leave callers with stale tasks and inconsistent event/state order.
- **Reproduction/failure scenario:** Start `continue_execution()`, then queue `terminate()` or `detach()` before the next stop; inspect the original task and event queue.
- **Root cause:** Lifecycle operations bypass the `PendingWait` completion model.
- **Recommended fix:** Model detach/terminate as engine-thread operations with explicit completion promises, cancel/resolve active waits, and publish state only after native/event cleanup is complete.
- **Regression risks:** Teardown must still work if the target has already exited or DbgEng reports `S_FALSE`.
- **Relevant tests/validation:** Only detach from a stopped fixture is tested; terminate is used in teardown without asserting its result/state.

### MEDIUM-WDBG-005: Native failures collapse into an unstructured generic resource error

- **Status:** [ ] Remediation recommended
- **Source:** [`win_dbg_eng.cppm:36-77`](win_dbg_eng.cppm#L36-L77).
- **Component:** Backend error translation.
- **Technical evidence:** `native_error()` always creates `DiagnosticCode::resource_unavailable`, and `hresult_error()` embeds the numeric HRESULT only in a message string. Access denied, invalid state, timeout, unsupported operation, and partial transfer are not structured distinctly.
- **Expected behavior:** Generic callers should receive operation/backend/native-code information in a structured, backend-opaque error payload or stable diagnostic category.
- **Actual behavior:** Callers must parse English text to distinguish native failures; remediation and retry policy cannot be reliable.
- **Impact:** Poor diagnostics and unstable error handling for remote/brokered consumers.
- **Reproduction/failure scenario:** Compare an access-denied `CreateProcess` failure with a missing-interface failure; both expose the same resource-unavailable category.
- **Root cause:** Error mapping was centralized but not typed.
- **Recommended fix:** Extend the generic error/result model with operation, backend identifier, native numeric code, and stable category fields while keeping native types private.
- **Regression risks:** Existing message-based diagnostics should remain human-readable during migration.
- **Relevant tests/validation:** No negative-path test asserts error category/native information.

## low

### LOW-WDBG-001: Event and command queues are unbounded

- **Status:** [ ] Remediation recommended
- **Source:** [`win_dbg_eng.cppm:516-528`](win_dbg_eng.cppm#L516-L528) and [`win_dbg_eng.cppm:1815-1823`](win_dbg_eng.cppm#L1815-L1823).
- **Component:** Queue/resource management.
- **Technical evidence:** `commands_`, `events_`, and `sink_events_` grow without a limit or backpressure policy.
- **Expected behavior:** A debugger service should bound retained events/commands or document an explicit loss/backpressure policy.
- **Actual behavior:** A consumer that never polls events or submits commands faster than the engine can process can grow memory without limit.
- **Impact:** Long-running sessions may suffer memory exhaustion.
- **Reproduction/failure scenario:** Continue a target with high-frequency thread/module events while never calling `poll_events()`.
- **Root cause:** The queue was implemented as an unbounded `std::deque`.
- **Recommended fix:** Add configurable bounds, overflow diagnostics, and queue admission errors.
- **Regression risks:** Event ordering and loss policy must be documented for consumers.
- **Relevant tests/validation:** No queue-pressure or event-overflow test exists.

### LOW-WDBG-002: DbgEng output is discarded entirely

- **Status:** [ ] Remediation recommended
- **Source:** [`win_dbg_eng.cppm:1666-1698`](win_dbg_eng.cppm#L1666-L1698).
- **Component:** Native output callback.
- **Technical evidence:** `OutputCallbacks::Output()` always returns `S_OK` without retaining diagnostic text.
- **Expected behavior:** Important backend diagnostics should be available through a generic diagnostic event or structured logging sink.
- **Actual behavior:** DbgEng warnings, symbol-load messages, and useful troubleshooting output disappear.
- **Impact:** Debugging setup failures is harder and native behavior cannot be diagnosed from the service API.
- **Reproduction/failure scenario:** Trigger a symbol-load warning and inspect `poll_events()`; no diagnostic event contains the output.
- **Root cause:** Output was suppressed to prevent native text from leaking into the contract.
- **Recommended fix:** Translate selected output into a generic diagnostic event or configurable backend logger without exposing native types.
- **Regression risks:** Avoid blocking or invoking application code from the output callback.
- **Relevant tests/validation:** No output/diagnostic translation test exists.

## Verified Strengths

- [x] Native DbgEng interfaces are isolated inside the backend module.
- [x] The implementation creates interfaces and performs native calls on one dedicated engine thread.
- [x] RAII is used for native interface ownership, and callback teardown ordering was inspected.
- [x] Execution commands use `SetExecutionStatus()` and complete pending waits only after `WaitForEvent()` returns.
- [x] Generic events do not invoke the sink directly from the native callback; sink dispatch is deferred until after the wait returns.
- [x] The backend explicitly rejects execute-only data watchpoints instead of silently mapping them to another access mode.

## Validation Results

- [x] Backend source, callback signatures, queue state, native mapping, and cleanup paths inspected.
- [x] Existing focused debugger build/test evidence was reviewed; the available Windows run reported passing tests.
- [ ] No fault-injection test exercised partial initialization or native interface failures.
- [ ] No concurrency stress test exercised overlapping commands, pause while running, cancellation, or destruction with pending tasks.
- [ ] No remote or multi-process backend was available for compatibility testing.

## Unresolved Questions and Residual Risks

- [ ] Confirm the exact DbgEng callback return policy desired for thread/process/module events and initial-break behavior.
- [ ] Decide whether DbgEng process-server/remote support is in scope for this backend or only for future siblings.
- [ ] Define a target model/cache policy before adding process/thread/module state events.

## Final Follow-up Decision

- [ ] Fix `HIGH-WDBG-001`, `HIGH-WDBG-002`, `HIGH-WDBG-003`, and `HIGH-WDBG-004` before treating the service as production-safe under concurrent or failing operations.
