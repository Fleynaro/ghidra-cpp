# Debugger Integration Test Review

## Review Metadata

- **Date:** 2026-09-16
- **Reviewer:** Kilo, independent review pass
- **Scope:** [`debugger_contract_tests.cppm`](debugger_contract_tests.cppm), [`CMakeLists.txt`](CMakeLists.txt), [`data/debugger_debuggee.cpp`](data/debugger_debuggee.cpp), and [`data/build.bat`](data/build.bat).
- **Reviewed state:** Current working-tree implementation; no test or production implementation changes were made.
- **Assumptions:** These are intended to be contract-level integration tests, not merely smoke tests. A passing CTest result must demonstrate that DbgEng launched and controlled the real debuggee.

## Review Status

### Post-review implementation update

Windows fixture setup failures now fail rather than silently skip, the CTest registration has a hard timeout, detached debuggee stdout is closed deterministically, and regression tests assert running pause, duplicate-operation rejection, shutdown completion, timeout, process selection, register aliases, memory transfer bounds, exact code-breakpoint identity, and exact watchpoint/event identity. Stepping remains intentionally listed below because its semantic oracle is still incomplete.

- [x] Test fixture and setup/teardown inspected.
- [x] Each test case and assertion inspected.
- [x] Debuggee synchronization, symbols, threads, heap/global data, loops, and exception path inspected.
- [x] CMake/debuggee build integration inspected.
- [x] No implementation changes made during review.

## critical

No findings.

## high

### HIGH-TEST-001: Windows integration tests can all pass by skipping when the backend/debuggee is unavailable

