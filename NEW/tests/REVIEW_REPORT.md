# Architecture Tests Review Report

## Review Metadata

- [x] **Scope:** exactly `HEAD~2..HEAD` (`778c5d87ad` and `3c123d1fda`), covering root integration/replay tests, service test registration, fixtures, and related project/runtime tests.
- [x] **Date:** 2026-09-16.
- [x] **Reviewer:** Kilo, independent read-only pass.
- [x] **Assumption:** Existing feature parity tests are regression coverage; this review also checks whether new architecture failure modes are exercised.

## Review Status

- [x] Scope confirmation.
- [x] Test source, fixture paths, and CMake registration inspected.
- [x] Cross-service integration assertions inspected.
- [x] Validation status recorded.
- [x] No implementation fixes applied.

## Findings: Critical

### No findings

The service lifetime defect is a production ownership issue with missing test coverage; it is tracked as [`../REVIEW_REPORT.md#critical-001`](../REVIEW_REPORT.md#critical-001).

## Findings: High

### No findings

## Findings: Medium

### MEDIUM-001: Tests cover happy-path composition but not the migration's failure and lifetime contracts

- [ ] **Remediation status:** Open.
- **References:** `integration/end_to_end_tests.cppm:22-50`, `replay/replay_tests.cppm:23-50`, `../runtime/project/tests/project_tests.cppm:104-134`, `../services/service_tests.cppm:23-56`, `../services/analyzers/tests/CMakeLists.txt:1-14`.
- **Affected component:** Architecture regression coverage.
- **Technical evidence:** Tests assert successful PE load, one x86 `ret` decode, successful analysis/decompilation, and in-memory replay. They do not assert default-app service composition, analyzer registration/mutation commits, service destruction with queued work, pre-start/context cancellation, mixed-batch/SQLite failure, event-subscriber failure, malformed FID hashes, alternate architecture, or decompiler fallback status.
- **Expected behavior:** Tests should lock down the new core/service/runtime contracts and failure behavior, not only the x86 happy path and legacy parity algorithms.
- **Actual behavior:** A full functional test pass can coexist with the critical lifetime, commit-atomicity, analyzer-registration, and architecture findings.
- **Impact:** Regressions in shutdown safety, persistence consistency, service substitution, and non-x86 behavior can ship undetected.
- **Failure scenario:** Break one of those paths while preserving x86 fixture output; current tests remain green.
- **Root cause:** New tests were added primarily as composition smoke tests and legacy test registrations were moved without new contract-level fixtures.
- **Recommended fix:** Add deterministic fake-worker/service lifetime tests, cancellation identity tests, fault-injected commit tests, throwing-subscriber tests, analyzer command persistence tests, malformed FID tests, alternate SLA/architecture fixtures, and fallback-status assertions.
- **Regression risks:** Fault-injection tests must remain deterministic and platform-independent.
- **Relevant validation:** `ctest --test-dir NEW/build --output-on-failure` executed during the review and passed 49/49; the missing failure-path coverage remains untested.

## Findings: Low

### No findings

## Verified Strengths

- [x] Root integration and replay targets are registered under `NEW/tests`.
- [x] Service fixtures are referenced through CMake-defined source-root macros rather than machine-specific paths.
- [x] PE, Sleigh, decompiler, Function ID, and analyzer feature tests remain in their migrated service directories.
- [x] Existing tests include successful event-log recovery and compiled-SLA/PE fixture use.

## Reviewed Areas With No Findings

- [x] Fixture relocation paths used by service and root integration tests.
- [x] Focused service test target registration in `services/*/tests`.
- [x] Root integration/replay CMake target declarations.
- [x] Test comments and fixture ownership in the reviewed newly added tests.

## Validation Results

- [x] Test source and CMake registration inspected read-only.
- [x] Existing generated CTest test files inspected.
- [x] `ctest --test-dir NEW/build --output-on-failure`: 49/49 passed.
- [x] `git diff HEAD~2..HEAD --check`: passed.
- [ ] No sanitizer, race, fault-injection, concurrent-run, or fixture runtime validation was performed.

## Unresolved Questions And Residual Risks

- [ ] Define a deterministic harness for queuing work behind a blocked worker before destroying a service.
- [ ] Add a fixture with non-x86 architecture/SLA metadata or explicitly constrain supported architectures.
- [ ] Decide whether SQLite domain-row materialization is part of the test contract.

## Follow-Up Decision

- [ ] Test gap `MEDIUM-001` remains open.
- [ ] No fixes were authorized or applied.
