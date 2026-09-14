# Strict Audit Report: Call Convention ID

- [x] Scope confirmed: [`src/call_convention_id.cppm`](src/call_convention_id.cppm), [`tests/call_convention_id_tests.cppm`](tests/call_convention_id_tests.cppm), [`tests/data/run_ghidra.py`](tests/data/run_ghidra.py), [`tests/data/test_call_convention_id.md`](tests/data/test_call_convention_id.md), [`CMakeLists.txt`](CMakeLists.txt), and the checked-in PE fixture.
- [x] Exact Ghidra sources inspected: [`DecompilerCallConventionAnalyzer.java`](../../../../Ghidra/Features/Decompiler/src/main/java/ghidra/app/plugin/core/analysis/DecompilerCallConventionAnalyzer.java) and [`DecompilerParallelConventionAnalysisCmd.java`](../../../../Ghidra/Features/Decompiler/src/main/java/ghidra/app/cmd/function/DecompilerParallelConventionAnalysisCmd.java).
- [x] Review date: 2026-09-15. No commit was supplied; the working tree was audited as-is.
- [x] Reviewer: Kilo.
- [x] Review method: source-to-source comparison, fixture assertion inspection, and CMake/test-registration inspection.
- [ ] Native build, test, formatter, and tidy commands were not run because this was requested as a strict read-only audit.

## Findings

### Critical

No findings.

### High

#### HIGH-001: Eligibility and signature-state contract is not ported

