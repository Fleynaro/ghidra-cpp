# Strict Audit Report: PDB MSDIA

- [x] Scope confirmed: [`src/pdb_msdia.cppm`](src/pdb_msdia.cppm), [`tests/pdb_msdia_tests.cppm`](tests/pdb_msdia_tests.cppm), [`tests/data/run_ghidra.py`](tests/data/run_ghidra.py), [`tests/data/test_pdb_msdia.md`](tests/data/test_pdb_msdia.md), [`CMakeLists.txt`](CMakeLists.txt), and the checked-in PE/PDB fixtures.
- [x] Exact Ghidra sources inspected: [`PdbAnalyzer.java`](../../../../Ghidra/Features/PDB/src/main/java/ghidra/app/plugin/core/analysis/PdbAnalyzer.java), [`PdbAnalyzerCommon.java`](../../../../Ghidra/Features/PDB/src/main/java/ghidra/app/plugin/core/analysis/PdbAnalyzerCommon.java), and the legacy [`pdb/PdbParser.java`](../../../../Ghidra/Features/PDB/src/main/java/ghidra/app/util/bin/format/pdb/PdbParser.java). No `PdbMsdiaAnalyzer.java` exists in the current Ghidra tree.
- [x] Review date: 2026-09-15. No commit was supplied; the working tree was audited as-is.
- [x] Reviewer: Kilo.
- [x] Review method: source-to-source comparison, provider/applicator coverage inspection, fixture assertion inspection, and CMake/test-registration inspection.
- [ ] Native build, test, formatter, and tidy commands were not run because this was requested as a strict read-only audit.

## Findings

### Critical

No findings.

### High

#### HIGH-001: Native MSDIA is not the behavior of Ghidra `PdbAnalyzer`

