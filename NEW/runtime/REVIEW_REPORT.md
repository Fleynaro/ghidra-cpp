# Runtime Module Review Report

## Review Metadata

- [x] **Scope:** exactly `HEAD~2..HEAD` (`778c5d87ad` and `3c123d1fda`), covering workers, persistence, projections, analysis scheduling, project lifecycle, and runtime composition.
- [x] **Date:** 2026-09-16.
- [x] **Reviewer:** Kilo, independent read-only pass.
- [x] **Assumption:** runtime owns task lifetime, commit ordering, projection durability, and project shutdown.

## Review Status

- [x] Scope confirmation.
- [x] Runtime source, CMake, and test inspection.
- [x] Service ownership and shutdown inspection.
- [x] Validation status recorded.
- [x] Audit baseline recorded; projection payload fixes were applied in the authorized follow-up pass.

## Findings: Critical

### CRITICAL-001: Queued service tasks can call destroyed service objects

- [ ] **Remediation status:** Open.
- **References:** `project/project_manager.cppm:38-50`, `project/runtime_core.cppm:52-59`, with raw task captures in [`../services/REVIEW_REPORT.md`](../services/REVIEW_REPORT.md#critical-001).
- **Affected component:** Runtime shutdown and worker-pool ownership.
- **Technical evidence:** Project close releases session ownership before worker shutdown, while queued service lambdas retain raw service pointers and the pool continues draining work.
- **Expected behavior:** Shutdown must cancel/await work or retain service state until task completion.
- **Actual behavior:** A queued task can execute after its service has been destroyed.
- **Impact:** Use-after-free and process crashes during close/shutdown.
- **Failure scenario:** Queue decompilation with a busy worker, close the project, then let the worker drain.
- **Root cause:** No task/service lease or shutdown barrier.
- **Recommended fix:** Add shared service state and a defined cancellation/drain barrier.
- **Regression risks:** Task completion status and close latency.
- **Relevant validation:** No race or destruction-order test was run or found.

## Findings: High

### HIGH-001: Event commits are not atomic across log, memory projection, and SQLite

- [ ] **Remediation status:** Open.
- **References:** `event_store/append_only_log.cppm:38-79`, `projections/projection_coordinator.cppm:24-48`, `storage/sqlite_projection_store.cppm:47-62`.
- **Affected component:** Durable project commit path.
- **Technical evidence:** Log frames are written during draft iteration; projection and SQLite are applied afterward with no compensating transaction.
- **Expected behavior:** Failed commits leave all stores at one revision or recover through a durable transaction protocol.
- **Actual behavior:** A later invalid draft or SQLite failure can leave partial history and divergent projections.
- **Impact:** Replay, revision checks, and in-process queries can disagree.
- **Failure scenario:** Submit a mixed-project batch or inject a second-event SQLite failure.
- **Root cause:** Separate append/apply/persist stages lack atomic coordination.
- **Recommended fix:** Validate the full batch first and implement transactional/compensating persistence.
- **Regression risks:** Retry and idempotency behavior.
- **Relevant validation:** Only successful replay/commit paths are covered.

### HIGH-002: Runtime analysis does not commit scheduler mutation commands

- [ ] **Remediation status:** Open.
- **References:** `analysis/analysis_scheduler.cppm:31-52`, `project/project_session.cppm:190-200`.
- **Affected component:** Analysis scheduler/project commit lane.
- **Technical evidence:** Scheduler returns `report.commands`, but `ProjectSession::analyze` commits only started/completed lifecycle events.
- **Expected behavior:** Analyzer mutation proposals become durable domain events before analysis completion.
- **Actual behavior:** Any command-producing analyzer result is dropped.
- **Impact:** Analysis can report success while no analyzer state changes persist.
- **Failure scenario:** Register an analyzer returning one `MutationCommand` and call `ProjectFacade::analyze`.
- **Root cause:** No scheduler-to-coordinator mutation bridge.
- **Recommended fix:** Validate and translate commands into event drafts and commit them in the project lane.
- **Regression risks:** Revision ordering and analyzer retry semantics.
- **Relevant validation:** Existing entry analyzer returns no commands, so this path is untested.

### HIGH-003: SQLite projection tables are declared but never materialized

- [ ] **Remediation status:** Open.
- **References:** `storage/sqlite_projection_store.cppm:27-36`, `storage/sqlite_projection_store.cppm:47-61`, `projections/software_model_projection.cppm:88-154`.
- **Affected component:** Durable runtime projection.
- **Technical evidence:** SQLite creates domain tables but `persist()` records only event IDs/checkpoints; query state remains in-memory.
- **Expected behavior:** Domain rows are persisted transactionally or the store is explicitly a checkpoint-only implementation.
- **Actual behavior:** SQLite domain tables remain empty after successful commits.
- **Impact:** Durable query projection is unavailable and checkpoint state overstates materialization.
- **Failure scenario:** Inspect `projection.sqlite` after loading instructions/functions.
- **Root cause:** Event-to-row SQL application is missing.
- **Recommended fix:** Materialize domain rows in the same transaction or remove/document the tables.
- **Regression risks:** Replacement/deletion and migration semantics.
- **Relevant validation:** Replay tests do not query SQLite domain rows.

## Findings: Medium

### MEDIUM-001: Operation-context cancellation is disconnected from returned tasks

- [ ] **Remediation status:** Open.
- **References:** `project/project_session.cppm:204-218`, `workers/worker_pool.cppm:45-85`, service references in [`../services/REVIEW_REPORT.md`](../services/REVIEW_REPORT.md#medium-002).
- **Affected component:** Worker task cancellation.
- **Technical evidence:** `WorkerPool::submit` creates a new control instead of adopting `OperationContext::operation`; pre-start cancellation stores an exception in the promise.
- **Expected behavior:** Context and returned task cancellation share state and preserve the declared result contract.
- **Actual behavior:** Context cancellation is ignored and `Task::get()` can throw for cancellation.
- **Impact:** Shutdown/scheduler cancellation is ineffective and callers need undeclared exception handling.
- **Failure scenario:** Cancel the supplied operation after dispatch or cancel a queued task before start.
- **Root cause:** Independent worker control ownership.
- **Recommended fix:** Submit with the supplied operation state and normalize cancellation results.
- **Regression risks:** Operation status races.
- **Relevant validation:** Worker-local cancellation only; context identity is untested.

### MEDIUM-002: Project load failures can leave status stuck at `loading`

- [ ] **Remediation status:** Open.
- **Reference:** `project/project_session.cppm:97-137`.
- **Affected component:** Project lifecycle state machine.
- **Technical evidence:** Decoder-open and entry-point failures return after setting `loading` but do not set `failed` or restore the prior state.
- **Expected behavior:** Every failure has a stable lifecycle state and cleanup policy.
- **Actual behavior:** Missing SLA/invalid entry can leave a partially initialized session reporting `loading`.
- **Impact:** Retry and caller state handling are ambiguous.
- **Failure scenario:** Configure an invalid SLA and inspect state after `load()` returns an error.
- **Root cause:** Error transitions are handled on only some branches.
- **Recommended fix:** Centralize failure cleanup/status transition.
- **Regression risks:** Persisted input events and retry semantics.
- **Relevant validation:** No load-failure lifecycle test.

### MEDIUM-003: Event subscriber exceptions escape after durable commit

- [ ] **Remediation status:** Open.
- **References:** `event_bus/event_bus.cppm:32-42`, `projections/projection_coordinator.cppm:45-48`.
- **Affected component:** Runtime publication boundary.
- **Technical evidence:** Callbacks are invoked after durable persistence without exception containment.
- **Expected behavior:** Subscriber failures are isolated from commit success.
- **Actual behavior:** A callback exception escapes after state has advanced.
- **Impact:** Process termination or inconsistent caller-visible commit status.
- **Failure scenario:** Subscribe a throwing callback and commit an event.
- **Root cause:** No bus failure policy.
- **Recommended fix:** Catch/aggregate failures or use durable asynchronous delivery.
- **Regression risks:** Notification timing changes.
- **Relevant validation:** No throwing-subscriber test.

### MEDIUM-004: Project analysis records an invalid resource identity

- [ ] **Remediation status:** Open.
- **Reference:** `project/project_session.cppm:179-185`.
- **Affected component:** Runtime resource reproducibility.
- **Technical evidence:** Primary resource snapshots use byte size `0` and caller-provided hash text.
- **Expected behavior:** Snapshot identity matches loaded bytes.
- **Actual behavior:** Non-empty input can be represented as zero bytes.
- **Impact:** Resource validation/reproducibility is unreliable.
- **Failure scenario:** Validate the snapshot with `ResourceManager`.
- **Root cause:** Load metadata is not computed/propagated.
- **Recommended fix:** Compute actual digest and size before analysis.
- **Regression risks:** Load cost and metadata changes.
- **Relevant validation:** End-to-end tests use placeholder metadata.

## Findings: Low

### No findings

## Verified Strengths

- [x] Worker priority selection, append-only framing, checksum recovery, and in-memory replay have explicit implementations and focused tests.
- [x] Project manager, session, runtime core, and persistence responsibilities are separated into runtime modules.
- [x] Projection coordinator exposes one commit/rebuild boundary for future transactional remediation.

## Reviewed Areas With No Findings

- [x] Basic worker priority ordering and successful task completion.
- [x] Successful event-log replay and checkpoint advancement.
- [x] Basic project registration and close lookup behavior.
- [x] Runtime CMake module registration.

## Validation Results

- [x] Runtime source/CMake/test registration inspected read-only.
- [x] Existing generated CTest metadata inspected.
- [x] `ctest --test-dir build --output-on-failure`: 49/49 passed.
- [x] `git diff HEAD~2..HEAD --check`: passed.
- [ ] Fault injection, multi-process, sanitizer, and shutdown stress validation were not run.

## Unresolved Questions And Residual Risks

- [ ] Decide whether SQLite is authoritative domain projection or only an event checkpoint/cache.
- [ ] Define whether shutdown drains, cancels, or awaits queued native tasks.
- [ ] Define the transaction boundary between event log, memory projection, SQLite, and event publication.

## Follow-Up Decision

- [ ] Runtime findings `CRITICAL-001`, `HIGH-001` through `HIGH-003`, and `MEDIUM-001` through `MEDIUM-004` remain open.
- [x] Listing projection now persists/replays hash-critical bytes, masks, and operand object facts; full `\build.bat all` passed 49/49 tests.