- **Status:** [x] Remediated for Windows setup; non-Windows remains an explicit unsupported-platform skip
- **Source:** [`debugger_contract_tests.cppm:21-53`](debugger_contract_tests.cppm#L21-L53), [`CMakeLists.txt:43-49`](CMakeLists.txt#L43-L49).
- **Component:** Fixture setup and CTest pass/fail policy.
- **Technical evidence:** Every setup failure calls `GTEST_SKIP()`, including backend creation, session creation, launch, loader continuation, symbol resolution, breakpoint creation, and worker stop. The CMake test has no Windows-only failure policy or required-test guard.
- **Expected behavior:** On a Windows integration build, missing DbgEng, missing debuggee, failed launch, or failed symbol setup should fail the integration test. Non-Windows builds may skip explicitly because the backend is unsupported, but a Windows run must not report green without exercising the backend.
- **Actual behavior:** A broken DbgEng installation, missing executable, failed PDB/symbol load, or backend regression can produce an all-skipped test executable that CTest reports as passed.
- **Impact:** CI can falsely claim debugger coverage while testing nothing.
- **Reproduction/failure scenario:** Remove `debugger_debuggee.exe` or make `create_win_dbg_eng()` return an error on Windows; all fixture tests skip and CTest can still pass.
- **Root cause:** `GTEST_SKIP()` is used as universal error handling instead of separating platform availability from integration setup failures.
- **Recommended fix:** Keep an explicit non-Windows skip test, but make Windows setup failures `ASSERT_TRUE`/`FAIL`; verify the fixture executable exists and require at least one executed test in CI.
- **Regression risks:** Developer machines without DbgEng need a clear opt-in/skip mechanism rather than silently green integration results.
- **Relevant tests/validation:** Existing focused CTest evidence is green, but the report cannot establish from the assertions alone that every environment would execute the backend.

### HIGH-TEST-002: The watchpoint test passes without proving a memory breakpoint fired

- **Status:** [x] Remediated; the test disables the unrelated entry breakpoint and asserts watchpoint ID/address/event
- **Source:** [`debugger_contract_tests.cppm:177-190`](debugger_contract_tests.cppm#L177-L190).
- **Component:** Data breakpoint/watchpoint integration coverage.
- **Technical evidence:** The test installs a watchpoint, leaves the fixture's code breakpoint enabled, calls `continue_execution()` twice, and accepts either `watchpoint` **or** `breakpoint` at lines 188-189. It never checks `StopReason.watchpoint`, the watchpoint ID, the access mode, the hit address, or a `WatchpointHitEvent`.
- **Expected behavior:** The test must demonstrate that a deterministic write to the watched address caused the stop, independently of code breakpoints.
- **Actual behavior:** The test passes if execution stops at the existing `debugger_test_entry` code breakpoint or any unrelated event.
- **Impact:** The mandatory memory-breakpoint/watchpoint capability is effectively untested.
- **Reproduction/failure scenario:** Remove or break the DbgEng data-breakpoint mapping while retaining the entry breakpoint; the test can still satisfy the OR assertion.
- **Root cause:** The test uses broad stop-kind acceptance to avoid synchronization ambiguity instead of disabling unrelated breakpoints and asserting the watchpoint identity/event.
- **Recommended fix:** Disable/remove the entry breakpoint, arrange a single deterministic write after the watched stop, then assert `StopReasonKind::watchpoint`, matching `WatchpointId`, address, access, and translated event payload.
- **Regression risks:** Hardware watchpoint size/alignment limitations may require a backend capability skip with an explicit reason, not acceptance of a code breakpoint.
- **Relevant tests/validation:** Current test execution passing does not prove data-breakpoint correctness.

### HIGH-TEST-003: Step-into, step-over, and step-out tests assert only a generic stop label

- **Status:** [ ] Partially remediated; stop completion is covered, but callee/caller address or frame assertions are still needed
- **Source:** [`debugger_contract_tests.cppm:151-175`](debugger_contract_tests.cppm#L151-L175).
- **Component:** Execution semantics tests.
- **Technical evidence:** Step-into and step-over only assert `StopReasonKind::step_complete`; step-out performs one step-into and then asserts the same label. No instruction address, callee/caller symbol, return address, or before/after location is checked.
- **Expected behavior:** Step into must enter the intended callee, step over must advance past a call without entering it, and step out must return to the caller frame.
- **Actual behavior:** Any stop reported as `step_complete`, including a backend that stops at an unrelated instruction, satisfies the tests.
- **Impact:** Core stepping functionality can be broken while all three tests remain green.
- **Root cause:** The fixture starts at a function breakpoint but does not establish a caller/callee address oracle or compare pre/post stack frames.
- **Recommended fix:** Resolve caller/callee symbols, record pre-step IP/frame, assert step-into IP lies in callee, assert step-over IP is after the call and not in callee, and assert step-out returns to the caller frame/address.
- **Regression risks:** `/Od` and PDB builds need stable symbol/address assertions; use exported noinline functions and explicit call markers.
- **Relevant tests/validation:** No test checks stepping against `compute_value`, `worker_nested`, or `debugger_test_entry` addresses.

### HIGH-TEST-004: Several mandatory debugger capabilities have no meaningful integration test

- **Status:** [ ] Partially remediated; process/register/memory/stack/timeout coverage expanded, but attach, heap, cancellation, event sink, and full breakpoint lifecycle remain
- **Source:** Test file overall, especially [`debugger_contract_tests.cppm:68-211`](debugger_contract_tests.cppm#L68-L211).
- **Component:** Coverage of the requested debugger contract.
- **Technical evidence:** There is no attach test, no pause-while-running test, no explicit termination-state/exit-code test, no memory-region test, no heap allocation/pointer test, no register context comparison across selected threads, no `RAX-R15`/`RBP`/`RFLAGS`/SIMD verification, no breakpoint disable/remove test, no watchpoint enable/remove test, no cancellation/concurrency test, and no event-sink/order test.
- **Expected behavior:** The suite should cover the required lifecycle, execution, process/thread context, register, memory/heap, stack, module, code-breakpoint, watchpoint, exception, event, and cleanup semantics through the generic interface.
- **Actual behavior:** The current suite is a successful local smoke path around one launch and one implicit current process.
- **Impact:** Important regressions in attach, pause, context isolation, memory maps, heap inspection, breakpoint lifecycle, event delivery, and cleanup can ship undetected.
- **Root cause:** Tests were added for happy-path values but not for operation boundaries, negative paths, or semantic oracles.
- **Recommended fix:** Add isolated fixtures/tests for every required capability, including a separate attach-spawn helper, a running worker pause scenario, heap pointer dereference, full x64 register set, memory-region permissions, breakpoint/watchpoint lifecycle, event sequencing, cancellation, and termination/detach state.
- **Regression risks:** Some DbgEng features are capability-dependent; skip only the specific unsupported test with a verified capability result, never the whole fixture.
- **Relevant tests/validation:** Existing CTest passing is insufficient evidence for the missing cases.

## medium

### MEDIUM-TEST-001: Fixture setup destroys the distinction between initial stop and target-ready stop

- **Status:** [ ] Remediation required
- **Source:** [`debugger_contract_tests.cppm:30-53`](debugger_contract_tests.cppm#L30-L53).
- **Component:** Launch/initial-stop determinism.
- **Technical evidence:** Setup launches, immediately continues once, resolves a symbol, installs the entry breakpoint, continues again, and leaves every test stopped at `debugger_test_entry`. The test named `LaunchStopsAndReportsProcess` therefore does not inspect the first launch stop as returned by `launch()`.
- **Expected behavior:** Launch should have a dedicated test for initial stop/process-created state, while a separate helper should advance to the worker barrier for tests that need ready threads.
- **Actual behavior:** Initial event reason, initial module state, and launch transition are hidden by fixture warmup; failures are skipped instead of diagnosed.
- **Impact:** Regressions in initial-break/loader/event ordering can pass unnoticed.
- **Root cause:** One fixture setup path is reused for all semantic stages.
- **Recommended fix:** Split fixtures into `LaunchedStoppedFixture` and `WorkerReadyFixture`; assert each transition and stop reason explicitly.
- **Regression risks:** The worker-ready helper must retain deterministic synchronization without relying on incidental thread callback stops.
- **Relevant tests/validation:** Launch test asserts only final stopped state and that at least one broad event kind exists.

### MEDIUM-TEST-002: Thread switching is not a context-isolation test

- **Status:** [ ] Remediation required
- **Source:** [`debugger_contract_tests.cppm:80-96`](debugger_contract_tests.cppm#L80-L96).
- **Component:** Thread selection/register context coverage.
- **Technical evidence:** The test selects another thread and compares only the returned `ThreadId`. It does not read IP/registers/stack before and after selection or prove that the selected thread's context differs and remains independent.
- **Expected behavior:** Selecting a thread should change the context queried by registers, IP, and stack without corrupting the other thread's context.
- **Actual behavior:** A backend that ignores `select_thread()` but returns the requested ID could pass.
- **Impact:** Required thread-context inspection is not verified.
- **Root cause:** The test treats identity selection as sufficient proof of context switching.
- **Recommended fix:** Capture register/IP/stack snapshots for two workers, select each, assert the selected context is returned, and restore the original thread.
- **Regression risks:** Worker code should publish distinguishable per-thread markers rather than relying on incidental register differences.
- **Relevant tests/validation:** No register or stack query follows `select_thread()`.

### MEDIUM-TEST-003: Memory and stack tests do not exercise heap or multiple frames

- **Status:** [ ] Remediation required
- **Source:** [`debugger_contract_tests.cppm:114-148`](debugger_contract_tests.cppm#L114-L148), debuggee [`data/debugger_debuggee.cpp`](data/debugger_debuggee.cpp).
- **Component:** Memory/stack coverage.
- **Technical evidence:** Memory test reads/writes only exported `g_debug_value`; stack test asserts `size() >= 1`, despite the debuggee containing nested calls and heap storage. No pointer is read from `g_heap_value`, no heap allocation is dereferenced, and no multiple-frame assertion exists.
- **Expected behavior:** Tests should prove virtual-memory access works for global and heap-backed addresses and that stack walking preserves nested frames.
- **Actual behavior:** A backend that supports one global read and a single frame but fails heap reads or unwinding can pass.
- **Impact:** Heap and meaningful stack inspection remain unverified.
- **Root cause:** The debuggee's heap/nested-call features are not connected to test oracles.
- **Recommended fix:** Export/resolve the heap pointer, read it through the generic API, assert nested frame symbols/levels, and verify stack behavior for another selected worker thread.
- **Regression risks:** Heap address must be captured while stopped and synchronized through the existing barrier.
- **Relevant tests/validation:** Current tests use only a fixed global symbol and first-frame address.

### MEDIUM-TEST-004: Module assertions do not verify base, size, symbol state, or event lifecycle

- **Status:** [ ] Remediation recommended; detached-process cleanup is fixed, but generic teardown still discards terminate errors
- **Source:** [`debugger_contract_tests.cppm:114-126`](debugger_contract_tests.cppm#L114-L126).
- **Component:** Module enumeration.
- **Technical evidence:** The test checks that one module path contains `debugger_debuggee` and that the module list is non-empty. It does not assert nonzero base/size, symbol state, load/unload events, or stable module identity.
- **Expected behavior:** Module tests should validate the fields the contract promises and event translation for load/unload where the fixture can exercise it.
- **Actual behavior:** An incorrect base/size or symbol flag can pass.
- **Impact:** Module mapping regressions are not detected.
- **Root cause:** Test assertion is only a name-presence smoke check.
- **Recommended fix:** Assert the fixture module's base/size/path, symbol behavior, and translated module event payload; use a capability-specific unload scenario if possible.
- **Regression risks:** System DLL symbol states may vary; assert only the fixture module's deterministic fields.
- **Relevant tests/validation:** Existing test passes without inspecting the returned base/size values.

## low

### LOW-TEST-001: Teardown ignores cleanup failures

- **Status:** [ ] Remediation recommended
- **Source:** [`debugger_contract_tests.cppm:56-60`](debugger_contract_tests.cppm#L56-L60).
- **Component:** Fixture cleanup.
- **Technical evidence:** `terminate()` is called through `static_cast<void>` and its error is discarded.
- **Expected behavior:** Integration tests should report cleanup failures and distinguish a clean exit from forced cleanup.
- **Actual behavior:** Termination regressions can be hidden after a passing test body.
- **Impact:** Resource/lifecycle defects may not fail CI.
- **Root cause:** Teardown was made best-effort without recording diagnostics.
- **Recommended fix:** Record cleanup errors with `ADD_FAILURE()` or use a fixture cleanup assertion while avoiding masking the primary failure.
- **Regression risks:** Tests that intentionally detach should continue to skip termination and verify the detached process cleanup separately.
- **Relevant tests/validation:** No teardown result is asserted.

## Verified Strengths

- [x] Tests import the generic contract and do not include `dbgeng.h` or use native DbgEng constants.
- [x] The debuggee is a real symbolized MSVC executable with multiple functions, nested calls, heap/global data, loops, three worker threads, synchronization, and a controlled exception.
- [x] The fixture uses explicit worker synchronization rather than arbitrary long sleeps.
- [x] Tests are split into logically focused cases rather than one monolithic test.
- [x] The suite exercises real launch, memory read/write, module/stack queries, code breakpoints, stepping, watchpoint setup, exception translation, thread selection, and detach paths.

## Validation Results

- [x] Test source, fixture build script, debuggee source, and CMake registration inspected.
- [x] Existing focused debugger test evidence was reviewed; the available run reported passing tests.
- [x] Full CTest validation: **50/50 passed**, including `win_dbg_eng_tests` in 4.34 seconds after detached-process cleanup.
- [ ] No test was found for attach, pause while running, heap memory, memory regions, register writes, event sink ordering, cancellation, or concurrent command safety.

## Unresolved Questions and Residual Risks

- [ ] Decide whether unsupported DbgEng capabilities should be reported as per-test capability skips or hard failures for this Windows backend.
- [ ] Add a process-spawn helper for attach tests that does not call DbgEng directly.
- [ ] Establish stable symbol/address or call-marker oracles for stepping tests.

## Final Follow-up Decision

- [ ] Fix `HIGH-TEST-001`, `HIGH-TEST-002`, `HIGH-TEST-003`, and `HIGH-TEST-004` before using the suite as acceptance evidence for a production debugger.
