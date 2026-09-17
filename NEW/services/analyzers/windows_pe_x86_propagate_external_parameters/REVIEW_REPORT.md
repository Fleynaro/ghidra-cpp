# Review Report: Windows PE x86 External Parameters

- [x] Scope confirmed: [`src/params.cppm`](src/params.cppm), [`tests/params_tests.cppm`](tests/params_tests.cppm), fixture source/PDB/script/report, [`CMakeLists.txt`](CMakeLists.txt), [`build.bat`](build.bat), [`README.md`](README.md), and [`GHIDRA_PORT.md`](GHIDRA_PORT.md).
- [x] Original sources inspected: `Ghidra/Features/MicrosoftCodeAnalyzer/src/main/java/ghidra/app/plugin/prototype/MicrosoftCodeAnalyzerPlugin/PropagateExternalParametersAnalyzer.java` and `PEUtil.java`.
- [x] Reviewed diff: working tree at 2026-09-15; no commit supplied.
- [x] Reviewer: Kilo.
- [x] Source, test, fixture, documentation, CMake, and wrapper inspection completed.
- [x] Validation status recorded honestly: no build, CTest, formatter, or tidy command was run because this was a read-only audit.

## Findings

### Critical

No findings.

### High

#### EXTERNAL-PARAM-HIGH-001: Required comments, labels, data creation, and references are not applied