- [ ] Remediation status: open.
- Severity: high.
- Title: `CallConventionIdAnalyzer::analyze` analyzes and commits a materially broader set of functions than Ghidra.
- Exact references: [`src/call_convention_id.cppm:228-251`](src/call_convention_id.cppm#L228-L251), symbols `CallConventionIdAnalyzer::analyze` and `identify_calling_convention`; Ghidra [`DecompilerCallConventionAnalyzer.java:172-243`](../../../../Ghidra/Features/Decompiler/src/main/java/ghidra/app/plugin/core/analysis/DecompilerCallConventionAnalyzer.java#L172-L243); Ghidra [`DecompilerParallelConventionAnalysisCmd.java:129-201`](../../../../Ghidra/Features/Decompiler/src/main/java/ghidra/app/cmd/function/DecompilerParallelConventionAnalysisCmd.java#L129-L201).
- Affected component: calling-convention identification and repeat analysis.
- Technical evidence: Ghidra requires a P-code language with multiple conventions, skips thunks, inline functions, external functions, call-fixup functions, custom storage, known conventions, and functions without a non-default namespace signature or defined parameter types. The command temporarily resets the signature source, analyzes with the `paramid` decompiler, updates only the convention, and restores the original source in `finally`; failures create a warning bookmark. The native analyzer only checks `external`, `signature_committed`, and an empty/`default` convention, then calls `set_function_signature(..., true)`.
- Expected behavior: only the Java eligibility set should be analyzed; convention recovery must preserve all pre-existing signature-source state and must be retryable or warning-marked according to the command contract.
- Actual behavior: default/unknown functions outside the Java predicate are analyzed, and a successful native call commits the entire signature state. A failed or convention-less run does not create the Java warning marker and leaves the function eligible for repeated event-driven attempts.
- Impact: unsupported or user-prepared functions can be changed, and repeated `function_changed` events can cause non-deterministic or unnecessarily expensive re-analysis.
- Reproduction or failure scenario: create a non-external function with no defined parameter types and an uncommitted `default` convention. Native `analyze` decompiles it, while Ghidra `findLocations` excludes it. Trigger a later function event after a failed decompilation; native retries because no failure bookmark exists.
- Root cause: the native `AnalysisContext` adapter reduced Ghidra's location filtering, source restoration, and failure bookkeeping to a boolean `signature_committed` gate.
- Recommended fix: model the missing eligibility/source/failure state or explicitly make the native contract equivalent before applying a recovered convention; add tests for every excluded function class and retry behavior.
- Regression risks: changing selection and source-state transitions will affect analyzer ordering and dependent signature analyzers.
- Relevant tests or validation: [`tests/call_convention_id_tests.cppm:19-34`](tests/call_convention_id_tests.cppm#L19-L34) does not exercise any exclusion predicate or failure path.

### Medium

#### MEDIUM-001: Convention extraction accepts arbitrary printer text

- [ ] Remediation status: open.
- Severity: medium.
- Title: `identify_calling_convention` searches the complete C artifact rather than the recovered prototype model.
- Exact references: [`src/call_convention_id.cppm:215-220`](src/call_convention_id.cppm#L215-L220), symbol `identify_calling_convention`.
- Affected component: convention selection after decompilation.
- Technical evidence: the native implementation returns the first matching token anywhere in `c_source`. Ghidra reads `highFunction.getFunctionPrototype().getModelName()` in [`DecompilerParallelConventionAnalysisCmd.java:175-183`](../../../../Ghidra/Features/Decompiler/src/main/java/ghidra/app/cmd/function/DecompilerParallelConventionAnalysisCmd.java#L175-L183), then applies the model-specific update rules at lines 204-238.
- Expected behavior: only the decompiler's prototype model should determine the convention, including x86 `stdcall`/`cdecl` correction and namespace-based `thiscall` handling.
- Actual behavior: a token in a comment, string literal, called symbol name, or unrelated prototype text can be interpreted as the current function's convention; model-specific corrections and namespace conversion are absent.
- Impact: false convention changes are possible on otherwise valid decompilation output.
- Reproduction or failure scenario: pass C text containing `"__stdcall"` in a body or a called symbol with that substring and no such prototype convention. The native helper returns `__stdcall`.
- Root cause: a text-boundary adapter was used instead of exposing the frontend's recovered prototype model.
- Recommended fix: consume a structured prototype/model result and retain only the convention attached to the analyzed function.
- Regression risks: printer-format changes should no longer alter behavior when the structured model is unchanged.
- Relevant tests or validation: [`tests/call_convention_id_tests.cppm:11-16`](tests/call_convention_id_tests.cppm#L11-L16) covers only one positive token and one token-free negative string.

#### MEDIUM-002: The native analyzer has no meaningful fixture assertion

- [ ] Remediation status: open.
- Severity: medium.
- Title: the C++ test accepts either the unchanged result or `__fastcall`, and the PyGhidra fixture does not execute `NEW`.
- Exact references: [`tests/call_convention_id_tests.cppm:19-34`](tests/call_convention_id_tests.cppm#L19-L34), especially line 32; [`tests/data/run_ghidra.py:147-173`](tests/data/run_ghidra.py#L147-L173); [`tests/data/test_call_convention_id.md:21-46`](tests/data/test_call_convention_id.md#L21-L46); [`CMakeLists.txt:7-13`](CMakeLists.txt#L7-L13).
- Affected component: regression infrastructure and claimed behavioral evidence.
- Technical evidence: line 32 asserts `calling_convention == "default" || calling_convention == "__fastcall"`, so a no-op passes. `run_ghidra.py` imports `pyghidra`, imports the program into Ghidra, and calls `project.analyze`; it never loads a native library or invokes the C++ analyzer. The CMake file registers only `analyzer_call_convention_id_tests`, not the script or Markdown report.
- Expected behavior: a parity test must assert the native postcondition exactly and must execute the native implementation or explicitly mark the artifact as Java-only evidence.
- Actual behavior: the report proves that the Java analyzer changed the prepared Ghidra function to `__fastcall`, while CTest can pass without proving any native state change.
- Impact: regressions in native call-convention recovery can remain green and the checked-in Markdown can become stale without CI detection.
- Reproduction or failure scenario: remove the native analyzer body or make it a no-op; the current C++ test still passes if the fixture remains `default`.
- Root cause: fixture generation and native unit testing are disconnected, and the test assertion was intentionally permissive.
- Recommended fix: add an exact native integration assertion and register the fixture workflow through the repository's required PyGhidra wrapper if Java reference evidence is retained.
- Regression risks: stricter assertions must distinguish a valid no-convention frontend result from a recovered convention.
- Relevant tests or validation: source inspection only; no commands were run in this read-only audit.

### Low

No findings.

## Verified Strengths

- [x] The native implementation uses the existing decompiler frontend instead of an ABI register heuristic.
- [x] The explicit-token helper covers the five convention spellings used by the native printer.
- [x] The checked-in fixture has a real exported function with defined integer parameters and a call edge matching the Java setup predicate.

## Reviewed Areas With No Findings

- [x] CMake target dependency and native unit-test registration are internally consistent.
- [x] The PE fixture is suitable for observing a calling-convention change; its limitation is assertion/wiring, not fixture reachability.

## Validation

- [x] Original Java analyzer, command, native implementation, native tests, fixture script, report, and CMake registration were inspected.
- [ ] Focused build/test was not run by this audit.
- [ ] `NEW\format.bat analyzer` and `NEW\tidy.bat analyzer --check` were not run by this audit.
- [x] No implementation, test, fixture, or build source was edited.

## Unresolved Questions And Residual Risks

- [ ] The native model needs a structured decompiler prototype and signature-source/failure-bookmark representation before exact command parity can be claimed.
- [ ] The current report records Java behavior, not native behavior.

## Follow-Up Decision

- [ ] Highest-priority open item is `HIGH-001`; `MEDIUM-001` and `MEDIUM-002` should be remediated before this analyzer is described as behaviorally equivalent.
