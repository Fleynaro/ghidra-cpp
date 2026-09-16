# Strict Audit Report: Decompiler Switch Analysis

- [x] Scope confirmed: [`src/decompiler_switch_analysis.cppm`](src/decompiler_switch_analysis.cppm), [`tests/decompiler_switch_analysis_tests.cppm`](tests/decompiler_switch_analysis_tests.cppm), [`tests/data/run_ghidra.py`](tests/data/run_ghidra.py), [`tests/data/test_decompiler_switch_analysis.md`](tests/data/test_decompiler_switch_analysis.md), [`CMakeLists.txt`](CMakeLists.txt), and the checked-in PE fixture.
- [x] Exact Ghidra sources inspected: [`DecompilerSwitchAnalyzer.java`](../../../../Ghidra/Features/Decompiler/src/main/java/ghidra/app/plugin/core/analysis/DecompilerSwitchAnalyzer.java) and [`DecompilerSwitchAnalysisCmd.java`](../../../../Ghidra/Features/Decompiler/src/main/java/ghidra/app/cmd/function/DecompilerSwitchAnalysisCmd.java).
- [x] Review date: 2026-09-15. No commit was supplied; the working tree was audited as-is.
- [x] Reviewer: Kilo.
- [x] Review method: source-to-source comparison, fixture assertion inspection, and CMake/test-registration inspection.
- [ ] Native build, test, formatter, and tidy commands were not run because this was requested as a strict read-only audit.

## Findings

### Critical

No findings.

### High

#### HIGH-001: JumpTable recovery is replaced by a false-positive-prone heuristic

