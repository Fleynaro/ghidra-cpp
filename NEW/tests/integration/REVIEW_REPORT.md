# Runtime Integration Logic Review

## Scope

- [x] Reviewed the generated contract at [`reports/FullPeRuntimePipelineProducesProjectionAndReport.md`](reports/FullPeRuntimePipelineProducesProjectionAndReport.md).
- [x] Reviewed the facade pipeline in [`runtime/project/project_session.cppm`](../../runtime/project/project_session.cppm), the analyzer scheduler/registry, the SQLite projection in [`runtime/storage/sqlite_projection_store.cppm`](../../runtime/storage/sqlite_projection_store.cppm), the event encoders, and the native decompiler adapter.
- [x] Reviewed the integration test and shared report workflow in [`end_to_end_tests.cppm`](end_to_end_tests.cppm) and [`report_generator.cppm`](report_generator.cppm).
- [x] **Date:** 2026-09-18.
- [x] **Reviewer:** Kilo, independent read-only business-logic review.
- [x] **Reviewed diff:** current working-tree sources and the checked-in golden report; findings describe confirmed behavior in the report baseline, and concurrent remediation is not considered validated until the report is regenerated.
- [x] **Assumptions:** The aggregate fixture oracle documents the expected full analyzer behavior; the facade report must either meet that profile or label its reduced profile explicitly.
- [x] Reviewed baseline validation from `NEW\build.bat all`: 53/53 tests passed before remediation.
- [x] Post-remediation `NEW\build.bat all` also passed 53/53.

## Critical Findings

### CRITICAL-001: The test still passes when a sampled decompilation is failed

- [ ] Remediated: the generated report now derives `PASS`/`PARTIAL` from decompilation statuses and preserves diagnostics, but the test still accepts a failed result and returns success when the `PARTIAL` golden text matches.
- **Reference:** `report_generator.cppm`, `run_full_pipeline_report`; `reports/FullPeRuntimePipelineProducesProjectionAndReport.md:781-865,916-953`.
- **Affected component:** Integration business-result contract and native decompiler service.
- **Evidence:** The current report header says `Status: PARTIAL` and `update_entity_pointer` remains `Status: failed` with `Could not find op at target address`. `run_full_pipeline_report` still checks only that `Result<Decompilation>` exists and fallback `c_source` is non-empty (`report_generator.cppm:681-695`); the GTest passes if the partial report matches the golden file.
- **Expected behavior:** A full-pipeline test must fail when a requested decompilation fails, or explicitly report a non-passing status that cannot be mistaken for success.
- **Actual behavior:** Native failure is converted into fallback C text, displayed as `PARTIAL`, and still accepted as a successful integration test.
- **Impact:** CI can remain green while the full-pipeline test contains a failed native decompilation; consumers may treat the generated artifact as an accepted baseline.
- **Reproduction:** Run `NEW\build.bat integration`; inspect `update_entity_pointer` at report `:950-989` and observe that the golden comparison succeeds despite `Status: failed`.
- **Root cause:** [`services/decompiler/decompiler_service.cppm`](../../services/decompiler/decompiler_service.cppm) returns a failed-but-usable fallback; [`report_generator.cppm`](report_generator.cppm) accepts that status as success.
- **Recommended fix:** Reject failed statuses in `run_full_pipeline_report` when this test claims full native coverage, or register a separate explicitly-successful partial test target whose name/status cannot be mistaken for full-pipeline success.
- **Regression risk:** Existing entry-stub fallback behavior may need an explicit separate compatibility test.
- **Validation:** Add assertions for every selected result status and a golden status field derived from those statuses.

## High Findings

### HIGH-001: Branch-target/provider resolution remains incomplete after CFG boundary fix

