# Test Coverage Review: tests/integration

## Review Metadata

- [x] **Scope:** root end-to-end integration test, CMake registration, analyzer executable fixture, and architecture commits `778c5d87ad` and `3c123d1fda`.
- [x] **Date:** 2026-09-16.
- [x] **Reviewer:** Kilo, independent test-coverage review.
- [x] **Assumptions:** This suite is the primary cross-service acceptance test; focused unit tests are reviewed in sibling reports.

## Inventory

- [x] Existing test: `end_to_end_tests.cppm`.
- [x] Existing fixture: `services/analyzers/tests/data/test_analyzers_integration.exe`.
- [x] CMake target: `architecture_end_to_end_tests`.

## Findings: Critical

### No findings

## Findings: High

### TESTS-INTEGRATION-HIGH-001: End-to-end test does not prove the default application uses the new runtime

- [ ] **Remediation status:** Open.
- **Source references:** `end_to_end_tests.cppm:22-50`, `CMakeLists.txt`.
- **Affected component:** Application composition and service runtime.
- **Technical evidence:** The test opens `ProjectFacade` directly; it never invokes `new_ghidra_app`, so the legacy `AnalysisContext`/`AutoAnalysisManager` default path can remain broken or bypass the architecture while this test passes.
- **Expected behavior:** The acceptance suite verifies both the facade pipeline and the shipped default executable composition.
- **Actual behavior:** Only the parallel facade pipeline is exercised.
- **Impact:** The primary user entry point can remain on legacy architecture unnoticed.
- **Failure scenario:** Regress `src/main.cpp` back to legacy analysis; this test remains green.
- **Root cause:** The executable smoke test is a separate CTest target and is not connected to the realistic PE/SLA fixture.
- **Recommended fix:** Add `DefaultApplicationUsesRuntimeFacade` invoking the real executable with the analyzer fixture and assert service/persistence output.
- **Regression risks:** Process invocation and fixture paths must remain deterministic on CI.
- **Relevant validation:** Current CTest passes 49/49.

### TESTS-INTEGRATION-HIGH-002: Analyzer mutation persistence is not verified

- [ ] **Remediation status:** Open.
- **Source references:** `end_to_end_tests.cppm:22-50`, `CMakeLists.txt`.
- **Affected component:** Runtime analysis scheduler and commit lane.
- **Technical evidence:** The test checks that `analyze()` returns but does not assert the complete analyzer registry, returned mutation commands, durable domain events, or SQLite projection rows.
- **Expected behavior:** A real analysis run proves feature analyzer results are committed and replayable.
- **Actual behavior:** Lifecycle success is sufficient for the test.
- **Impact:** Analysis can report success while feature state is absent.
- **Failure scenario:** Drop scheduler commands or unregister analyzers; this test still passes.
- **Root cause:** The fixture assertion stops at function/decompilation success.
- **Recommended fix:** Add a command-producing fake analyzer and assert event log, projection, SQLite rows, and reopen state.
- **Regression risks:** Test must distinguish fixture-created state from analyzer-created state.
- **Relevant validation:** No mutation-persistence assertion exists.

## Findings: Medium

### TESTS-INTEGRATION-MEDIUM-001: Alternate and failure pipelines are absent

- [ ] **Remediation status:** Open.
- **Source references:** `end_to_end_tests.cppm:22-50`.
- **Affected component:** Project lifecycle and service contracts.
- **Missing scenarios:** Missing SLA transitions to `failed`; invalid entry-point cleanup; non-x86/unsupported architecture; selected non-default SLA; native decompiler fallback status; actual artifact identity; queued decompile cancellation/close.
- **Impact:** Only x86-64 happy-path behavior is protected.
- **Recommended fix:** Add deterministic fixture/config variants and assert diagnostics, lifecycle, status, and persistence semantics.

## Findings: Low

### No findings

## Verified Strengths

- [x] Uses the real analyzer executable fixture.
- [x] Exercises loading, scheduler-backed analysis, function query, command execution, and decompilation.
- [x] Is registered with CTest.

## Validation Results

- [x] `ctest --test-dir NEW/build --output-on-failure`: 49/49 passed.
- [x] Source/CMake/fixture ownership inspected.
- [ ] No alternate-architecture, fault-injection, sanitizer, or process-level default-app test exists.

## Follow-Up Decision

- [ ] Add TESTS-INTEGRATION-HIGH-001/HIGH-002 and TESTS-INTEGRATION-MEDIUM-001 before treating end-to-end coverage as complete.
