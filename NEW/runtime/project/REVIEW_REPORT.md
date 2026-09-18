# Runtime Project Logic Review

## Scope And Validation

- [x] Reviewed [`project_session.cppm`](project_session.cppm) against [`../../tests/integration/reports/FullPeRuntimePipelineProducesProjectionAndReport.md`](../../tests/integration/reports/FullPeRuntimePipelineProducesProjectionAndReport.md).
- [x] Reviewed analyzer registration, PE function seeding, bounded decode, analysis lifecycle, and decompiler request construction.
- [x] **Date:** 2026-09-18.
- [x] **Reviewer:** Kilo, independent read-only business-logic review.
- [x] **Reviewed diff:** current working-tree source, remediation changes, and checked-in golden report.
- [x] **Assumptions:** The aggregate analyzer fixture's checked-in Ghidra oracle is the expected full-analysis behavior; the facade report is expected to state accurately which profile it executes.
- [x] Baseline validation before remediation: `NEW\build.bat all`, 53/53 passed.

## Critical

- No additional critical finding local to this module. The false-green decompilation finding is recorded in [`../../tests/integration/REVIEW_REPORT.md`](../../tests/integration/REVIEW_REPORT.md) under `CRITICAL-001`.

## High

### HIGH-001: Native branch-target resolution remains incomplete after CFG boundary fix

