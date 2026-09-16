# Strict Decompiler API Audit

## Scope

- [x] Reviewed [`src/decompiler.cppm`](src/decompiler.cppm), [`src/decompiler_impl.cppm`](src/decompiler_impl.cppm), the provider boundary, and the requested decompiler analyzer consumers.
- [x] Compared result and commit behavior with [`DecompilerParameterIdCmd.java`](../../../Ghidra/Features/Decompiler/src/main/java/ghidra/app/cmd/function/DecompilerParameterIdCmd.java) and [`DecompilerFunctionAnalyzer.java`](../../../Ghidra/Features/Decompiler/src/main/java/ghidra/app/plugin/core/analysis/DecompilerFunctionAnalyzer.java).
- [x] Reviewer: Kilo.
- [x] Review date: 2026-09-15.
- [x] Review type: strict read-only source audit; no implementation or configuration changes were made.
- [x] Recommendations target the public decompiler/result and commit boundaries, not analyzer-local workarounds.
- [x] Current core-contract boundary addendum reviewed; its detailed findings are recorded in [`../REVIEW_REPORT.md`](../REVIEW_REPORT.md) and [`../../core/contracts/REVIEW_REPORT.md`](../../core/contracts/REVIEW_REPORT.md).

## Findings

### Critical

No findings.

### High

#### DECOMP-HIGH-001: Decompiler Parameter ID reports completion without applying recovered parameters

