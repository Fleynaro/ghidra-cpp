# Strict Audit Report: Decompiler Parameter ID

- [x] Scope confirmed: [`src/decompiler_parameter_id.cppm`](src/decompiler_parameter_id.cppm), [`tests/decompiler_parameter_id_tests.cppm`](tests/decompiler_parameter_id_tests.cppm), [`tests/data/run_ghidra.py`](tests/data/run_ghidra.py), [`tests/data/test_decompiler_parameter_id.md`](tests/data/test_decompiler_parameter_id.md), [`CMakeLists.txt`](CMakeLists.txt), and the checked-in PE/PDB fixtures.
- [x] Exact Ghidra sources inspected: [`DecompilerFunctionAnalyzer.java`](../../../../Ghidra/Features/Decompiler/src/main/java/ghidra/app/plugin/core/analysis/DecompilerFunctionAnalyzer.java) and [`DecompilerParameterIdCmd.java`](../../../../Ghidra/Features/Decompiler/src/main/java/ghidra/app/cmd/function/DecompilerParameterIdCmd.java).
- [x] Review date: 2026-09-15. No commit was supplied; the working tree was audited as-is.
- [x] Reviewer: Kilo.
- [x] Review method: source-to-source comparison, fixture assertion inspection, and CMake/test-registration inspection.
- [ ] Native build, test, formatter, and tidy commands were not run because this was requested as a strict read-only audit.

## Findings

### Critical

No findings.

### High

#### HIGH-001: Recovered parameters are never committed