- [ ] Remediated; CFG boundary portion is fixed.
- **Reference:** [`project_session.cppm:306-340`](project_session.cppm#L306-L340), report function rows at `:81-84`.
- **Evidence:** The loader now uses a bounded CFG worklist and known export boundaries, eliminating the shutdown/engine_tick overlap; `update_entity_pointer` still fails in native target-op resolution.
- **Expected behavior:** Each function body should contain its own reachable instructions and stop before the next known function seed; direct branch targets must be represented in the body.
- **Actual behavior:** The baseline loader advances linearly and imposes a fixed cap, so it consumes later exported functions and omits branch-target blocks.
- **Impact:** Incorrect function ownership and invalid native decompilation ranges.
- **Reproduction:** Compare the overlapping baseline report rows and decompile `recursive_score`/`switch_mode`; the report records native target-op failures.
- **Root cause:** Native target-op mapping remains incomplete after the loader CFG fix.
- **Recommended fix:** Complete native target-op mapping and add branch-target regression coverage.
- **Regression risks:** Indirect branches and terminal calls need explicit unresolved-target semantics; a read-ahead range must not cross the next function.
- **Regression validation:** Assert no body contains a later function entry and that direct branch targets have materialized instructions.

### HIGH-002: Only EntryMaterializationAnalyzer is registered

- [ ] Remediated.
- **Reference:** [`project_session.cppm:55-56`](project_session.cppm#L55-L56), report `Executed Analyzers`.
- **Evidence:** The facade analysis report contains only `runtime.entry_materialization`; no references, data objects, signatures, or analyzer-derived facts are produced.
- **Expected behavior:** The configured facade profile must register and execute every supported analyzer, or identify itself as an intentionally partial profile.
- **Actual behavior:** `analyze()` completes after one read-only invariant checker while the report is labeled PASS without a profile qualifier.
- **Impact:** The user-facing facade does not run the full supported analyzer pipeline.
- **Reproduction:** Compare report `Executed Analyzers` with `analyzer_global_integration_tests.cppm:270-281`, which expects 34 registered builtin analyzers and checks that each executes.
- **Root cause:** The per-session registry is populated with one analyzer and has no runtime analyzer composition factory.
- **Recommended fix:** Introduce a runtime analyzer profile/factory and register all supported analyzers, or mark the facade profile explicitly partial.
- **Regression risks:** Full registration requires deterministic dependency ordering and mutation-conflict handling.
- **Regression validation:** Assert analyzer IDs and known fixture entity counts.

### HIGH-003: References and data objects are never materialized

- [ ] Remediated.
- **Reference:** [`project_session.cppm:105-208`](project_session.cppm#L105-L208), report `SQLite References` and `SQLite Data Objects` sections.
- **Evidence:** The fixture contains calls, branches, `.data`, `.rdata`, and resources, but both durable tables contain zero rows.
- **Expected behavior:** References and data objects required by the configured analysis profile must be emitted as events, projected in memory, and persisted in SQLite.
- **Actual behavior:** The load path emits no reference/data events and the runtime has no active analyzer/command path that can add those entities; empty tables are accepted as PASS.
- **Impact:** Navigation and downstream analysis cannot consume cross-references or data facts.
- **Reproduction:** Inspect report `SQLite References`/`SQLite Data Objects` (both zero) while the same report's instruction table contains direct calls and memory operands.
- **Root cause:** Reference/data event producers and projection handlers are not wired into the facade path.
- **Recommended fix:** Add reference/data event contracts and direct control-flow/data-operand materializers before wiring higher-level analyzers.
- **Regression risks:** Entity IDs, duplicate suppression, and replay order must be deterministic across analyzer reruns.
- **Regression validation:** Assert known direct calls and data addresses in memory and SQLite projections.

## Medium

### MEDIUM-001: Function-session replay loses instruction p-code and flow facts

- [ ] Remediated.
- **Reference:** [`../../core/events/code_events.cppm:50-70`](../../core/events/code_events.cppm#L50-L70), [`../projections/software_model_projection.cppm:284-320`](../projections/software_model_projection.cppm#L284-L320).
- **Affected component:** Loaded project query and decompiler request providers.
- **Evidence:** The session emits listing events that omit flow/p-code; replay initializes only the instruction address, matching report metadata `pcode=0`/`target=<none>`.
- **Expected behavior:** The query used by later analysis/decompilation must preserve decoded flow and p-code facts at the committed revision.
- **Actual behavior:** The live projection is already the lossy replay representation, so decompilation receives a listing that cannot explain branch targets independently.
- **Impact:** Reopen and analysis behavior can differ from the decoder's original facts.
- **Reproduction:** Inspect any sampled branch/CALL row in the report and its metadata fields.
- **Root cause:** Listing event payload and projection do not carry the complete `core::Instruction` model.
- **Recommended fix:** Version and round-trip flow/p-code fields, or explicitly re-decode during projection rebuild with tests.
- **Regression risks:** Existing event frames require migration/default handling.
- **Relevant validation:** Round-trip a branch and a p-code-bearing memory instruction.

### MEDIUM-002: Function-session events cannot preserve signatures or variables

- [ ] Remediated.
- **Reference:** [`../../core/events/function_events.cppm:11-38`](../../core/events/function_events.cppm#L11-L38), [`../projections/software_model_projection.cppm:321-336`](../projections/software_model_projection.cppm#L321-L336).
- **Affected component:** Function query model and analyzer/decompiler handoff.
- **Evidence:** Function events carry only identity, name, range end, and status; replay creates no signature, variable, namespace, no-return, or instruction-start facts. The report's structured signature/variable fields remain empty.
- **Expected behavior:** Function facts returned by analysis/decompilation must be durable and queryable without reparsing presentation C text.
- **Actual behavior:** The session projects a minimal function snapshot and the report supplies a C-source heuristic as a display substitute.
- **Impact:** ABI/signature consumers and later analyzers cannot use recovered facts after replay.
- **Reproduction:** Inspect `Functions and Signatures` plus `Recovered Variables` in the report for sampled successful functions.
- **Root cause:** Function event schema and projection omit the structured fields.
- **Recommended fix:** Add versioned signature/variable event fields and map native results into the domain model.
- **Regression risks:** Storage locations and type IDs need stable serialization.
- **Relevant validation:** Assert parameter storage, calling convention, and variable counts through close/reopen.

## Verified Strengths

- [x] The session owns project lifecycle, event history, projection, and provider construction behind the facade.
- [x] PE regions and imported/exported symbols are emitted with stable project-scoped event identities.

## Reviewed Areas With No Findings

- [x] Project identity and event-log opening on the happy path.
- [x] Normal close path and worker-pool ownership were not found to lose resources in the inspected fixture run.

## Validation Results

- [x] Checked-in golden report and source symbols were inspected read-only.
- [x] Baseline `NEW\build.bat all` result recorded as 53/53 before concurrent remediation.
- [x] Post-remediation `NEW\build.bat project` and `NEW\build.bat integration` passed.

## Unresolved Questions And Residual Risks

- [ ] The supported runtime analyzer profile and event schema ownership remain unspecified.
- [ ] Concurrent CFG remediation must be regenerated and checked for adjacent-function isolation.

## Follow-Up

- [ ] Fix `HIGH-001`, `HIGH-002`, and `HIGH-003` after the integration report baseline is updated.
- [ ] Repeat the full review after remediation.
