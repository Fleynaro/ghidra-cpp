# C++ Debugger Binding Review

## Review Metadata

- **Date:** 2026-09-16
- **Reviewer:** Kilo, independent review pass
- **Scope:** [`debugger.cppm`](debugger.cppm), specifically whether the binding remains an adapter and exposes the generic debugger functionality promised by the contract.
- **Reviewed state:** Current working-tree implementation; no source changes were made.
- **Assumptions:** The binding should not duplicate backend logic or import DbgEng, but should expose the usable generic contract rather than only a launch/memory smoke subset.

## Review Status

### Post-review implementation update

The C++ facade now wraps process selection, thread/context queries, registers, memory regions, stack/modules/symbols, breakpoint/watchpoint lifecycle, detach/terminate, and event sinks in addition to the original execution and memory methods.

- [x] Binding source and imports inspected.
- [x] Native/backend dependency boundary checked.
- [x] Public wrapper coverage compared with [`../../../core/contracts/debugger.cppm`](../../../core/contracts/debugger.cppm).
- [x] No implementation changes made during review.

## critical

No findings.

## high

### HIGH-BINDING-001: The C++ binding exposes only a small subset of the debugger contract

- **Status:** [x] Remediated; the facade now covers the generic session surface without importing a concrete backend
- **Source:** [`debugger.cppm:21-124`](debugger.cppm#L21-L124), compared with [`../../core/contracts/debugger.cppm:57-138`](../../core/contracts/debugger.cppm#L57-L138).
- **Component:** `recode::bindings::cpp::DebugSession` facade.
- **Technical evidence:** The wrapper exposes launch/attach/continue/pause/step operations, process, threads, raw memory, and event polling. It does not expose detach, terminate, thread selection/current thread, registers/IP, memory regions, stack, modules, symbol resolution, code breakpoints, watchpoints, enable/remove operations, or event sink registration.
- **Expected behavior:** The binding should make the generic debugger service usable from C++ without forcing callers to bypass it and hold the raw contract interface.
- **Actual behavior:** Most core debugger functionality is inaccessible through the advertised C++ binding, including code/data breakpoints, register context, stack, modules, and cleanup.
- **Impact:** The binding does not satisfy the requested architecture for a complete generic debugger API and encourages backend-specific escape hatches.
- **Reproduction/failure scenario:** A C++ binding consumer cannot install a breakpoint, inspect registers/stack, detach, or terminate using this module despite those operations existing in `IDebugSession`.
- **Root cause:** The binding was implemented as a minimal facade rather than a complete adapter over the existing interface.
- **Recommended fix:** Add one-to-one wrappers for the remaining generic methods, preserving `Task<Result<T>>` and value types; do not add DbgEng logic.
- **Regression risks:** Keep return types/value ownership consistent and avoid exposing raw backend pointers.
- **Relevant tests/validation:** The binding has no dedicated tests for any debugger method.

## medium

### MEDIUM-BINDING-001: Event subscription is missing even though the contract exposes it

- **Status:** [ ] Remediation required
- **Source:** [`debugger.cppm:91-94`](debugger.cppm#L91-L94) and contract [`../../../core/contracts/debugger.cppm:134-138`](../../../core/contracts/debugger.cppm#L134-L138).
- **Component:** Event delivery adapter.
- **Technical evidence:** `DebugSession` exposes `poll_events()` but not `set_event_sink()`. Consumers cannot use the callback delivery path through the binding.
- **Expected behavior:** The binding should expose the same documented event delivery options as the generic contract or intentionally document polling-only behavior.
- **Actual behavior:** The available binding surface is narrower without stating the omission.
- **Impact:** Event-driven clients must poll or bypass the binding; callback threading semantics cannot be tested through this layer.
- **Root cause:** Only the simplest value-returning methods were wrapped.
- **Recommended fix:** Add event sink/subscription wrappers with explicit lifetime/threading documentation, or remove the sink from the contract until a safe binding design exists.
- **Regression risks:** Callback lifetime and exception containment must be handled at the binding boundary.
- **Relevant tests/validation:** No binding event test exists.

## low

### LOW-BINDING-001: Binding construction does not state backend lifetime requirements

- **Status:** [ ] Remediation recommended
- **Source:** [`debugger.cppm:105-123`](debugger.cppm#L105-L123).
- **Component:** `Debugger`/`DebugSession` ownership model.
- **Technical evidence:** The binding stores `shared_ptr` values, but documentation does not state whether a backend must outlive returned sessions or how outstanding tasks behave when either wrapper is destroyed.
- **Expected behavior:** An adapter should document task/session lifetime and shutdown behavior inherited from the contract/backend.
- **Actual behavior:** Users can infer ownership but not the pending-task guarantees.
- **Impact:** Minor misuse risk around asynchronous task lifetime and session destruction.
- **Root cause:** The facade delegates ownership without documenting lifecycle semantics.
- **Recommended fix:** Document that session ownership keeps the backend session alive and specify behavior for tasks during wrapper destruction.
- **Regression risks:** Documentation must match the backend's eventual shutdown guarantees.
- **Relevant tests/validation:** No binding lifetime test exists.

## Verified Strengths

- [x] The binding imports only generic core contracts/domain types and does not import DbgEng or a concrete backend.
- [x] It delegates operations rather than duplicating debugger logic.
- [x] Asynchronous methods preserve the repository's `Task<Result<T>>` type.
- [x] Null backend creation is reported as a generic resource error.

## Validation Results

- [x] Binding source and contract comparison completed.
- [x] Existing binding build evidence was reviewed.
- [ ] No dedicated binding tests exist.

## Unresolved Questions and Residual Risks

- [ ] Decide whether the C++ binding is intended to be a complete facade or only an intentionally minimal starter adapter.
- [ ] Define callback exception/threading behavior before exposing `set_event_sink()`.

## Final Follow-up Decision

- [ ] Fix `HIGH-BINDING-001` before treating the C++ binding as complete.
