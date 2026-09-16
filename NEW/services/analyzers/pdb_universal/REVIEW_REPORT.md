# Strict Audit Report: PDB Universal

- [x] Scope confirmed: [`src/pdb_universal.cppm`](src/pdb_universal.cppm), [`tests/pdb_universal_tests.cppm`](tests/pdb_universal_tests.cppm), [`tests/data/run_ghidra.py`](tests/data/run_ghidra.py), [`tests/data/test_pdb_universal.md`](tests/data/test_pdb_universal.md), [`CMakeLists.txt`](CMakeLists.txt), and the checked-in PE/PDB fixtures.
- [x] Exact Ghidra sources inspected: [`PdbUniversalAnalyzer.java`](../../../../Ghidra/Features/PDB/src/main/java/ghidra/app/plugin/core/analysis/PdbUniversalAnalyzer.java), [`PdbAnalyzerCommon.java`](../../../../Ghidra/Features/PDB/src/main/java/ghidra/app/plugin/core/analysis/PdbAnalyzerCommon.java), [`PdbParser.java`](../../../../Ghidra/Features/PDB/src/main/java/ghidra/app/util/bin/format/pdb2/pdbreader/PdbParser.java), [`DefaultPdbApplicator.java`](../../../../Ghidra/Features/PDB/src/main/java/ghidra/app/util/pdb/pdbapplicator/DefaultPdbApplicator.java), and the `pdb2/pdbreader` package.
- [x] Review date: 2026-09-15. No commit was supplied; the working tree was audited as-is.
- [x] Reviewer: Kilo.
- [x] Review method: source-to-source comparison, parser/applicator coverage inspection, fixture assertion inspection, and CMake/test-registration inspection.
- [ ] Native build, test, formatter, and tidy commands were not run because this was requested as a strict read-only audit.

## Findings

### Critical

No findings.

### High

#### HIGH-001: The native parser/applicator is a fixture subset, not the full Universal port