- [ ] Remediation status: open.
- Severity: high.
- Title: `PdbMsdiaAnalyzer` introduces a direct DIA global-symbol subset instead of porting the legacy `PdbAnalyzer`/`PdbParser`/XML applicator contract.
- Exact references: [`src/pdb_msdia.cppm:175-249`](src/pdb_msdia.cppm#L175-L249), symbol `MsdiaSession::open`; [`src/pdb_msdia.cppm:303-334`](src/pdb_msdia.cppm#L303-L334), symbol `PdbMsdiaAnalyzer::analyze`; Ghidra [`PdbAnalyzer.java:58-132`](../../../../Ghidra/Features/PDB/src/main/java/ghidra/app/plugin/core/analysis/PdbAnalyzer.java#L58-L132); Ghidra [`PdbParser.java:165-212`](../../../../Ghidra/Features/PDB/src/main/java/ghidra/app/util/bin/format/pdb/PdbParser.java#L165-L212) and [`:336-420`](../../../../Ghidra/Features/PDB/src/main/java/ghidra/app/util/bin/format/pdb/PdbParser.java#L336-L420).
- Affected component: legacy PDB analysis, type application, symbol application, and exact analyzer selection.
- Technical evidence: Ghidra `PdbAnalyzer.added` rejects partial sets, checks PDB-loaded/universal conflicts, locates the PDB through `PdbAnalyzerCommon`, constructs the legacy `PdbParser`, opens datatype archives, parses/validates the external parser output, and calls `parser.applyTo`. Native creates COM/DIA, enumerates only global `SymTagFunction` and `SymTagData`, and applies name-only `PdbSymbolRecord`/basic data. The exact source path named by native comments, `PdbMsdiaAnalyzer.java`, does not exist.
- Expected behavior: the native analyzer should either port Ghidra's legacy provider/parser/application semantics or be explicitly named and documented as a different DIA provider feature.
- Actual behavior: many legacy PDB records/types/functions are not processed, and the native analyzer can report a successful DIA load without producing any equivalent Ghidra datatype/application state.
- Impact: users receive a different analyzer with different availability, matching, and output semantics under the same `PDB MSDIA` label.
- Reproduction or failure scenario: use a PDB containing user-defined structs/enums and function internals. Native enumerates global functions/data but does not enumerate/apply the type graph; Ghidra legacy parser applies types through its `PdbParser.applyTo` pipeline.
- Root cause: the native implementation selected the Microsoft DIA COM SDK as a provider boundary, while the current Ghidra legacy analyzer delegates to `pdb.exe`/XML parsing and a separate applicator.
- Recommended fix: decide whether this is an intentional new provider or an exact legacy port. For exact parity, port the legacy parser/application behavior and preserve Ghidra's analyzer conflict/matching rules; for a new provider, rename/document the intentional difference.
- Regression risks: changing provider selection can alter Windows build requirements and fixture availability.
- Relevant tests or validation: [`tests/pdb_msdia_tests.cppm:19-37`](tests/pdb_msdia_tests.cppm#L19-L37) tests only direct provider symbol enumeration, not `PdbMsdiaAnalyzer::analyze` or legacy application.

### Medium

#### MEDIUM-001: Identity and analyzer-conflict rules are absent

- [ ] Remediation status: open.
- Severity: medium.
- Title: native MSDIA accepts a configured PDB path without CodeView identity matching, loaded-state checks, or Universal/legacy exclusivity.
- Exact references: [`src/pdb_msdia.cppm:175-249`](src/pdb_msdia.cppm#L175-L249); [`src/pdb_msdia.cppm:303-334`](src/pdb_msdia.cppm#L303-L334); Ghidra [`PdbAnalyzer.java:61-90`](../../../../Ghidra/Features/PDB/src/main/java/ghidra/app/plugin/core/analysis/PdbAnalyzer.java#L61-L90); Ghidra [`PdbAnalyzerCommon.java:100-177`](../../../../Ghidra/Features/PDB/src/main/java/ghidra/app/plugin/core/analysis/PdbAnalyzerCommon.java#L100-L177).
- Affected component: PDB applicability and repeated analysis.
- Technical evidence: Ghidra checks `PdbParser.isAlreadyLoaded`, refuses to run alongside enabled Universal, locates/matches the program's PDB metadata, and uses one-time/full-set analysis. Native only checks that `pdb_path` is nonempty/existing and that enumerated virtual addresses fall in a memory region.
- Expected behavior: a mismatched or already-loaded PDB should be rejected or governed by an explicit allow-untrusted option, and the two analyzers should not both apply the same file.
- Actual behavior: any valid PDB with in-range DIA addresses can add symbols/data, and repeated `memory_added` events reopen and reapply it.
- Impact: wrong names/data can be applied to a program and provider work can be repeated unpredictably.
- Reproduction or failure scenario: set native `pdb_path` to a different executable's PDB whose symbols happen to have in-range virtual addresses; native accepts them without comparing RSDS GUID/signature/age.
- Root cause: native `AnalysisContext` lacks CodeView/program-property identity and analyzer-option conflict APIs.
- Recommended fix: add identity and loaded-state providers, full-set/one-time gating, and explicit Universal/MSDIA mutual exclusion.
- Regression risks: user-selected nonmatching PDB workflows require an intentional option equivalent to Ghidra's allow-untrusted path.
- Relevant tests or validation: [`tests/data/run_ghidra.py:234-246`](tests/data/run_ghidra.py#L234-L246) validates the Java program/PDB match, but no native analyzer test does.

#### MEDIUM-002: Test coverage is provider-only and permits environmental non-application

- [ ] Remediation status: open.
- Severity: medium.
- Title: current tests do not prove analyzer application and deliberately accept missing DIA on Windows.
- Exact references: [`tests/pdb_msdia_tests.cppm:19-47`](tests/pdb_msdia_tests.cppm#L19-L47); [`tests/data/run_ghidra.py:230-246`](tests/data/run_ghidra.py#L230-L246); [`tests/data/test_pdb_msdia.md:7-10`](tests/data/test_pdb_msdia.md#L7-L10) and [`:164-167`](tests/data/test_pdb_msdia.md#L164-L167); [`CMakeLists.txt:19-26`](CMakeLists.txt#L19-L26).
- Affected component: Windows provider/application regression infrastructure.
- Technical evidence: on Windows, `OpensThroughDiaProvider` returns successfully when the DIA COM class is unavailable after checking only for HRESULT `0x80040154`; it never calls `PdbMsdiaAnalyzer::analyze`. On non-Windows it only tests the expected platform error. The PyGhidra fixture runs Java `PdbAnalyzer`, records `PDB applied by MSDIA: false` in this environment, and treats no application as an environment limitation. CMake registers only the direct GoogleTest target and does not run `run_ghidra.py`.
- Expected behavior: a passing analyzer test must assert native symbol/data/type application when DIA is available and a separately classified infrastructure test must assert the expected unavailable-provider failure.
- Actual behavior: all current CTest assertions can pass without any native analyzer mutation, and the Markdown report can be stale/unapplied without failing CI.
- Impact: the central behavioral gap in `HIGH-001` is untested.
- Reproduction or failure scenario: make `PdbMsdiaAnalyzer::analyze` return before applying symbols; both current tests remain green because neither invokes it.
- Root cause: provider-session tests and Java reference fixture are disconnected from the analyzer target.
- Recommended fix: add a native `AnalysisContext` application test, separate provider-availability tests, and wire any Java reference script through the required wrapper.
- Regression risks: DIA-dependent tests need deterministic skip/fail policy across machines.
- Relevant tests or validation: source inspection only; no commands were run in this audit.

#### MEDIUM-003: Port references point to a nonexistent original class

- [ ] Remediation status: open.
- Severity: medium.
- Title: native source and `GHIDRA_PORT.md` claim a traceable `PdbMsdiaAnalyzer.java` source that is absent.
- Exact references: [`src/pdb_msdia.cppm:13-16`](src/pdb_msdia.cppm#L13-L16); [`GHIDRA_PORT.md:3-6`](GHIDRA_PORT.md#L3-L6); original directory [`Ghidra/Features/PDB/src/main/java/ghidra/app/plugin/core/analysis/`](../../../../Ghidra/Features/PDB/src/main/java/ghidra/app/plugin/core/analysis/).
- Affected component: port evidence and auditability.
- Technical evidence: the original directory contains `PdbAnalyzer.java` and `PdbUniversalAnalyzer.java`, but no `PdbMsdiaAnalyzer.java`. The native class name and direct DIA workflow therefore cannot be traced to an exact Java class as documented.
- Expected behavior: every original-reference claim must resolve to an existing source, and intentional provider differences must be marked as such.
- Actual behavior: a reader following the required port reference reaches no source for the native analyzer class.
- Impact: reviewers cannot verify the claimed behavioral origin, and the report's source mapping overstates parity.
- Reproduction or failure scenario: resolve the Markdown/source reference path; it is absent from the Ghidra tree.
- Root cause: the native provider design was mapped to a presumed class rather than the actual `PdbAnalyzer` implementation.
- Recommended fix: correct references and document the direct DIA provider as an intentional divergence or port it to the actual legacy parser contract.
- Regression risks: documentation changes may expose additional unported behavior but do not affect runtime.
- Relevant tests or validation: path search found no `PdbMsdiaAnalyzer.java` in `Ghidra`.

### Low

No findings.

## Verified Strengths

- [x] The Windows provider uses real COM/DIA `DiaSource`, `loadDataFromPdb`, `openSession`, and BSTR cleanup rather than a name-only fake.
- [x] Non-Windows builds return a deterministic unsupported-provider error.
- [x] COM/DIA resource ownership and move-only session behavior are explicitly represented.

## Reviewed Areas With No Findings

- [x] The Windows CMake dependency check correctly requires `dia2.h` and `diaguids.lib` before compiling the provider.
- [x] Provider-only tests have a deterministic platform boundary; their limitation is scope, not assertion nondeterminism.

## Validation

- [x] Original legacy analyzer/common/parser, native provider/analyzer, tests, fixture source/script/report, and CMake registration were inspected.
- [ ] Focused build/test was not run by this audit.
- [ ] `format.bat analyzer` and `tidy.bat analyzer --check` were not run by this audit.
- [x] No implementation, test, fixture, or build source was edited.

## Unresolved Questions And Residual Risks

- [ ] The project must decide whether direct DIA is a new provider feature or an exact replacement for legacy Ghidra `PdbAnalyzer`; the current name and references imply both.
- [ ] A Windows machine with registered DIA was not available for application-level validation in this read-only review.

## Follow-Up Decision

- [ ] Highest-priority open item is `HIGH-001`; `MEDIUM-001` and `MEDIUM-002` must be resolved before native MSDIA application is treated as tested.