- [ ] Remediation status: open.
- Severity: high.
- Title: `switch_targets` does not consume a recovered `JumpTable` and can label unrelated return blocks as switch cases.
- Exact references: [`src/decompiler_switch_analysis.cppm:131-176`](src/decompiler_switch_analysis.cppm#L131-L176), symbol `switch_targets`; [`src/decompiler_switch_analysis.cppm:218-257`](src/decompiler_switch_analysis.cppm#L218-L257), symbol `DecompilerSwitchAnalysisAnalyzer::analyze`; Ghidra [`DecompilerSwitchAnalysisCmd.java:98-137`](../../../../Ghidra/Features/Decompiler/src/main/java/ghidra/app/cmd/function/DecompilerSwitchAnalysisCmd.java#L98-L137), [`:165-218`](../../../../Ghidra/Features/Decompiler/src/main/java/ghidra/app/cmd/function/DecompilerSwitchAnalysisCmd.java#L165-L218), and [`:308-487`](../../../../Ghidra/Features/Decompiler/src/main/java/ghidra/app/cmd/function/DecompilerSwitchAnalysisCmd.java#L308-L487).
- Affected component: computed-jump references, switch labels, case disassembly, and function bodies.
- Technical evidence: Ghidra iterates `HighFunction.getJumpTables()`, verifies ownership and existing references, handles default-case exclusion, applies switch target context, disassembles every case, labels the switch namespace/cases/load tables, creates data/reference labels, and repairs the owning function body. Native detects the word `switch` in printed C, counts textual `case ` occurrences, collects any native control-flow block address, then scans every address after the branch for a decodable instruction followed by a return and treats those addresses as cases.
- Expected behavior: only addresses in the frontend's recovered jump-table cases should become computed-jump references and labels; default cases, load tables, data, and body repair must follow the Java command contract.
- Actual behavior: a return-shaped block elsewhere in the executable can be selected as a case, while valid sparse/indirect/multi-stage tables, default-case rules, load-table labels, and namespace/case labels are not represented.
- Impact: incorrect control-flow edges can change disassembly, function bodies, and later analysis; missing table data leaves the listing observably different from Ghidra.
- Reproduction or failure scenario: place an unrelated two-instruction return block after an indirect branch in an executable whose decompiler prints a switch. The fallback at lines 157-167 can add that block as a switch target even if it is not in the `JumpTable`.
- Root cause: the public native result exposes text/raw instructions but no structured `JumpTable`; the implementation substitutes address scanning instead of adding that boundary.
- Recommended fix: expose structured jump-table objects, case values, load tables, and switch ownership from the frontend, then port `DecompilerSwitchAnalysisCmd` state transitions directly.
- Regression risks: newly precise targets may remove references currently produced by the heuristic; dependent body and disassembly tests must be updated together.
- Relevant tests or validation: [`tests/decompiler_switch_analysis_tests.cppm:21-40`](tests/decompiler_switch_analysis_tests.cppm#L21-L40) checks one dense table's eight target addresses only.

### Medium

#### MEDIUM-001: Location discovery and one-time lifecycle are not equivalent

- [ ] Remediation status: open.
- Severity: medium.
- Title: native analysis ignores the Java location-selection, call-other, function-selection, and restart workflow.
- Exact references: [`src/decompiler_switch_analysis.cppm:198-260`](src/decompiler_switch_analysis.cppm#L198-L260); Ghidra [`DecompilerSwitchAnalyzer.java:70-151`](../../../../Ghidra/Features/Decompiler/src/main/java/ghidra/app/plugin/core/analysis/DecompilerSwitchAnalyzer.java#L70-L151), [`:237-375`](../../../../Ghidra/Features/Decompiler/src/main/java/ghidra/app/plugin/core/analysis/DecompilerSwitchAnalyzer.java#L237-L375), and [`:407-447`](../../../../Ghidra/Features/Decompiler/src/main/java/ghidra/app/plugin/core/analysis/DecompilerSwitchAnalyzer.java#L407-L447).
- Affected component: analyzer scheduling and switch candidates.
- Technical evidence: Java first scans the supplied address set for computed jumps or call-fixup cases, rejects unrecoverable `CALLOTHER` flows, filters existing computed references, resolves defined/undefined owning functions, and schedules a one-time restart for non-returning thunks. Native triggers on every function add/change and decompiles every non-external function not marked recovered.
- Expected behavior: only candidate indirect-flow locations in the event set should be processed, and restart/one-time semantics should be preserved.
- Actual behavior: unrelated functions are repeatedly decompiled, and a failed or unsupported function remains eligible without Java's restart/bookmark state.
- Impact: unnecessary work and different results when function bodies or references are still changing.
- Reproduction or failure scenario: add a normal function with no indirect branch; native still calls the frontend because it scans all functions.
- Root cause: the event model lacks the Java address-set candidate boundary and the analyzer descriptor has no one-time state.
- Recommended fix: carry candidate addresses in events, gate on computed flow/call-fixup metadata, and implement a one-time/restart state.
- Regression risks: event filtering can change analyzer ordering and must be tested with newly discovered case code.
- Relevant tests or validation: no current native test exercises a no-switch function, a `CALLOTHER`, a non-returning thunk, or repeat analysis.

#### MEDIUM-002: The fixture does not test the declared negative and label contracts

- [ ] Remediation status: open.
- Severity: medium.
- Title: current native tests prove only target references for one positive table.
- Exact references: [`tests/decompiler_switch_analysis_tests.cppm:21-40`](tests/decompiler_switch_analysis_tests.cppm#L21-L40); fixture source [`tests/data/test_decompiler_switch_analysis.cpp:62-74`](tests/data/test_decompiler_switch_analysis.cpp#L62-L74); fixture script [`tests/data/run_ghidra.py:62-73`](tests/data/run_ghidra.py#L62-L73); report [`tests/data/test_decompiler_switch_analysis.md:23-54`](tests/data/test_decompiler_switch_analysis.md#L23-L54); [`CMakeLists.txt:7-13`](CMakeLists.txt#L7-L13).
- Affected component: regression evidence.
- Technical evidence: the C++ test never asserts the `switchD` symbol, default-case exclusion, case labels/namespaces, load-table data, function-body extension, or that `switch_fixture_negative` remains unrecovered. The PyGhidra script runs Ghidra Java through `project.analyze` and extracts only Java listing facts; CMake registers no script/report test.
- Expected behavior: a meaningful parity suite must assert both the dense positive and sparse/ordinary negative fixture on the native context, plus the observable label/reference/body invariants.
- Actual behavior: a heuristic could pass the current eight-reference assertion while still producing invalid extra labels or incorrectly recovering the negative function.
- Impact: false positives and missing Java-side effects are untested.
- Reproduction or failure scenario: add an invalid `switchD` symbol or recover the negative function; the current C++ test remains green if the eight expected references are still present.
- Root cause: the test was narrowed to stable references and the Java reference fixture is not wired to native execution.
- Recommended fix: add native assertions for all stable side effects and register/clearly classify the Java fixture workflow.
- Regression risks: namespace and body representations must be normalized before exact assertions are added.
- Relevant tests or validation: source inspection only; no commands were run in this audit.

#### MEDIUM-003: The decompiler adapter is hard-coded to x86-64

- [ ] Remediation status: open.
- Severity: medium.
- Title: the analyzer cannot preserve Ghidra's P-Code-language coverage.
- Exact references: [`src/decompiler_switch_analysis.cppm:82-106`](src/decompiler_switch_analysis.cppm#L82-L106), symbol `architecture`; Ghidra [`DecompilerSwitchAnalyzer.java:74-77`](../../../../Ghidra/Features/Decompiler/src/main/java/ghidra/app/plugin/core/analysis/DecompilerSwitchAnalyzer.java#L74-L77).
- Affected component: non-x86 switch analysis.
- Technical evidence: native constructs a fixed x86-64 register/space description while Java accepts any language with P-Code support.
- Expected behavior: architecture/compiler facts should come from the analyzed program.
- Actual behavior: another P-Code language is interpreted with x86-64 facts or fails to recover.
- Impact: unsupported architectures cannot reach equivalent behavior.
- Reproduction or failure scenario: pass an ARM P-Code context to the native analyzer; it still installs RAX/RSP/R8-style registers.
- Root cause: no architecture provider is exposed by the current `AnalysisContext`.
- Recommended fix: add a language-specific architecture provider and a non-x86 fixture.
- Regression risks: provider changes affect all decompiler-backed analyzers.
- Relevant tests or validation: no non-x86 fixture exists.

### Low

No findings.

## Verified Strengths

- [x] The native analyzer requires native decompiler output containing a switch and an indirect branch before it creates references.
- [x] Reference insertion uses the existing analysis-source computed-jump model and bounds candidates to decoded executable instructions.
- [x] The dense PE fixture reliably exposes a real indirect dispatch and eight case blocks.

## Reviewed Areas With No Findings

- [x] The local CMake target and GoogleTest registration are internally consistent.
- [x] The control-flow address parser has deterministic sorting/deduplication behavior for its narrow text boundary.

## Validation

- [x] Original Java analyzer/command, native implementation, tests, fixture source/script/report, and CMake registration were inspected.
- [ ] Focused build/test was not run by this audit.
- [ ] `NEW\format.bat analyzer` and `NEW\tidy.bat analyzer --check` were not run by this audit.
- [x] No implementation, test, fixture, or build source was edited.

## Unresolved Questions And Residual Risks

- [ ] A structured native `JumpTable` API is the main prerequisite for exact parity; text parsing cannot preserve load-table and case-value semantics.
- [ ] The current native report does not include function-body or data-table state.

## Follow-Up Decision

- [ ] Highest-priority open item is `HIGH-001`; `MEDIUM-001` and `MEDIUM-002` must be addressed before switch recovery is considered equivalent.