- [ ] Remediation status: open.
- Severity: high.
- Title: one native `cppm` contains a compact MSF/TPI/symbol subset and cannot reproduce the full `pdb2` reader plus `DefaultPdbApplicator` behavior.
- Exact references: [`src/pdb_universal.cppm:67-85`](src/pdb_universal.cppm#L67-L85), symbols `PdbReader` and `PdbUniversalAnalyzer`; parser functions [`src/pdb_universal.cppm:416-967`](src/pdb_universal.cppm#L416-L967); application [`src/pdb_universal.cppm:985-1032`](src/pdb_universal.cppm#L985-L1032). Original applicator entry points are [`DefaultPdbApplicator.java:296-355`](../../../../Ghidra/Features/PDB/src/main/java/ghidra/app/util/pdb/pdbapplicator/DefaultPdbApplicator.java#L296-L355), [`:496-591`](../../../../Ghidra/Features/PDB/src/main/java/ghidra/app/util/pdb/pdbapplicator/DefaultPdbApplicator.java#L496-L591), and [`:1282-1388`](../../../../Ghidra/Features/PDB/src/main/java/ghidra/app/util/pdb/pdbapplicator/DefaultPdbApplicator.java#L1282-L1388).
- Affected component: raw PDB parsing, data-type application, symbols, function internals, and analysis reporting.
- Technical evidence: the repository contains 591 original Java files below `Ghidra/Features/PDB/src/main/java/ghidra/app/util/bin/format/pdb2/pdbreader`, while the native Universal module has one source file. Native models only `PdbIdentity`, compact named types/fields, procedure/data/public/section symbols, and a small procedure-signature map. The Java applicator resolves the full type graph, typedefs/classes/vtables, public/global/module/thunk symbols, functions, parameters/locals/block scopes, source lines, FPO, disassembly, data archives, reporting, and deferred processing through dedicated appliers.
- Expected behavior: all supported PDB record kinds and Universal applicator phases should be represented or explicitly rejected with a documented compatibility boundary; a fixture subset must not be presented as a full port.
- Actual behavior: unsupported records are omitted or the parser stops at unknown field-list entries, and native application can only add compact `PdbTypeRecord`/`PdbSymbolRecord` data plus basic functions/signatures and one identity bookmark.
- Impact: real PDBs with common records outside the fixture lose debug information, and downstream analyses see incomplete or incorrect program state.
- Reproduction or failure scenario: use a PDB containing source/line records, locals, FPO, class inheritance/vtables, typedefs, forward references, nested procedures, or unsupported CodeView symbol records. Native produces no equivalent source/local/class/function-internal state while Java applicator processes those phases.
- Root cause: the implementation intentionally stopped at the records needed by `test_pdb_universal.pdb` and does not provide the data-type manager, namespace, listing, line-table, or deferred-applicator boundaries used by Ghidra.
- Recommended fix: port the reader record families and applicator phases module-by-module, with explicit per-record compatibility tests; keep the status marked partial until those phases exist.
- Regression risks: type-resolution and symbol-primary ordering must remain deterministic and compatible with existing PE/PDB facts.
- Relevant tests or validation: [`tests/pdb_universal_tests.cppm:20-44`](tests/pdb_universal_tests.cppm#L20-L44) checks one struct and one procedure; [`:54-72`](tests/pdb_universal_tests.cppm#L54-L72) checks only the compact native model.

#### HIGH-002: PDB identity, loaded-state, and analyzer applicability rules are missing

- [ ] Remediation status: open.
- Severity: high.
- Title: native `PdbUniversalAnalyzer::analyze` trusts the configured path and reparses on memory events without Java's identity/applicability guard.
- Exact references: [`src/pdb_universal.cppm:974-983`](src/pdb_universal.cppm#L974-L983), symbol `pdb_address`; [`src/pdb_universal.cppm:1036-1049`](src/pdb_universal.cppm#L1036-L1049), symbol `PdbUniversalAnalyzer::analyze`; Ghidra [`PdbUniversalAnalyzer.java:116-248`](../../../../Ghidra/Features/PDB/src/main/java/ghidra/app/plugin/core/analysis/PdbUniversalAnalyzer.java#L116-L248); Ghidra [`PdbAnalyzerCommon.java:100-177`](../../../../Ghidra/Features/PDB/src/main/java/ghidra/app/plugin/core/analysis/PdbAnalyzerCommon.java#L100-L177); Ghidra [`PdbParser.java:239-243`](../../../../Ghidra/Features/PDB/src/main/java/ghidra/app/util/bin/format/pdb/PdbParser.java#L239-L243).
- Affected component: PDB-to-PE matching, repeat analysis, and user-visible PDB loaded state.
- Technical evidence: Java locates the PDB from CodeView metadata, checks the already-loaded property, validates GUID/signature/age through the reader/applicator path, requires a full analysis set, and sets `PDB_LOADED` only after main application. Native maps section/offset to the current image but never compares `PdbIdentity` with the image's RSDS identity, has no `PDB_LOADED` program property API, accepts any configured PDB path, and runs on every `memory_added` trigger.
- Expected behavior: a mismatched PDB must be rejected or explicitly allowed by an option, and an already-applied PDB must not be silently reapplied.
- Actual behavior: a valid but unrelated PDB can add names/types to a program, and repeated analysis reparses/reapplies it; identity is merely recorded in a bookmark.
- Impact: wrong symbols/types can be applied to an executable and repeated runs can produce ordering-dependent duplicate/primary changes or unnecessary work.
- Reproduction or failure scenario: configure `pdb_path` to a valid PDB from another PE with compatible section numbers. Native accepts records whose addresses happen to map; Java's program/PDB identity validation prevents normal application.
- Root cause: `AnalysisContext` lacks CodeView identity and program-property boundaries, while the fixture harness performs identity validation externally in Python.
- Recommended fix: expose loader CodeView identity and a loaded-state property, implement exact matching and full-set/one-time gating in the analyzer, and test mismatched/previously-loaded cases.
- Regression risks: existing callers that intentionally apply a user-selected nonmatching PDB need an explicit allow-untrusted equivalent.
- Relevant tests or validation: [`tests/data/run_ghidra.py:192-199`](tests/data/run_ghidra.py#L192-L199) calls `validate_pdb_match` before Java analysis; no native test performs that check.

### Medium

#### MEDIUM-001: Type and signature application is lossy even for supported records

- [ ] Remediation status: open.
- Severity: medium.
- Title: native application flattens PDB types and procedure signatures into strings and discards varargs/storage/source metadata.
- Exact references: [`src/pdb_universal.cppm:1007-1025`](src/pdb_universal.cppm#L1007-L1025), symbol `apply_pdb`; [`NEW/services/analyzers/shared/src/analyzer_context.cppm:1141-1159`](../shared/src/analyzer_context.cppm#L1141-L1159); Ghidra [`DefaultPdbApplicator.java:289-308`](../../../../Ghidra/Features/PDB/src/main/java/ghidra/app/util/pdb/pdbapplicator/DefaultPdbApplicator.java#L289-L308) and [`:482-494`](../../../../Ghidra/Features/PDB/src/main/java/ghidra/app/util/pdb/pdbapplicator/DefaultPdbApplicator.java#L482-L494).
- Affected component: imported function signatures and type identity.
- Technical evidence: native derives one convention/return spelling by splitting a compact string, creates `param_N` entries with zero storage and `variadic=false`, and stores fields as string pairs. Java resolves actual `DataType` objects, calling conventions, parameter storage/source, arrays/pointers/typedefs, and function internals through appliers.
- Expected behavior: a supported PDB function should preserve its actual type graph, calling convention, parameter properties, and varargs/return details.
- Actual behavior: names may look correct while the native model loses storage, typedef identity, varargs, source priority, and nested type semantics.
- Impact: PDB consumers and decompiler analyzers receive weaker facts than Ghidra even on the fixture's supported record family.
- Reproduction or failure scenario: use a variadic procedure or a typedef/pointer/array-heavy signature; native stores a flat spelling and marks it non-variadic with zero storage.
- Root cause: `PdbFile::procedure_signature` returns only a pair of strings and `AnalysisContext::FunctionParameter` has no PDB type-resolution/source boundary.
- Recommended fix: represent CodeView type indices and resolved data types structurally, then apply function signatures with explicit varargs/storage/source metadata.
- Regression risks: changing existing string output can affect current fixture reports; retain stable display helpers while adding structure.
- Relevant tests or validation: [`tests/pdb_universal_tests.cppm:42-44`](tests/pdb_universal_tests.cppm#L42-L44) asserts only parameter count, not storage, varargs, or full type identity.

#### MEDIUM-002: The PDB fixture report is not a native integration test

- [ ] Remediation status: open.
- Severity: medium.
- Title: current tests prove a narrow parser/context subset, while the large Markdown artifact is generated from Ghidra Java and is not registered in CTest.
- Exact references: [`tests/pdb_universal_tests.cppm:20-72`](tests/pdb_universal_tests.cppm#L20-L72); [`tests/data/run_ghidra.py:182-201`](tests/data/run_ghidra.py#L182-L201); [`tests/data/test_pdb_universal.md:205-302`](tests/data/test_pdb_universal.md#L205-L302); [`CMakeLists.txt:7-13`](CMakeLists.txt#L7-L13).
- Affected component: fixture provenance and regression detection.
- Technical evidence: the C++ test directly exercises `PdbReader` and `PdbUniversalAnalyzer` against the native context, which is meaningful for the supported subset. The Python script starts PyGhidra, imports/reopens a Ghidra program, validates identity, runs `project.analyze`, and renders Ghidra listing artifacts; no native library is loaded. CMake adds only the GoogleTest executable.
- Expected behavior: report provenance should distinguish Java reference output from native parity, and CI should run every claimed fixture or replace the report with native assertions.
- Actual behavior: the report's 70-row delta demonstrates Java PDB application, not the native parser/applicator; the native CTest test does not cover those rows.
- Impact: unsupported records and identity/reapplication regressions can be introduced without failing tests.
- Reproduction or failure scenario: remove native support for a type record not asserted by `PdbUniversalTests`; the checked-in Markdown remains unchanged and CTest remains green.
- Root cause: reference fixture generation is disconnected from native CMake test registration.
- Recommended fix: add native end-to-end artifact assertions for the supported boundary and register the reference script through the required PyGhidra wrapper with an explicit Java-only label.
- Regression risks: full artifact snapshots are version-sensitive; assert structured deltas and parser counts where stable.
- Relevant tests or validation: source inspection only; no commands were run in this audit.

### Low

No findings.

## Verified Strengths

- [x] MSF 7.0 bounds and stream-directory checks reject the supplied non-PDB input instead of treating it as symbols.
- [x] The checked-in fixture test verifies identity GUID/age, a named struct's fields/size, and a procedure's parameter type-index path.
- [x] Native cancellation checks are present during type and symbol application.

## Reviewed Areas With No Findings

- [x] The native PDB path and PDB fixture are wired into the direct C++ analyzer test.
- [x] The local CMake target and GoogleTest registration are internally consistent.

## Validation

- [x] Original Universal analyzer, legacy common matching logic, raw parser/applicator entry points, native source, tests, fixture script/report, and CMake registration were inspected.
- [ ] Focused build/test was not run by this audit.
- [ ] `NEW\format.bat analyzer` and `NEW\tidy.bat analyzer --check` were not run by this audit.
- [x] No implementation, test, fixture, or build source was edited.

## Unresolved Questions And Residual Risks

- [ ] The native model needs program metadata/property and resolved datatype boundaries before identity and applicator parity can be evaluated end-to-end.
- [ ] The exact intended scope must be reconciled with `GHIDRA_PORT.md`, which currently calls many subset behaviors complete while also listing pending records.

## Follow-Up Decision

- [ ] Highest-priority open items are `HIGH-001` and `HIGH-002`; the fixture report must not be used as proof of full native parity until `MEDIUM-002` is resolved.