- [ ] Remediation complete.
- Severity: high.
- Title: `DecompilerParameterIdAnalyzer` drops the structured decompiler result and sets only a boolean.
- Source: [`../analyzers/decompiler_parameter_id/src/decompiler_parameter_id.cppm:117-126`](../analyzers/decompiler_parameter_id/src/decompiler_parameter_id.cppm#L117-L126), [`../analyzers/decompiler_parameter_id/src/decompiler_parameter_id.cppm:135-150`](../analyzers/decompiler_parameter_id/src/decompiler_parameter_id.cppm#L135-L150), symbols `has_decompilation` and `DecompilerParameterIdAnalyzer::analyze`.
- Affected component: parameter ID, return recovery, variable storage, function signatures, and downstream calling-convention/stack analysis.
- Technical evidence: `has_decompilation` returns only `!result.c_source.empty()` and discards `DecompilationResult`; the analyzer then calls `set_parameter_id_complete(entry)` without modifying `Function.parameters`, `return_type`, or storage. Original `DecompilerParameterIdCmd` commits either `HighFunctionDBUtil.commitParamsToDatabase` or `HighParamID.storeParametersToDatabase` and `storeReturnToDatabase` at [`DecompilerParameterIdCmd.java:217-230`](../../../Ghidra/Features/Decompiler/src/main/java/ghidra/app/cmd/function/DecompilerParameterIdCmd.java#L217-L230).
- Expected behavior: successful parameter identification must atomically apply recovered parameter/return types and variable storage with the requested source/commit policy, then mark the function complete.
- Actual behavior: a nonempty C artifact marks the function complete while its previous empty or stale signature remains unchanged.
- Impact: later analyzers trust a false completion flag, skip parameter recovery, and operate on incorrect ABI information.
- Reproduction or failure scenario: run the analyzer on a function whose decompiler emits C but recovers one register parameter; `parameter_id_complete` becomes true while `function->parameters` remains empty.
- Root cause: the native decompiler exposes output text but no structured high-parameter result/commit transaction at the analyzer context boundary.
- Recommended fix: expose structured recovered parameters, return, local variables, source priority, and commit options from the decompiler result, then provide one shared transactional commit operation before changing the completion state.
- Regression risks: committing recovered data can alter call convention, stack, and Function ID scheduling; preserve user/imported signatures and make partial decompilation failures leave completion false.
- Relevant tests or validation: [`../analyzers/decompiler_parameter_id/tests/decompiler_parameter_id_tests.cppm:16-23`](../analyzers/decompiler_parameter_id/tests/decompiler_parameter_id_tests.cppm#L16-L23) asserts only the completion flag and does not assert a changed signature.

#### DECOMP-HIGH-002: `DecompilationResult` exposes only diagnostic text, not structured analysis artifacts

- [ ] Remediation complete.
- Severity: high.
- Title: The public result cannot represent high variables, recovered prototypes, CFG/switch structures, or commit-ready storage.
- Source: [`src/decompiler.cppm:522-531`](src/decompiler.cppm#L522-L531), symbols `DecompilationResult` and `Decompiler::decompile`.
- Affected component: all analyzers that need structured decompiler output, especially parameter ID and switch analysis.
- Technical evidence: the result contains raw instructions plus strings for raw p-code, high p-code, data flow, control flow, AST, and C source. It has no typed high variables, parameter/return records, Varnode storage pieces, function prototype, switch/jump-table model, source mappings, or error/status field.
- Expected behavior: the decompiler boundary must preserve the structured artifacts that the native engine computes and that Ghidra commands commit or use for follow-up analysis.
- Actual behavior: consumers can only test whether text is nonempty or parse implementation-specific diagnostic strings; structured state is unavailable after `decompile()` returns.
- Impact: analyzers cannot faithfully reproduce Ghidra side effects and any text parsing would be brittle, lossy, and analyzer-local.
- Reproduction or failure scenario: decompile a function with split parameter storage and a recovered switch; `DecompilationResult` has textual output but no typed representation from which an `AnalysisContext` transaction could apply either result.
- Root cause: the public API was designed as a rendering result even though requested analyzers require a semantic result/commit boundary.
- Recommended fix: add typed result records for prototype, variables, storage pieces, CFG/switch edges, source mappings, and analysis status; expose a model-level application transaction rather than requiring consumers to parse strings.
- Regression risks: result lifetime and ownership must remain independent of native decompiler state; keep raw textual artifacts for CLI/debug use while making structured records authoritative.
- Relevant tests or validation: current decompiler tests cover text artifacts and provider wiring but no structured result round trip into the analyzer context.

### Medium

#### DECOMP-MEDIUM-001: Provider prototype richness is not connected to the analyzer function model

- [ ] Remediation complete.
- Severity: medium.
- Title: The decompiler accepts structured prototypes, but the analyzer context has no corresponding import/export adapter.
- Source: [`src/decompiler.cppm:255-295`](src/decompiler.cppm#L255-L295), [`src/decompiler.cppm:446-462`](src/decompiler.cppm#L446-L462), [`../analyzers/decompiler_parameter_id/src/decompiler_parameter_id.cppm:79-107`](../analyzers/decompiler_parameter_id/src/decompiler_parameter_id.cppm#L79-L107), symbols `PrototypeDescription`, `ProviderContext`, and `architecture`.
- Affected component: PDB/DIA prototypes, calling conventions, hidden structure returns, call fixups, and decompiler-to-program state transfer.
- Technical evidence: `PrototypeDescription` supports return storage, split pieces, no-return, inline, call-fixup, and hidden-return storage, but the parameter analyzer constructs only an architecture and raw P-code provider. No prototype, symbol, type, variable, flow, comment, or injection provider is connected from `AnalysisContext`.
- Expected behavior: decompilation used by analysis should see the current program's symbols, types, prototypes, variables, comments, and flow corrections, and its structured result should be written back with the same identities.
- Actual behavior: the analyzer runs a minimally bootstrapped decompiler detached from existing function signatures and PDB/type state, then discards the result.
- Impact: decompiler output is not equivalent to the same function analyzed inside Ghidra's program-backed decompiler and cannot account for known signatures or flow corrections.
- Reproduction or failure scenario: apply a PDB signature or call-fixup, run parameter ID, and inspect the provider context; no `PrototypeProvider`, `TypeProvider`, `SymbolProvider`, `VariableProvider`, or `FlowProvider` is installed.
- Root cause: provider contracts were implemented independently from the shared program model.
- Recommended fix: create a shared bidirectional provider adapter and structured commit transaction, with explicit precedence between imported, analysis, and user data.
- Regression risks: provider reads must be immutable during a decompile transaction, and commits must emit scheduler events only after the whole result is validated.
- Relevant tests or validation: no current integration test verifies decompiler behavior after a PDB signature, type archive, or flow override is present.

### Low

No findings.

## Verified Strengths

- [x] The public decompiler provider interfaces are separated from native implementation classes.
- [x] The frontend retains raw p-code and textual output for diagnostics.
- [x] Prototype descriptions explicitly document split storage and hidden-return fields, which provides a useful source contract for a future commit boundary.
- [x] Decompiler CLI and engine build/test targets are registered in the preserved CTest build.

## Reviewed Areas With No Findings

- [x] Move-only ownership of `Decompiler` and `SleighPcodeProvider`.
- [x] Memory provider exact-range failure propagation.
- [x] Provider opcode/storage conversion for the covered fixtures.

## Validation

- [x] `git diff --check` completed without whitespace errors.
- [x] `ctest --test-dir NEW/build -N` listed decompiler engine, architecture, CLI, and parameter-ID tests.
- [x] `NEW\\build.bat all` completed successfully with 49/49 tests passing; tidy checks were limited by MSVC IFC parsing.

## Unresolved Questions And Residual Risks

- [ ] The exact source-priority and commit policy for analysis, imported/PDB, and user signatures must be shared by the decompiler, PDB, and analyzer context.
- [ ] Native high-function structures are internal to the current decompiler implementation; the public ownership/lifetime contract for exposing them is not defined.

## Follow-Up Decision

- [x] Architecture propagation, raw decoder boundary, memory-space selection, volatile ranges, option gating, and checked function-range handling were fixed in the authorized follow-up.
- [ ] Remaining decompiler work: native structured result projection and source-priority policy.