- [ ] Remediation status: open.
- Severity: high.
- Affected component: [`src/params.cppm#L38-L47`](src/params.cppm#L38-L47) and [`#L85-L89`](src/params.cppm#L85-L89).
- Technical evidence: original `PropagateExternalParametersAnalyzer.java#L267-L307` consumes `PushedParamInfo` results, adds address symbols, creates plate comments, clears undefined data, creates parameter-typed data, and updates local comments/data. Native `record_parameter_evidence` only adds a `SymbolRecord` and optional bookmark; it cannot set EOL/plate comments, create typed data, clear undefined data, or add original data references.
- Expected behavior: push sites receive parameter comments; referenced data receives parameter labels/comments and compatible data types without overwriting defined data.
- Actual behavior: only an evidence record/bookmark is produced at the push address.
- Impact: the analyzer's user-visible purpose is absent even when parameter matching succeeds.
- Root cause: the native context lacks comment, listing-data, and parameter-data APIs and the port substitutes evidence bookkeeping.
- Recommended fix: expose the original listing/comment/data operations and port `PushedParamInfo`, address-reference, type-length, and no-overwrite behavior.
- Regression risks: data creation can conflict with loader-defined data and affect later analyzers; test undefined, defined, pointer, and typed targets.
- Relevant tests or validation: native tests assert evidence/bookmarks, while the original fixture report asserts four EOL comments at [`tests/data/test_windows_pe_x86_propagate_external_parameters.md#L60-L67`](tests/data/test_windows_pe_x86_propagate_external_parameters.md#L60-L67).

#### EXTERNAL-PARAM-HIGH-002: Push matching omits original thunk, nested-call, branch, and reference semantics

- [ ] Remediation status: open.
- Severity: high.
- Affected component: [`src/params.cppm#L50-L89`](src/params.cppm#L50-L89).
- Technical evidence: original `PropagateExternalParametersAnalyzer.java#L52-L100` distinguishes direct `CALL` from thunk `JMP`, follows references to thunk callers, and uses `hasEnoughPushes`/`propogateParams` at `#L103-L210` to skip pushes belonging to nested calls and stop/adjust at branches. Native collects every `PUSH` before the reference and takes the last N; it also accepts `ReferenceKind::external` at line 53, whereas original direct processing checks the call mnemonic and `RefType.isCall()` path.
- Expected behavior: select pushes belonging to the target call in the current/thunk caller, skip nested-call arguments, respect branch boundaries, and preserve original parameter order.
- Actual behavior: unrelated pushes in the same function or fallback instruction history can be attributed to the target external function.
- Impact: comments/evidence can label the wrong stack arguments, especially in realistic x86 call sequences.
- Root cause: bounded history was used instead of the original code-unit/call-aware algorithm.
- Recommended fix: port exact call/thunk traversal and nested-call skip accounting, then use instruction/reference metadata rather than all prior pushes.
- Regression risks: stack order and branch behavior are architecture-sensitive; add nested call, thunk, branch, insufficient-push, and interleaved-call fixtures.
- Relevant tests or validation: current fixture has one simple push sequence and does not exercise the original skip/thunk cases.

### Medium

#### EXTERNAL-PARAM-MEDIUM-001: Eligibility and lifecycle contract are only approximate

- [ ] Remediation status: open.
- Severity: medium.
- Affected component: [`src/params.cppm#L93-L123`](src/params.cppm#L93-L123).
- Technical evidence: original uses `PEUtil.canAnalyze` for PE/binary-PE identification and a `BYTE_ANALYZER` at `DATA_TYPE_PROPOGATION.after()` (`PropagateExternalParametersAnalyzer.java#L44-L50`). Native gates only COFF machine IDs (i386/amd64) and triggers on code/external/function changes. The original iterates symbol-table external symbols and their references at `#L241-L265`; native searches `external_symbols()` and requires a native function exactly at the IAT address.
- Expected behavior: run for eligible PE programs independent of a narrow machine shortcut and consume external function/reference state as the original model supplies it.
- Actual behavior: a valid PE with a missing native IAT function object is silently skipped, and the lifecycle is not the original byte-analysis contract.
- Impact: valid imported functions can receive no propagation even when signatures/references exist in the loader model.
- Root cause: native external-function and analyzer scheduling boundaries are narrower than Ghidra's listing/symbol APIs.
- Recommended fix: port PE eligibility and external-function/reference lookup semantics, then align descriptor/lifecycle events.
- Regression risks: enabling more PE cases can affect x64 behavior; test PE/binary-PE and machine variants explicitly.
- Relevant tests or validation: no native scheduler/PE eligibility test covers this distinction.

#### EXTERNAL-PARAM-MEDIUM-002: Native tests validate evidence, not the original fixture result

- [ ] Remediation status: open.
- Severity: medium.
- Affected component: [`tests/params_tests.cppm`](tests/params_tests.cppm), fixture script/report, and [`CMakeLists.txt#L12-L19`](CMakeLists.txt#L12-L19).
- Technical evidence: CMake registers the native test only. The native test seeds a synthetic external function and checks four `external_parameter` records; the PyGhidra report observes original EOL comments and no signature/resource/reference delta. The x86 fixture build explicitly uses `VsDevCmd -arch=x86` at [`tests/data/build.bat#L4-L26`](tests/data/build.bat#L4-L26), while shared native support loads every fixture with `x86-64.sla` at [`../shared/test_support/analyzer_test_support.cppm#L20-L32`](../shared/test_support/analyzer_test_support.cppm#L20-L32).
- Expected behavior: native integration should decode the actual x86 fixture with the x86 language and assert original listing mutations.
- Actual behavior: native tests exercise a synthetic evidence boundary and do not validate actual x86 decoding or original output.
- Impact: green tests cannot establish x86 analyzer parity.
- Root cause: fixture reports and native model tests are disconnected, and shared loader language is hard-coded to x86-64.
- Recommended fix: add an x86 language/profile path and native comment/data APIs, then compare exact before/after state.
- Regression risks: separate language loading can affect all fixture helpers; keep x64 helper behavior unchanged.
- Relevant tests or validation: no commands were run during this audit.

### Low

No findings.

## Verified Strengths

- [x] Push detection, parameter-count sufficiency, cancellation, and direct external-symbol iteration are explicit.
- [x] The x86 fixture source, PDB, build wrapper, script, and report are present.

## Reviewed Areas With No Findings

- [x] No implementation, test, fixture, documentation, or build source was changed.
- [x] The CMake alias/test-name arrangement was inspected.

## Validation Results

- [x] Read-only original/native comparison completed.
- [x] Read-only test, fixture, documentation, CMake, and wrapper inspection completed.
- [ ] Focused build/test: not run by request.
- [ ] `format.bat windows_pe_x86_propagate_external_parameters`: not run by request.
- [ ] `tidy.bat windows_pe_x86_propagate_external_parameters --check`: not run by request.

## Unresolved Questions

- [ ] Native comment, listing-data, and x86-language APIs needed for full parity are not exposed in this module boundary.

## Residual Risks

- [ ] The fallback scan can misattribute pushes when unrelated calls precede the target call.

## Follow-Up

- [ ] Highest proposed remediations: EXTERNAL-PARAM-HIGH-001 and EXTERNAL-PARAM-HIGH-002.
- [ ] Medium proposed remediations: EXTERNAL-PARAM-MEDIUM-001 and EXTERNAL-PARAM-MEDIUM-002.
- [ ] Final follow-up decision: keep implementation unchanged until listing mutation and x86 integration parity is authorized.
