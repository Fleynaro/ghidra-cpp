# Decompiler Service Logic Review

## Scope And Validation

- [x] Reviewed [`decompiler_service.cppm`](decompiler_service.cppm) and selected decompilation sections in [`../../tests/integration/reports/FullPeRuntimePipelineProducesProjectionAndReport.md`](../../tests/integration/reports/FullPeRuntimePipelineProducesProjectionAndReport.md).
- [x] Confirmed native successes and failures through the integration golden report and `NEW\build.bat all`.
- [x] **Date:** 2026-09-18.
- [x] **Reviewer:** Kilo, independent read-only business-logic review.
- [x] **Reviewed diff:** current working-tree decompiler adapter/remediation and checked-in golden report.
- [x] **Assumptions:** `DecompilationStatus::failed` is not an acceptable successful result, and structured core fields are part of the public decompiler contract rather than report-only decoration.

## Critical

### CRITICAL-001: Failed native decompilation was exposed as usable fallback while the pipeline remained PASS

- [x] Remediated at the report acceptance boundary: failures now produce `PARTIAL` status and remain visible.
- **Reference:** [`decompiler_service.cppm:148-180`](decompiler_service.cppm#L148-L180), [`decompiler_service.cppm:219-250`](decompiler_service.cppm#L219-L250), report `:781-865,916-953`.
- **Affected component:** Native decompiler result status and project-facing decompilation API.
- **Evidence:** `recursive_score`, `switch_mode`, and `update_entity_pointer` have `Status: failed` and native target diagnostics, yet the integration test accepts non-empty fallback C and reports PASS.
- **Expected behavior:** Native failure must fail the requested operation or make the overall report non-passing.
- **Actual behavior:** A synthetic fallback is retained for diagnostics, but the report no longer claims full PASS.
- **Impact:** Silent semantic loss is no longer hidden; native target resolution remains a high finding.
- **Root cause:** Fallback is returned as a `Result<Decompilation>` and the orchestration checks only source non-emptiness.
- **Recommended fix:** Fix target-op resolution before reclassifying the report as complete.
- **Regression validation:** Assert every selected result is `DecompilationStatus::complete` and contains raw instructions/control flow.

## High

### HIGH-001: Provider resolves the selected direct branch targets

- [x] Remediated for the fixture's selected branch/interprocedural samples.
- **Reference:** [`decompiler_service.cppm:62-82`](decompiler_service.cppm#L62-L82), report diagnostics at `:823,865,953`.
- **Affected component:** Native p-code provider and branch-target flow recovery.
- **Evidence:** The regenerated report shows all 10 selected decompilations as `complete`; `update_entity_pointer` now emits `update_entity(param_1,param_2 + 1)`.
- **Expected behavior:** The provider must make every decoded direct branch target available to native flow recovery, or return an explicit incomplete-CFG diagnostic before claiming native decompilation.
- **Actual behavior:** The loader CFG and bounded direct-callee/flow providers expose known child ranges and classify tail jumps with native `callreturn` semantics.
- **Impact:** Control-flow functions cannot be decompiled, while straight-line functions succeed.
- **Reproduction:** Decompile `recursive_score`, `switch_mode`, and `update_entity_pointer`; the integration regression now requires complete status for all selected samples.
- **Root cause:** The loader lacked reachable CFG bodies and the native adapter omitted bounded child/flow providers; override names also had to match native lowercase identifiers.
- **Recommended fix:** Preserve the CFG, root-only FunctionProvider, and `callreturn` FlowProvider regression.
- **Regression risks:** CFG discovery must keep indirect/external targets unresolved rather than inventing addresses, and range bounds must not consume adjacent functions.
- **Regression validation:** Decompile `recursive_score`, `switch_mode`, and `update_entity_pointer` with complete native status.

### HIGH-002: Decompilation has no provider for direct calls into other project functions

- [x] Remediated for direct tail-call target `0x1400012b0`.
- **Reference:** [`decompiler_service.cppm:135-141`](decompiler_service.cppm#L135-L141), [`src/decompiler.cppm:596-604`](src/decompiler.cppm#L596-L604), and current report `:950-989`.
- **Affected component:** Native flow recovery for interprocedural calls.
- **Technical evidence:** The adapter supplies a root-only direct-callee `FunctionProvider` and `callreturn` `FlowProvider`; the regenerated report marks `update_entity_pointer` complete at `0x140001300`.
- **Expected behavior:** A decompilation request must either resolve direct calls to known project functions through a bounded function provider or classify the external target without aborting the caller's native decompilation.
- **Actual behavior:** Native flow resolves the known callee through the provider and emits a call/return C expression.
- **Impact:** Interprocedural functions fail while simple and self-recursive functions succeed; a partial report cannot provide complete decompilation for the project sample.
- **Reproduction:** Run the current integration pipeline and inspect `update_entity_pointer` at `0x140001300`; the diagnostic target is `0x1400012b0`.
- **Root cause:** The initial service omitted a function provider and used non-native uppercase override names; both are corrected.
- **Recommended fix:** Keep provider scope root-bounded and add mutual-recursion/tail-call fixtures.
- **Regression risks:** Recursive/mutual calls must not recurse unboundedly; provider ranges must retain per-function names/boundaries and avoid cross-function byte leakage.
- **Relevant tests or validation:** Add direct cross-function, mutual-recursion, and external-call decompilation cases and require complete status for each supported case.

## Medium

### MEDIUM-001: Native structured signature and analysis facts are discarded

- [ ] Remediated.
- **Reference:** [`decompiler_service.cppm:155-172`](decompiler_service.cppm#L155-L172), [`src/decompiler.cppm:607-625`](src/decompiler.cppm#L607-L625), and report `:573-624,867-914`.
- **Affected component:** `IDecompiler` result contract and signature/variable consumers.
- **Technical evidence:** The native result contains `raw_instructions`, p-code/provenance, and C output, but the adapter copies only status, optional text, control flow, and query instructions. It never maps native signature/variables/switch/evidence/cache facts. The report masks null `recovered_signature` by parsing the first C declaration (`report_generator.cppm:477-502,560-570`), while recovered-variable sections remain zero.
- **Expected behavior:** Every result field promised by `core::Decompilation` should be populated from native analysis or explicitly marked unavailable with a diagnostic; report rendering must not manufacture a substitute for the query contract.
- **Actual behavior:** Callers receive readable C text but cannot query recovered ABI/signature/variables/switches/evidence, and the displayed fallback signature is not a structured fact.
- **Impact:** Function signature propagation and downstream analyzers cannot use the decompiler output; results are not reproducible across consumers that do not parse presentation text.
- **Reproduction:** Inspect any successful sampled result: `recovered_signature` is absent, `Recovered Variables` is zero, and only a report-only `C source:` signature is shown.
- **Root cause:** The adapter treats native `DecompilationResult` as text/control-flow output and omits the domain conversion layer.
- **Recommended fix:** Define native-to-core mappings for signature, storage, variables, switches, evidence, and cache identity; make report formatting consume those structured values without fallback parsing except in an explicitly labeled compatibility view.
- **Regression risks:** Native type/storage names and variable lifetimes need stable conversion rules; preserve diagnostics for fields that native analysis cannot recover.
- **Relevant tests or validation:** Add assertions for return type, calling convention, parameter storage, switch facts, and variables on decorated fixture exports.

### MEDIUM-002: Native read-ahead could cross the next function boundary

- [x] Remediated: native range is again bounded to `body_end + 1`; CFG boundary work is handled in the project loader.
- **Reference:** [`decompiler_service.cppm:146-156`](decompiler_service.cppm#L146-L156) and the fixture function boundaries in [`../../services/analyzers/tests/data/test_analyzers_integration.md:773-835`](../../services/analyzers/tests/data/test_analyzers_integration.md#L773-L835).
- **Affected component:** Native function range construction and decompiler isolation.
- **Technical evidence:** The adapter now uses `native_end = body_end + 1`; CFG seed boundaries and the loader body range prevent unconditional read-ahead into the next export.
- **Expected behavior:** Native decompilation must read only the requested function body and its explicitly reachable blocks, never bytes belonging to an adjacent function.
- **Actual behavior:** The previous fixed read-ahead was removed; the native range now stops at the exclusive end of the materialized body.
- **Impact:** C output, CFG, raw instructions, and signatures can include instructions attributed to the wrong function; end-of-image/adjacent-function behavior becomes data-dependent.
- **Reproduction:** Request decompilation for `fixture_entry` with its current body end and inspect native raw flow or output around `0x140001810`.
- **Root cause:** Read-ahead was added to compensate for target-op lookup, but the native range API has no upper-bound parameter and memory provider remains unrestricted.
- **Recommended fix:** Supply a CFG-complete bounded range or a provider that rejects reads at/after the next known function boundary; do not use unconditional read-ahead.
- **Regression risks:** Branch targets immediately after the nominal end must be represented as part of the function before range construction, not admitted by over-reading.
- **Relevant tests or validation:** Add a neighboring-function isolation test that asserts no decompilation artifact references an address outside the selected function's materialized instruction set.

## Follow-Up

- [x] Fix `CRITICAL-001` and native target-op resolution; [ ] add broader interprocedural fixtures.
- [ ] Fix `MEDIUM-001` before exposing decompiler results as structured analysis input.
- [ ] Fix `MEDIUM-002` before accepting read-ahead as a CFG workaround.
- [ ] Repeat the report review after provider target resolution changes.

## Verified Strengths

- [x] Successful straight-line samples preserve readable native C and control-flow text.
- [x] Failed native calls retain diagnostics and a failed status in the `Decompilation` value rather than throwing across the task boundary.
- [x] Provider reads use the canonical memory/decoder contracts and the required x86-64 processor context.

## Reviewed Areas With No Findings

- [x] Worker-pool task submission and native pipeline serialization were not found to be the cause of the reported semantic failures.
- [x] Result revision stamping and function-key propagation are present on both success and fallback paths.

## Validation Results

- [x] Checked-in golden report and adapter source were inspected read-only.
- [x] Baseline validation recorded by the repository report: `NEW\build.bat all`, 53/53 before concurrent remediation.
- [x] Post-remediation integration run completed; all 10 selected decompilations are complete.

## Unresolved Questions And Residual Risks

- [ ] Native type/storage conversion rules for structured signatures and variables are not specified.
- [ ] CFG-complete range construction must be verified against adjacent-function and indirect-branch cases.
