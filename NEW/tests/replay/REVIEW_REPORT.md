# Test Coverage Review: tests/replay

## Review Metadata

- [x] **Scope:** event-log replay test, SQLite checkpoint assertions, CMake registration, and architecture commits `778c5d87ad` and `3c123d1fda`.
- [x] **Date:** 2026-09-16.
- [x] **Reviewer:** Kilo, independent test-coverage review.
- [x] **Assumptions:** Replay must prove durable atomicity and idempotence, not only successful in-memory reconstruction.

## Inventory

- [x] Existing test: `replay_tests.cppm`.
- [x] Existing target: `architecture_replay_tests`.
- [x] Existing assertions: one listing event, SQLite checkpoint, fresh in-memory projection rebuild.

## Findings: Critical

### No findings

## Findings: High

### TESTS-REPLAY-HIGH-001: Replay suite does not test atomic or corrupt projection recovery

- [ ] **Remediation status:** Open.
- **Source references:** `replay_tests.cppm:24-50`, `CMakeLists.txt`.
- **Affected component:** Append-only event store, projection coordinator, SQLite projection.
- **Technical evidence:** The test commits one valid event and rebuilds from a valid stream. It does not submit a mixed-validity batch, inject a second-event SQLite failure, corrupt the projection database, or verify whether partial frames are recoverable without advancing checkpoints.
- **Expected behavior:** Failed/mixed commits leave a defined authoritative state and replay reconstructs it exactly.
- **Actual behavior:** Only successful single-event replay is asserted.
- **Impact:** The primary persistence guarantees can regress while replay tests remain green.
- **Failure scenario:** Append one valid and one invalid draft, or fail SQLite persistence after the first event.
- **Root cause:** Replay coverage was designed around the happy path.
- **Recommended fix:** Add fault-injected atomicity, corruption, and checkpoint divergence tests.
- **Regression risks:** Fault injection must not depend on filesystem timing.
- **Relevant validation:** Current CTest passes 49/49.

## Findings: Medium

### TESTS-REPLAY-MEDIUM-001: Idempotence, subscriber failure, and multi-project isolation are missing

- [ ] **Remediation status:** Open.
- **Source references:** `replay_tests.cppm:24-50`.
- **Missing scenarios:** Duplicate idempotency key; duplicate event replay; throwing subscriber after commit; two projects sharing one physical log; corrupt checksum versus incomplete tail; SQLite domain-row materialization.
- **Impact:** Retry and cross-project revision behavior are not locked down.
- **Recommended fix:** Add deterministic replay/property tests for duplicate, corruption, subscriber, and multi-project cases.

## Findings: Low

### No findings

## Verified Strengths

- [x] Uses a fresh projection instance for rebuild.
- [x] Checks persisted checkpoint and listing value.
- [x] Is registered with CTest.

## Validation Results

- [x] `ctest --test-dir build --output-on-failure`: 49/49 passed.
- [x] Event store/projection source and CMake registration inspected.
- [ ] No fault-injection or multi-process replay test exists.

## Follow-Up Decision

- [ ] Add TESTS-REPLAY-HIGH-001 and TESTS-REPLAY-MEDIUM-001 before claiming persistence/replay coverage complete.
