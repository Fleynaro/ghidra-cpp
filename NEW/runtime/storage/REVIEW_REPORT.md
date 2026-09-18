# SQLite Projection Logic Review

## Scope And Validation

- [x] Reviewed [`sqlite_projection_store.cppm`](sqlite_projection_store.cppm), event persistence schema, and the SQLite sections of [`../../tests/integration/reports/FullPeRuntimePipelineProducesProjectionAndReport.md`](../../tests/integration/reports/FullPeRuntimePipelineProducesProjectionAndReport.md).
- [x] Reviewed fresh projection counts after the PE pipeline: checkpoint 539, functions 31, instructions 444, symbols 53, references 0, data objects 0.
- [x] **Date:** 2026-09-18.
- [x] **Reviewer:** Kilo, independent read-only business-logic review.
- [x] **Reviewed diff:** current working-tree storage/project recovery code and checked-in golden report.
- [x] **Assumptions:** The event log is authoritative and SQLite is required to be a replayable materialized view of the same project revision.

## Critical

- No critical defect was independently confirmed in SQLite transaction/checkpoint handling during this pass.

## High

### HIGH-001: Durable projection is structurally incomplete for references/data

- [ ] Remediated.
- **Reference:** [`sqlite_projection_store.cppm:46-49`](sqlite_projection_store.cppm#L46-L49), report `:548-566`.
- **Evidence:** `references_projection` and `data_objects` tables exist but no event path populates them, despite corresponding query-contract entity kinds and fixture evidence.
- **Expected behavior:** Every supported reference/data event must be applied to the durable table in the same transaction as its event identity and checkpoint.
- **Actual behavior:** The tables are created but `persist` has no `ReferenceStateChanged`/`DataStateChanged` branches; the current report therefore contains zero rows and a passing checkpoint.
- **Impact:** Reopen/replay of the project cannot recover cross-references or data entities.
- **Reproduction:** Query the report's `SQLite References` and `SQLite Data Objects` sections after the fixture run; both are empty while instruction rows contain call and memory-operand evidence.
- **Root cause:** The durable schema was added before reference/data event producers and projection handlers.
- **Recommended fix:** Add versioned reference/data event schemas, persistence branches, replay tests, and known-fixture assertions.
- **Regression risks:** Reference target spaces, nullable/external targets, duplicate identity, and data range updates must be encoded without lossy coercion.
- **Regression validation:** Compare live projection and reopened SQLite counts for both entity kinds.

## Medium

### MEDIUM-001: Listing events do not persist p-code or control-flow facts

- [ ] Remediated.
- **Reference:** [`../../core/events/code_events.cppm:50-70`](../../core/events/code_events.cppm#L50-L70), [`../projections/software_model_projection.cppm:284-320`](../projections/software_model_projection.cppm#L284-L320).
- **Evidence:** The report's SQLite instruction metadata shows `pcode=0` and `target=<none>` for branch instructions because the event payload carries bytes/operands only and replay initializes only the instruction address.
- **Expected behavior:** The durable instruction row and replayed in-memory instruction must preserve flow kind/target and p-code operations needed by analysis and navigation.
- **Actual behavior:** The report exposes mnemonic/bytes/operands but replay leaves `FlowInfo` defaulted and `PcodeSequence` empty; SQLite cannot represent those facts at all.
- **Impact:** A reopened projection is not equivalent to the live decoded model and cannot support p-code/flow-driven analyzers.
- **Reproduction:** Inspect `SQLite Instructions` metadata for visible `JG`, `CALL`, or `JMP` rows; each reports `flow=0`, `target=<none>`, and `pcode=0`.
- **Root cause:** Event/schema serialization omits flow and p-code fields.
- **Recommended fix:** Version listing events and persist flow/p-code or define and test deterministic replay-time decoder reconstruction.
- **Regression risks:** Existing event frames need a migration/default policy; malformed p-code must fail transactionally rather than silently default.
- **Regression validation:** Round-trip branch, memory, and p-code instructions through events and SQLite.

### MEDIUM-002: Opening an existing project could leave SQLite stale while memory was rebuilt

- [x] Remediated in `ProjectSession::open`: durable rebuild now runs through `ProjectionCoordinator` before the session becomes ready.
- **Reference:** [`../project/project_session.cppm:39-60`](../project/project_session.cppm#L39-L60), [`../projections/projection_coordinator.cppm:51-63`](../projections/projection_coordinator.cppm#L51-L63), and [`sqlite_projection_store.cppm:19-57`](sqlite_projection_store.cppm#L19-L57).
- **Affected component:** Project recovery and in-memory/SQLite revision consistency.
- **Technical evidence:** `ProjectSession::open` reads `events.log` and calls `projection->rebuild(stream->events)`, then sets the session revision from that rebuilt in-memory projection. It never calls `ProjectionCoordinator::rebuild` or persists those events into SQLite. The coordinator's durable rebuild exists as a separate method but is not used by the open path.
- **Expected behavior:** Opening a project must either validate the durable checkpoint and rebuild SQLite from authoritative history when it is missing/stale, or fail explicitly before exposing a ready session.
- **Actual behavior:** The previous open path rebuilt only memory; it now replays the authoritative event stream into both projections. Fault-injection/reopen parity still needs a dedicated test.
- **Impact:** Consumers reading SQLite after recovery can miss functions, instructions, symbols, and analysis runs even though the live query appears complete; later reports and exports become inconsistent.
- **Reproduction or failure scenario:** Create and load a project, close it, remove `projection.sqlite` while retaining `events.log`, then reopen. `open()` rebuilds memory from the log but only recreates empty SQLite tables/metadata; no durable rows or checkpoint are restored.
- **Root cause:** Event-log replay and durable projection replay are separate, and recovery invokes only the in-memory half.
- **Recommended fix:** Add checkpoint comparison and a close/delete/reopen test to verify the new coordinator rebuild path under failure.
- **Regression risks:** Rebuild must be idempotent and must not duplicate rows/events; test interruption and corrupt-event handling separately.
- **Relevant tests or validation:** Add a close/delete-SQLite/reopen test that compares checkpoint, counts, addresses, and representative rows in memory and SQLite.

## Verified Strengths

- [x] SQLite writes applied-event identity, event payload, and checkpoint in one transaction per event.
- [x] Memory, function, instruction, symbol, and analysis-run tables use deterministic stable keys for the current fixture.

## Reviewed Areas With No Findings

- [x] Basic schema creation and project metadata initialization.
- [x] Checkpoint monotonicity and transaction rollback paths in `persist` for the event kinds currently handled.

## Validation Results

- [x] Read-only inspection of storage source and the checked-in report completed.
- [x] Current report checkpoint and counts were recorded: revision 539, functions 31, instructions 444, symbols 53.
- [ ] No fault-injection or close/delete/reopen runtime test was run; the normal project/integration tests passed after the recovery fix.

## Unresolved Questions And Residual Risks

- [ ] Migration behavior for existing SQLite schemas after adding reference/data/p-code columns is unspecified.
- [ ] A durable rebuild must define whether old materialized rows are cleared atomically before replay.

## Follow-Up

- [ ] Coordinate `HIGH-001` with runtime project event producers.
- [ ] Add recovery parity coverage for `MEDIUM-002`.
- [ ] Repeat SQLite reopen/replay validation after fixes.