- [ ] Remediated; CFG boundary portion is fixed, native target mapping remains open.
- **Reference:** [`runtime/project/project_session.cppm:306-340`](../../runtime/project/project_session.cppm#L306-L340), especially the `for (count < 64)` sequential `address.offset += instruction.length` loop; report `:81-84,262-263`.
- **Affected component:** PE function discovery, function boundaries, and decompiler request ranges.
- **Evidence:** The current report bounds `shutdown_engine` before `engine_tick` and expands reachable branch bodies, but `update_entity_pointer` still has a native target-op failure.
- **Expected behavior:** A function body must be bounded by control flow/function starts and include reachable branch targets without consuming the next exported function.
- **Actual behavior:** CFG discovery now uses visited/pending sets and known export boundaries; one native target operation still cannot be resolved.
- **Impact:** Incorrect function ownership, duplicated instructions, invalid decompiler ranges, and missed control-flow paths.
- **Reproduction:** Compare the overlapping report rows and the `Could not find op at target address` diagnostics for `recursive_score` and `switch_mode`.
- **Root cause:** Native provider target-op mapping remains incomplete after the loader CFG fix.
- **Recommended fix:** Make native target-op addresses resolve against the complete bounded CFG and add a direct regression test for `update_entity_pointer`.
- **Regression risk:** Indirect branches/calls must not be treated as direct CFG targets; external targets require explicit unresolved-reference handling.
- **Validation:** Assert no function body overlaps a later known function entry and compare branch target addresses against materialized instructions.

### HIGH-002: The facade analysis pipeline runs only one analyzer

- [ ] Remediated.
- **Reference:** [`runtime/project/project_session.cppm:55-56`](../../runtime/project/project_session.cppm#L55-L56); report `:20-22`.
- **Affected component:** Analyzer registration and user-visible analysis completeness.
- **Evidence:** `ProjectSession::open` registers only `EntryMaterializationAnalyzer`, and the report lists only `runtime.entry_materialization`. The broader analyzer suite is tested separately and is not connected to this facade runtime.
- **Expected behavior:** The user-facing project pipeline should register and execute the analyzers advertised as part of the runtime, or clearly expose a reduced profile.
- **Actual behavior:** `analyze()` completes with no diagnostics after one analyzer, while the report's PASS status can be mistaken for full analysis.
- **Impact:** Functions, references, data, signatures, calling conventions, strings, and resources are not discovered by the facade pipeline.
- **Reproduction:** Inspect `Executed Analyzers` in the report and compare with the analyzer targets under [`services/analyzers`](../../services/analyzers).
- **Root cause:** The registry is created per session and populated with one analyzer only; no runtime analyzer composition/configuration is wired.
- **Recommended fix:** Add an explicit runtime analyzer profile/registry factory and include all supported analyzers, or mark the report status/profile as partial and assert the expected profile.
- **Regression risk:** Analyzer ordering, mutation conflicts, and performance need dedicated coverage.
- **Validation:** Assert the configured analyzer set and materialized entity counts for a fixture with known expected outputs.

### HIGH-003: References and data objects are silently absent from the projection

- [ ] Remediated.
- **Reference:** Report `:37-44,548-566`; [`runtime/project/project_session.cppm:105-208`](../../runtime/project/project_session.cppm#L105-L208); [`core/contracts/project_query.cppm:40-43`](../../core/contracts/project_query.cppm#L40-L43).
- **Affected component:** Software model projection and SQLite durable model.
- **Evidence:** The fixture has direct calls, conditional branches, memory operands, `.data`, `.rdata`, and resource regions, yet both `SQLite References` and `SQLite Data Objects` contain zero rows. The load path emits only project input, memory, symbol, listing, and function events.
- **Expected behavior:** Analysis should materialize references and data objects, or the report should explicitly classify the current runtime as unsupported for those entity kinds.
- **Actual behavior:** Empty tables are treated as a normal PASS and no analyzer command produces those entities.
- **Impact:** Cross-references, data-flow/navigation, strings, globals, and resource-backed facts are unavailable to users and downstream analyzers.
- **Reproduction:** Inspect the empty tables and compare them with the calls/data operands visible in the instruction table.
- **Root cause:** No reference/data event contracts or runtime materializers are wired into the facade pipeline.
- **Recommended fix:** Add reference/data event types and materializers for direct control-flow/data operands first, then register the relevant analyzer services.
- **Regression risk:** Entity identity and replay ordering must remain deterministic.
- **Validation:** Assert known call/reference/data entities from the fixture in both in-memory and SQLite projections.

## Medium Findings

### MEDIUM-001: Durable instruction projection drops p-code and flow-target facts

- [ ] Remediated.
- **Reference:** [`core/events/code_events.cppm:50-70`](../../core/events/code_events.cppm#L50-L70); [`runtime/projections/software_model_projection.cppm:284-320`](../../runtime/projections/software_model_projection.cppm#L284-L320); report instruction metadata `pcode=0` and `target=<none>`.
- **Affected component:** Event replay, durable SQLite projection, and decompiler/provider observability.
- **Evidence:** `ListingStateChanged` serializes bytes, mask, and operands but not flow kind/target or p-code operations. The projection initializes `instruction.pcode.instruction` but never restores operations. The report consequently shows `pcode=0` and no targets even for visible `JG/JZ/CALL/JMP` instructions.
- **Expected behavior:** Replaying the event log into memory/SQLite must preserve the instruction facts needed by analysis and diagnostics.
- **Actual behavior:** A fresh projection loses p-code and control-flow metadata and cannot independently explain the decompiler result.
- **Impact:** Replay/reopen behavior differs from the live decoder; downstream analyzers cannot use durable p-code/flow facts.
- **Root cause:** Event schema and SQLite instruction table are narrower than `core::Instruction`.
- **Recommended fix:** Extend event/schema serialization for flow, operands, and p-code (or explicitly define a replay-time decoder contract and test it).
- **Regression risk:** Event schema versioning and old project migration are required.
- **Validation:** Round-trip one branch, one memory operation, and one p-code operation through events and SQLite.

### MEDIUM-002: Function signatures and variables are not first-class projection facts

- [ ] Remediated.
- **Reference:** [`core/events/function_events.cppm:11-34`](../../core/events/function_events.cppm#L11-L34); report `Functions and Signatures` and `Recovered Variables` sections.
- **Affected component:** Function model, signature analysis, and report correctness.
- **Evidence:** Function events serialize only id/space/entry/end/name/status/instruction starts/body. The report can show a C-source signature for a few sampled functions, but structured signatures, parameters, ABI, and variables remain `<none>`/zero.
- **Expected behavior:** Analyzer-derived signature and variable facts should be stored in `FunctionSnapshot`, replayed, and exposed through the query contract.
- **Actual behavior:** The report heuristically parses the first C declaration line, while the durable model has no corresponding facts.
- **Impact:** Signatures are not queryable or reproducible and cannot drive later analyzer decisions.
- **Root cause:** Function event payload and projection schema omit signature/variable fields; decompiler result mapping is incomplete.
- **Recommended fix:** Version function signature/variable serialization and map native recovered facts into core domain types.
- **Regression risk:** ABI/type identifiers and backward-compatible event replay need explicit tests.
- **Validation:** Assert parameter types/storage/calling convention for the decorated fixture exports.

### MEDIUM-003: The report does not assert in-memory/SQLite entity parity

- [ ] Remediated.
- **Reference:** [`report_generator.cppm:659-662`](report_generator.cppm#L659-L662) and [`report_generator.cppm:696-701`](report_generator.cppm#L696-L701).
- **Affected component:** Full-pipeline acceptance contract and durable projection verification.
- **Evidence:** The workflow now checks the SQLite checkpoint and counts for functions, instructions, regions, symbols, and data objects, but it still computes no address-set, status, reference-count, or complete payload equality between the two representations.
- **Expected behavior:** A projection/report test must prove that durable rows represent the same current entities and addresses as the in-memory query at the reported revision.
- **Actual behavior:** A database with correct row counts but a wrong address, stale status, or mismatched entity payload can still pass; the Markdown merely prints both views.
- **Impact:** Durable divergence can reach users and survive reopen while CI remains green.
- **Reproduction:** Alter one SQLite function/instruction address or status without changing table counts, then run `read_projection`; current checkpoint/count checks still pass and report generation proceeds.
- **Root cause:** The report workflow treats SQLite inspection as a presence check rather than a cross-store invariant check.
- **Recommended fix:** Compare revisions and canonicalized entity keys/counts/addresses/statuses for functions, instructions, regions, symbols, references, and data objects; fail with a precise first mismatch.
- **Regression risks:** Stable ordering and entity fields must be normalized consistently; sampled report sections must not be used as the parity source.
- **Validation:** Add a test helper that compares complete sorted sets and injects missing/wrong-address durable rows to prove the guard fails.

## Low Findings

### LOW-001: Report PASS wording overstates analyzer completeness

- [ ] Remediated.
- **Reference:** Report `:3,20-22,31-35` and [`report_generator.cppm`](report_generator.cppm).
- **Evidence:** The report says PASS while documenting one analyzer and empty references/data tables.
- **Expected behavior:** The report status and wording should identify the tested profile and must not imply that unsupported analyzer/entity classes passed.
- **Actual behavior:** `Status: PASS` is unconditional report text even when only one analyzer executes and references/data are empty.
- **Impact:** Readers may interpret a bounded facade smoke test as equivalent to the full legacy analyzer pipeline.
- **Reproduction:** Read the report header and compare `Executed Analyzers` plus the empty reference/data sections.
- **Root cause:** Report generation hard-codes PASS instead of deriving status/profile from analyzer coverage and decompilation/entity outcomes.
- **Recommended fix:** Add an explicit `profile=facade-partial` status/coverage field and reserve PASS for the asserted profile.
- **Regression risks:** A stricter status must distinguish intentionally unsupported optional entity kinds from unexpected omissions.
- **Validation:** Add a golden assertion that status/profile agrees with the configured analyzer IDs and required entity counts.

## Verified Strengths

- [x] PE regions and imported/exported symbols are persisted in SQLite with deterministic ordering.
- [x] The report uses stable seed-based sampling and repository-relative paths.
- [x] SQLite checkpoint and in-memory revision agree at regenerated revision 539.
- [x] The selected simple functions produce native C output with readable signatures.
- [x] Task completion ordering is checked by the integration workflow.

## Validation And Limitations

- [x] Reviewed the complete checked-in report, including all sections within configured limits.
- [x] `NEW\build.bat all` passed 53/53 after the remediation work.
- [ ] No independent Ghidra reference output was available for byte-for-byte semantic comparison.
- [ ] The report samples event records and SQLite instructions, so unsampled rows require direct SQLite inspection.

## Reviewed Areas With No Findings

- [x] Fixture and SLA existence checks are explicit and repository-relative.
- [x] Report sampling uses deterministic ordering and a stable seed.
- [x] SQLite checkpoint and in-memory revision agree for the regenerated fresh database.

## Unresolved Questions And Residual Risks

- [ ] The intended supported analyzer profile for the facade is not declared in runtime configuration.
- [ ] Concurrent CFG/decompiler remediation must be validated by regenerating the golden report rather than inferred from source changes.
- [ ] Fault-injection coverage for partial event/SQLite commits is absent.

## Follow-Up Decision

- [x] Fix CRITICAL-001; [ ] continue HIGH-001, HIGH-002, and HIGH-003.
- [ ] After those fixes, re-run the golden report and repeat this review.