- [ ] Remediation status: open.
- Severity: high.
- Title: `DecompilerParameterIdAnalyzer::analyze` marks completion after a C artifact but does not apply recovered parameters, types, return values, or storage.
- Exact references: [`src/decompiler_parameter_id.cppm:117-125`](src/decompiler_parameter_id.cppm#L117-L125), symbol `has_decompilation`; [`src/decompiler_parameter_id.cppm:135-150`](src/decompiler_parameter_id.cppm#L135-L150), symbol `DecompilerParameterIdAnalyzer::analyze`; Ghidra [`DecompilerParameterIdCmd.java:203-231`](../../../../Ghidra/Features/Decompiler/src/main/java/ghidra/app/cmd/function/DecompilerParameterIdCmd.java#L203-L231).
- Affected component: function signature recovery and every analyzer depending on parameter facts.
- Technical evidence: Ghidra rejects inconsistent `HighFunction` results and then calls either `HighFunctionDBUtil.commitParamsToDatabase` or `HighParamID.storeParametersToDatabase`, followed by `storeReturnToDatabase`. Native `has_decompilation` returns only `!result.c_source.empty()`, and `analyze` calls only `context.set_parameter_id_complete(entry)`.
- Expected behavior: a successful decompilation must commit the recovered parameter/return model according to the configured data-type and void-return options, and completion must mean that state was stored.
- Actual behavior: an untouched or pre-existing signature is marked complete solely because a C string was produced. No recovered parameter count, names, types, variable storage, return type, stack purge, or consistency check is applied.
- Impact: the analyzer reports success while providing none of the user-visible parameter-identification behavior. Downstream analyzers can skip the function because `parameter_id_complete` is true and never obtain the missing facts.
- Reproduction or failure scenario: start with a function whose native `Function.parameters` is empty and whose machine code has three recoverable inputs. Native analysis sets `parameter_id_complete` but leaves `parameters` empty; Java commits the recovered inputs.
- Root cause: the public native decompiler result does not expose `HighParamID`/`HighFunction` symbols, and the adapter chose a completion flag instead of implementing a structured commit boundary.
- Recommended fix: expose structured recovered parameters/return/storage plus consistency status from the frontend, then call the existing signature mutation API only after the Java-equivalent commit policy is applied.
- Regression risks: committing speculative values can overwrite trusted data unless source priority and the clear-level policy are modeled first.
- Relevant tests or validation: [`tests/decompiler_parameter_id_tests.cppm:11-23`](tests/decompiler_parameter_id_tests.cppm#L11-L23) asserts only the completion flag, so it would pass with no recovered parameters.

#### HIGH-002: Analyzer selection and option lifecycle are materially incomplete

- [ ] Remediation status: open.
- Severity: high.
- Title: native scheduling does not implement the Java one-time, dependency-graph, source-reset, or configuration contract.
- Exact references: [`src/decompiler_parameter_id.cppm:130-150`](src/decompiler_parameter_id.cppm#L130-L150); [`services/analyzers/shared/src/analyzer_types.cppm:315-363`](../shared/src/analyzer_types.cppm#L315-L363); Ghidra [`DecompilerFunctionAnalyzer.java:66-141`](../../../../Ghidra/Features/Decompiler/src/main/java/ghidra/app/plugin/core/analysis/DecompilerFunctionAnalyzer.java#L66-L141); Ghidra [`DecompilerParameterIdCmd.java:64-105`](../../../../Ghidra/Features/Decompiler/src/main/java/ghidra/app/cmd/function/DecompilerParameterIdCmd.java#L64-L105) and [`:122-166`](../../../../Ghidra/Features/Decompiler/src/main/java/ghidra/app/cmd/function/DecompilerParameterIdCmd.java#L122-L166).
- Affected component: analysis ordering, repeatability, and user options.
- Technical evidence: Java constructs an acyclic call graph, resets eligible source types while preserving external/glue functions, supports clear-level/commit-data-types/commit-void-return/timeout options, and is one-time analysis. Native triggers on every `function_added`/`function_changed`, scans every non-external incomplete function, has no thunk/glue/signature-source filter, and the shared options expose only a timeout plus the boolean feature switch.
- Expected behavior: the native descriptor and context must preserve the same one-time full-set semantics and option-controlled source/commit policy.
- Actual behavior: native analysis can run repeatedly on every function event, may process functions Java excludes, and cannot honor the Java commit options.
- Impact: order-dependent results, repeated expensive decompilation, and inability to reproduce valid Java configurations.
- Reproduction or failure scenario: emit a `function_changed` event for an unrelated function after a failed native decompilation. The native analyzer rescans all incomplete functions; Java's one-time command and failure handling do not behave this way.
- Root cause: lifecycle mapping was reduced to an event trigger and a completion bit.
- Recommended fix: add explicit analyzer one-time/full-set scheduling and model the missing source/commit options before claiming parity.
- Regression risks: scheduling changes affect dependent PDB, switch, and calling-convention analyzers.
- Relevant tests or validation: no current test changes options, tests source reset, tests a thunk/external/glue exclusion, or emits a repeat-analysis event.

### Medium

#### MEDIUM-001: The fixture asserts preservation of PDB setup, not Parameter ID recovery

- [ ] Remediation status: open.
- Severity: medium.
- Title: current test and Markdown evidence cannot distinguish a correct parameter pass from a no-op.
- Exact references: [`tests/decompiler_parameter_id_tests.cppm:11-37`](tests/decompiler_parameter_id_tests.cppm#L11-L37); [`tests/data/run_ghidra.py:182-197`](tests/data/run_ghidra.py#L182-L197); [`tests/data/test_decompiler_parameter_id.md:18-35`](tests/data/test_decompiler_parameter_id.md#L18-L35); [`CMakeLists.txt:7-13`](CMakeLists.txt#L7-L13).
- Affected component: test evidence and CI coverage.
- Technical evidence: the C++ tests assert only `parameter_id_complete` and that parameters remain equal when already complete. The PyGhidra script first runs PDB Universal, captures already meaningful parameters, then runs Parameter ID and requires those facts to remain meaningful; it invokes Ghidra Java through `project.analyze`, not the native analyzer. CMake registers only the C++ unit target and never runs the script.
- Expected behavior: a behavioral test should begin with missing or weak signature facts, assert exact native recovered parameters/types/storage/return behavior, and separately cover preservation of trusted facts.
- Actual behavior: the fixture passes if Parameter ID does nothing, and the generated report is evidence for Ghidra Java only.
- Impact: the principal missing behavior in `HIGH-001` is invisible to current tests.
- Reproduction or failure scenario: make native `has_decompilation` always return true but remove all future signature mutation; both C++ tests still pass and the PyGhidra report remains unchanged.
- Root cause: source-level fixture generation and native CTest execution are disconnected, while the setup intentionally pre-populates the expected output.
- Recommended fix: add native integration assertions for recovered state and register the reference script through the required PyGhidra wrapper if it remains part of CI.
- Regression risks: exact type/storage assertions need architecture-specific expected data and must not encode unstable display formatting.
- Relevant tests or validation: source inspection only; no commands were run in this audit.

#### MEDIUM-002: The native decompiler adapter is hard-coded to x86-64

- [ ] Remediation status: open.
- Severity: medium.
- Title: the analyzer cannot preserve Ghidra's language-independent P-code eligibility.
- Exact references: [`src/decompiler_parameter_id.cppm:79-107`](src/decompiler_parameter_id.cppm#L79-L107), symbol `architecture`; [`tests/data/run_ghidra.py:39-63`](tests/data/run_ghidra.py#L39-L63).
- Affected component: non-x86 P-code programs and architecture selection.
- Technical evidence: native code creates a fixed x86-64 register/space description and always loads the test context with `x86-64.sla`. Ghidra's `canAnalyze` checks `program.getLanguage().supportsPcode()` without selecting x86-specific facts.
- Expected behavior: the analyzer should obtain the program language/compiler architecture from the context and run on every supported P-Code language.
- Actual behavior: ARM or another P-Code language cannot be analyzed correctly and may be interpreted with x86-64 register facts.
- Impact: incorrect signatures or frontend failures outside the fixture architecture.
- Reproduction or failure scenario: provide an ARM context with P-Code and invoke the analyzer; the native adapter still installs RAX/RCX/RSP x86-64 facts.
- Root cause: `AnalysisContext` does not expose a language architecture object and the fixture adapter was made architecture-specific.
- Recommended fix: add an architecture provider to `AnalysisContext` and remove the x86-specific fallback from the analyzer.
- Regression risks: provider API changes affect all three decompiler analyzers.
- Relevant tests or validation: no non-x86 fixture exists.

### Low

No findings.

## Verified Strengths

- [x] Native analysis uses the production decompiler frontend and does not synthesize parameters from register names.
- [x] The existing completion setter is change-detected and emits a function-change event.
- [x] The PE/PDB fixture contains real parameter uses and is suitable for a future exact state assertion.

## Reviewed Areas With No Findings

- [x] Native CMake target and focused unit-test registration are internally consistent.
- [x] Fixture setup correctly separates PDB baseline preparation from the target Java analyzer phase; its limitation is that it does not exercise ReCode.

## Validation

- [x] Original Java analyzer/command, native implementation, tests, fixture script/report, and CMake registration were inspected.
- [ ] Focused build/test was not run by this audit.
- [ ] `format.bat analyzer` and `tidy.bat analyzer --check` were not run by this audit.
- [x] No implementation, test, fixture, or build source was edited.

## Unresolved Questions And Residual Risks

- [ ] The native decompiler API must expose recovered high variables and commit diagnostics before completion can have the Java meaning.
- [ ] The correct native representation of `SourceType`, clear level, and return commit policy remains unspecified.

## Follow-Up Decision

- [ ] Highest-priority open items are `HIGH-001` and `HIGH-002`; `MEDIUM-001` must be fixed before fixture reports are treated as native evidence.
