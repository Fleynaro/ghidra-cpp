# Review Report: Apply Data Archives

- [x] Scope confirmed: [`src/apply_data_archives.cppm`](src/apply_data_archives.cppm), [`tests/apply_data_archives_tests.cppm`](tests/apply_data_archives_tests.cppm), fixture source/script/report, [`CMakeLists.txt`](CMakeLists.txt), [`build.bat`](build.bat), [`README.md`](README.md), and [`GHIDRA_PORT.md`](GHIDRA_PORT.md).
- [x] Original source inspected: `Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/analysis/ApplyDataArchiveAnalyzer.java`.
- [x] Reviewed diff: working tree at 2026-09-15; no commit supplied.
- [x] Reviewer: Kilo.
- [x] Source, test, fixture, documentation, CMake, and wrapper inspection completed.
- [x] Validation status recorded honestly: no build, CTest, formatter, or tidy command was run because this was a read-only audit.

## Findings

### Critical

No findings.

### High

#### APPLY-HIGH-001: Archives are never opened or applied

- [ ] Remediation status: open.
- Severity: high.
- Affected component: [`src/apply_data_archives.cppm#L24-L45`](src/apply_data_archives.cppm#L24-L45) and [`#L56-L66`](src/apply_data_archives.cppm#L56-L66).
- Technical evidence: `validate_archive` always returns `Native DataTypeManagerService is unavailable; archive selection was recorded but not applied` at line 44. Original `ApplyDataArchiveAnalyzer.java#L96-L115` obtains a `DataTypeManagerService`, opens selected archives, constructs `ApplyFunctionDataTypesCmd`, applies signatures, and reports the archive. Its archive selection/opening paths are `#L151-L230` and `#L233-L316`.
- Expected behavior: auto-detect source-language/built-in archives and user file/project archives, open valid managers, and apply function data types with the selected bookmark/source policy.
- Actual behavior: native code only validates file shape and records an unapplied `DataArchiveRecord`.
- Impact: the analyzer cannot change a function signature, which is its primary user-visible behavior.
- Reproduction or failure scenario: configure a valid `.gdt` path and `apply_data_archives=true`; `context.data_archives()` contains an error record and no function signature changes.
- Root cause: no native datatype-manager service or `ApplyFunctionDataTypesCmd` bridge is connected.
- Recommended fix: implement and connect archive opening, source-language discovery, function datatype application, bookmark creation, and error mapping before claiming analyzer parity.
- Regression risks: archive version errors, source precedence, and bookmark policy must match the original; add valid, invalid, built-in, user-file, project, and Go fixtures.
- Relevant tests or validation: [`tests/apply_data_archives_tests.cppm`](tests/apply_data_archives_tests.cppm#L15-L57) intentionally checks recording/validation rather than signature application.

#### APPLY-HIGH-002: Default enablement, selection model, and analyzer lifecycle differ

- [ ] Remediation status: open.
- Severity: high.
- Affected component: [`src/apply_data_archives.cppm#L50-L66`](src/apply_data_archives.cppm#L50-L66) and [`shared/src/analyzer_types.cppm#L315-L363`](../shared/src/analyzer_types.cppm#L315-L363).
- Technical evidence: native `apply_data_archives` defaults false and the descriptor triggers only `memory_added` at line 52. Original `ApplyDataArchiveAnalyzer.java#L81-L93` is a default-enabled `BYTE_ANALYZER` at `FUNCTION_ID_ANALYSIS.after()` and disables itself only for Go programs. Original options include chooser modes, bookmarks, user file, and project paths at `#L50-L78` and `#L119-L149`; native options expose only paths, source language, and a boolean.
- Expected behavior: one-time byte analysis should select archives using the original chooser/default rules and run for non-Go programs without requiring a synthetic memory event.
- Actual behavior: normal native analysis does nothing unless a caller flips a default-false option and supplies paths.
- Impact: the built-in registration at [`../analyzer_builtin.cpp#L40`](../analyzer_builtin.cpp#L40) does not reproduce the normal analyzer's automatic behavior.
- Root cause: the port replaced the archive-selection/application boundary with a path-validation boundary.
- Recommended fix: preserve the original lifecycle/options or clearly rename and scope the native component as an archive validator rather than an analyzer port.
- Regression risks: changing default enablement can alter pipeline ordering and source signatures; add scheduler tests for Go/non-Go and one-time execution.
- Relevant tests or validation: no scheduler integration test covers the original default path.

### Medium

#### APPLY-MEDIUM-001: Fixture report is an original-Java oracle, not a native parity test

- [ ] Remediation status: open.
- Severity: medium.
- Affected component: [`tests/data/run_ghidra.py`](tests/data/run_ghidra.py), [`tests/data/test_apply_data_archives.md`](tests/data/test_apply_data_archives.md), and [`CMakeLists.txt#L6-L13`](CMakeLists.txt#L6-L13).
- Technical evidence: CMake registers only the native GoogleTest executable. The generated report shows `memcpy` appearing after the Java target, but the native unit tests assert validation records and do not import or apply a real `.gdt` archive.
- Expected behavior: the native test must prove that an archive changes a function in the native model, or explicitly be labeled a boundary test.
- Actual behavior: the fixture proves original Ghidra behavior while the native test proves only refusal to apply.
- Impact: a passing test suite can coexist with a completely missing primary behavior.
- Root cause: the PyGhidra script is not part of CTest and the native model has no archive fixture/application API.
- Recommended fix: add a deterministic native archive/application test once the datatype boundary exists; retain the Java report as a separate oracle.
- Regression risks: generated archive contents are version-sensitive; pin a small test archive and assert source/parameter outcomes.
- Relevant tests or validation: no commands were run during this audit.

### Low

No findings.

## Verified Strengths

- [x] File existence, regular-file, extension, empty-file, source-language, cancellation, and diagnostic paths are explicit.
- [x] CMake and fixture wrapper navigation is present.

## Reviewed Areas With No Findings

- [x] The local CMake target and module build wrapper were inspected.
- [x] The checked-in fixture source and generated report were inspected without treating them as native proof.

## Validation Results

- [x] Read-only original/native comparison completed.
- [x] Read-only test, fixture, documentation, CMake, and wrapper inspection completed.
- [ ] Focused build/test: not run by request.
- [ ] `format.bat apply_data_archives`: not run by request.
- [ ] `tidy.bat apply_data_archives --check`: not run by request.

## Unresolved Questions

- [ ] The native datatype-manager/application API required for a complete port is not present in the reviewed boundary.

## Residual Risks

- [ ] Archive versioning and source-type precedence remain untested in native code.

## Follow-Up

- [ ] Highest proposed remediations: APPLY-HIGH-001 and APPLY-HIGH-002.
- [ ] Medium proposed remediation: APPLY-MEDIUM-001.
- [ ] Final follow-up decision: keep implementation unchanged until parity remediation is authorized.
