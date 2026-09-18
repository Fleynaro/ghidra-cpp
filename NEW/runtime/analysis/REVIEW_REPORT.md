# Runtime Analysis Logic Review

## Scope And Validation

- [x] Reviewed [`analysis_scheduler.cppm`](analysis_scheduler.cppm), [`analyzer_registry.cppm`](analyzer_registry.cppm), the project-session call site, and the analyzer result contract.
- [x] Correlated scheduler output with [`../../tests/integration/reports/FullPeRuntimePipelineProducesProjectionAndReport.md`](../../tests/integration/reports/FullPeRuntimePipelineProducesProjectionAndReport.md), which reports one executed analyzer and no analysis diagnostics.
- [x] This was a read-only source review on 2026-09-18; no implementation or test source was changed.
- [x] **Reviewed diff:** current working-tree runtime analysis/project orchestration and mutation-commit remediation.
- [x] **Assumptions:** `MutationCommand` is a durable proposal contract, not merely diagnostic output, and analysis completion must mean its proposals were committed or explicitly rejected.

## Critical Findings

### No findings

No confirmed critical defect was identified in registry graph validation itself. Duplicate IDs, missing prerequisites, and dependency cycles are rejected by `AnalyzerRegistry::register_analyzer`/`ordered`.

## High Findings

### HIGH-001: Scheduler mutation proposals were collected and then discarded

- [x] **Remediation status:** Fixed in `ProjectSession::analyze`; command batches now commit before the completed run event.
- **Source reference:** [`analysis_scheduler.cppm:46-50`](analysis_scheduler.cppm#L46-L50) and [`../project/project_session.cppm:248-258`](../project/project_session.cppm#L248-L258).
- **Affected component:** Analyzer execution and project commit lane.
- **Technical evidence:** `AnalysisScheduler::run` appends every `AnalyzerResult::commands` item to `SchedulerReport::commands`, but `ProjectSession::analyze` uses only `executed_analyzers` and `diagnostics` when constructing `AnalysisSummary`; it never translates or commits `report->commands`. The analyzer contract explicitly defines `MutationCommand` as the output of read-only analysis (`core/contracts/analyzer.cppm:53-68`).
- **Expected behavior:** Successful analyzer proposals must be validated, committed as durable events, applied to the in-memory projection, and reflected in the returned committed revision before analysis is marked complete.
- **Actual behavior:** The previous orchestration dropped commands; it now creates durable mutation events through the same coordinator commit lane. No command-producing facade analyzer is wired yet, so end-to-end command execution remains pending.
- **Impact:** Analysis results are silently lost; repeated analysis/reopen cannot reproduce analyzer facts, and the reported analysis revision does not include analyzer mutations.
- **Reproduction or failure scenario:** Register an analyzer whose `analyze` returns one valid `MutationCommand`; `AnalysisScheduler::run` reports it in `commands`, `ProjectSession::analyze` still commits only `AnalysisRunStateChanged` start/finish events, and the query/database remain unchanged.
- **Root cause:** The orchestration boundary stops after scheduler collection and omits the command-to-`EventDraft` commit step.
- **Recommended fix:** Add a command-producing analyzer fixture and validate command revision, entity identity, idempotency keys, and replay parity.
- **Regression risks:** Commit failures must leave analysis status failed and avoid reporting a completed run; event ordering and rerun idempotency need explicit tests.
- **Relevant tests or validation:** Add a command-producing fake analyzer and assert event-log, in-memory, SQLite, and reopen parity. The current golden report has no command count and cannot detect this failure.

## Medium Findings

### MEDIUM-001: Declared execution mode and worker scheduling are not enforced

- [ ] **Remediation status:** Open.
- **Source reference:** [`analysis_scheduler.cppm:23-52`](analysis_scheduler.cppm#L23-L52), especially the unused `workers_` member and synchronous `task.get()` at lines 42-44; descriptor fields are defined in [`../../core/contracts/analyzer.cppm:26-39`](../../core/contracts/analyzer.cppm#L26-L39).
- **Affected component:** Analyzer scheduling semantics, cancellation, and runtime responsiveness.
- **Technical evidence:** The registry preserves `preferred_mode`, `ExecutionClass`, `RunPolicy`, and mutation flags, but `AnalysisScheduler::run` invokes every triggered analyzer in the caller thread and immediately blocks on `task.get()`. It does not dispatch queued analyzers to the worker pool, enforce serial-mutating ordering, or apply one-time/repeatable policy.
- **Expected behavior:** Descriptor scheduling metadata must determine dispatch and repeat behavior, while cancellation and project commit serialization remain observable in the analysis lifecycle.
- **Actual behavior:** A descriptor marked queued or one-time behaves like an inline incremental analyzer. A slow or blocking analyzer can stall `ProjectSession::analyze`, and a repeat-policy violation is not rejected.
- **Impact:** Runtime responsiveness and analyzer lifecycle contracts are incorrect; unsupported concurrency or duplicate runs can produce nondeterministic state once mutation-producing analyzers are registered.
- **Reproduction or failure scenario:** Register a queued analyzer that blocks or a one-time analyzer, call `analyze` twice, and observe that the first call blocks synchronously and the analyzer is eligible again on the second call.
- **Root cause:** The scheduler treats descriptors as metadata only and has no dispatch/policy state.
- **Recommended fix:** Route queued work through `WorkerPool`, preserve dependency barriers, enforce `ExecutionClass` commit ordering, track run policy per analyzer/resource revision, and propagate operation cancellation/status without unchecked blocking.
- **Regression risks:** Parallel analyzers must still receive immutable snapshots and deterministic proposal ordering.
- **Relevant tests or validation:** Add tests for queued dispatch, cancellation, one-time versus incremental runs, and deterministic ordering of independent analyzers.

## Low Findings

### No findings

## Verified Strengths

- [x] `AnalyzerRegistry` rejects null analyzers and empty/duplicate stable IDs.
- [x] Prerequisite resolution rejects missing IDs and cycles and orders ready nodes by priority then stable ID.
- [x] Trigger matching is deterministic over the supplied event batch.

## Reviewed Areas With No Findings

- [x] Stable-ID uniqueness validation.
- [x] Missing-prerequisite and cycle detection.
- [x] Deterministic topological ordering for the registered graph.

## Validation Results

- [x] Source inspection and mutation-commit remediation completed.
- [x] Integration golden report was read through its analyzer, diagnostics, entity, and decompilation sections.
- [ ] No command-producing runtime analyzer is currently wired into `ProjectSession`, so command persistence still needs a dedicated runtime execution test.

## Unresolved Questions And Residual Risks

- [ ] Runtime analyzer composition is currently a project-session concern; the supported analyzer profile and migration boundary need an explicit owner.
- [ ] A command event schema for signatures, references, and data objects must be agreed before scheduler mutation tests can be end-to-end.

## Follow-Up Decision

- [x] Fix the scheduler-to-commit loss path; [ ] add command-producing end-to-end coverage.
- [ ] Address `MEDIUM-001` before enabling queued or mutation-producing analyzers in the facade pipeline.
