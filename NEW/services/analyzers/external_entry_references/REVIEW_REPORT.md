# Review Report: External Entry References

- [x] Scope confirmed: [`src/external_entry_references.cppm`](src/external_entry_references.cppm), [`tests/external_entry_references_tests.cppm`](tests/external_entry_references_tests.cppm), fixture source/script/report, [`CMakeLists.txt`](CMakeLists.txt), [`build.bat`](build.bat), [`README.md`](README.md), and [`GHIDRA_PORT.md`](GHIDRA_PORT.md).
- [x] Original source inspected: `Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/function/ExternalEntryFunctionAnalyzer.java`.
- [x] Reviewed diff: working tree at 2026-09-15; no commit supplied.
- [x] Reviewer: Kilo.
- [x] Source, test, fixture, documentation, CMake, and wrapper inspection completed.
- [x] Validation status recorded honestly: no build, CTest, formatter, or tidy command was run because this was a read-only audit.

## Findings

### Critical

No findings.

### High

#### EXTERNAL-ENTRY-HIGH-001: The source set and function-creation behavior are invented from PE exports

- [ ] Remediation status: open.
- Severity: high.
- Affected component: [`src/external_entry_references.cppm#L42-L80`](src/external_entry_references.cppm#L42-L80).
- Technical evidence: original `ExternalEntryFunctionAnalyzer.java#L55-L89` iterates `SymbolTable.getExternalEntryPointIterator()`, then `isGoodFunctionStart()` requires an instruction, rejects a fall-through destination, and rejects locations already represented by a function symbol before calling `AutoAnalysisManager.createFunction`. Native code instead iterates `image().exported_symbols()` at line 48, unconditionally adds an external entry at line 55, disassembles the export itself, and separately synthesizes an image-entry-point external entry at lines 71-79.
- Expected behavior: consume the program symbol table's already-established external-entry set and create only eligible functions; do not manufacture external-entry state from every export or the PE entry point.
- Actual behavior: exports and an entry point are treated as equivalent to original external-entry references.
- Impact: non-equivalent function and external-entry creation, especially for loader symbols, forwarded exports, non-code exports, and entry points not marked external by the symbol table.
- Reproduction or failure scenario: a PE with an executable entry point that is not in the external-entry iterator receives a native external entry/function but the original analyzer does not create it from that fact alone.
- Root cause: the native PE image model lacks the original symbol-table external-entry iterator, so the port substituted export metadata.
- Recommended fix: expose the symbol-table external-entry set and port `isGoodFunctionStart`/`createFunction` against it; keep export-derived behavior as an explicitly separate feature.
- Regression risks: loaders may provide duplicate export/entry addresses; test forwarded, data, fall-through, existing-function, and symbol-only cases.
- Relevant tests or validation: the fixture report lists four external entries but only three functions created; the native test intentionally asserts only three export entries at [`tests/external_entry_references_tests.cppm#L15-L31`](tests/external_entry_references_tests.cppm#L15-L31).

### Medium

#### EXTERNAL-ENTRY-MEDIUM-001: Eligibility side effects occur before the original checks

- [ ] Remediation status: open.
- Severity: medium.
- Affected component: [`src/external_entry_references.cppm#L52-L68`](src/external_entry_references.cppm#L52-L68).
- Technical evidence: native `add_external_entry` runs before executable, instruction-start, existing-function, and fall-through checks. Original `isGoodFunctionStart` performs those checks for function creation without using the analyzer to add new external-entry records.
- Expected behavior: ineligible addresses remain unchanged and existing loader/symbol-table state is preserved.
- Actual behavior: an export can become an external entry even when no function is created, and native code may disassemble a location as a side effect of analysis.
- Impact: downstream analyzers see invented external-entry events and changed code state.
- Root cause: the replacement conflates PE export discovery with the original analyzer's function-start consumer role.
- Recommended fix: make the external-entry set an input and keep eligibility/function creation side-effect-free until all original checks pass.
- Regression risks: event ordering can change; add tests that assert both function and external-entry state for every rejected case.
- Relevant tests or validation: no native test covers a rejected non-executable export or an already-existing external entry.

#### EXTERNAL-ENTRY-MEDIUM-002: Native test validity is export-only and omits the original symbol-table contract

- [ ] Remediation status: open.
- Severity: medium.
- Affected component: [`tests/external_entry_references_tests.cppm`](tests/external_entry_references_tests.cppm), fixture report, and [`CMakeLists.txt#L6-L13`](CMakeLists.txt#L6-L13).
- Technical evidence: CMake registers only GoogleTest. The PyGhidra report is generated from original Java analysis and records four external entries/three new functions, while native tests pre-disassemble three exports and assert the replacement's three-entry result. The fourth entry shown by the original report is not exercised natively.
- Expected behavior: tests must seed and assert the original external-entry iterator, existing function symbols, fall-through rejection, and exact created-function set.
- Actual behavior: a three-export approximation is treated as the native contract.
- Impact: native green tests cannot detect the core source-set mismatch.
- Root cause: no native symbol-table external-entry fixture API and no CTest integration for the PyGhidra oracle.
- Recommended fix: add a native external-entry model fixture and compare before/after state against the original report.
- Regression risks: loader-created entries and exports can have different source priorities; assert both state and events.
- Relevant tests or validation: no commands were run during this audit.

### Low

No findings.

## Verified Strengths

- [x] Executability, instruction presence, fall-through, duplicate-function, forwarding, and cancellation checks are explicit.
- [x] Module CMake and fixture build wrappers are present.

## Reviewed Areas With No Findings

- [x] No implementation, test, fixture, documentation, or build source was changed.
- [x] The original path and current module path are both traceable.

## Validation Results

- [x] Read-only original/native comparison completed.
- [x] Read-only test, fixture, documentation, CMake, and wrapper inspection completed.
- [ ] Focused build/test: not run by request.
- [ ] `NEW/format.bat external_entry_references`: not run by request.
- [ ] `NEW/tidy.bat external_entry_references --check`: not run by request.

## Unresolved Questions

- [ ] The native public representation of `SymbolTable.getExternalEntryPointIterator()` is not present in the reviewed boundary.

## Residual Risks

- [ ] Export-forwarding and loader-source precedence remain unverified in native integration.

## Follow-Up

- [ ] Highest proposed remediation: EXTERNAL-ENTRY-HIGH-001.
- [ ] Medium proposed remediations: EXTERNAL-ENTRY-MEDIUM-001 and EXTERNAL-ENTRY-MEDIUM-002.
- [ ] Final follow-up decision: keep implementation unchanged until the original input-set contract is authorized.
