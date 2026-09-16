# Review Report: Analyzer Port Audit

- [x] Scope confirmed: current `NEW/services/analyzers` implementations, tests, fixture C++/RC/PDB sources, PyGhidra scripts and generated reports, module `README.md`/`GHIDRA_PORT.md`, CMake files, module/root build wrappers, and shared analyzer/test-support boundaries.
- [x] Original sources inspected directly; prior review summaries were not used as evidence.
- [x] Reviewed diff: working tree at 2026-09-15; no commit supplied.
- [x] Reviewer: Kilo.
- [x] Exact moved original paths resolved: `Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/disassembler/CallFixupAnalyzer.java`, `CallFixupChangeAnalyzer.java`, and `Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/clear/ClearFlowAndRepairCmd.java`.
- [x] Read-only audit completed before remediation; subsequent changes are listed below.
- [x] Required module-local detailed reports: [`ascii_strings/REVIEW_REPORT.md`](ascii_strings/REVIEW_REPORT.md), [`apply_data_archives/REVIEW_REPORT.md`](apply_data_archives/REVIEW_REPORT.md), [`demangler_microsoft/REVIEW_REPORT.md`](demangler_microsoft/REVIEW_REPORT.md), [`external_entry_references/REVIEW_REPORT.md`](external_entry_references/REVIEW_REPORT.md), [`call_fixup_installer/REVIEW_REPORT.md`](call_fixup_installer/REVIEW_REPORT.md), [`windows_resource_reference/REVIEW_REPORT.md`](windows_resource_reference/REVIEW_REPORT.md), [`windows_pe_x86_propagate_external_parameters/REVIEW_REPORT.md`](windows_pe_x86_propagate_external_parameters/REVIEW_REPORT.md), [`x86_constant_reference/REVIEW_REPORT.md`](x86_constant_reference/REVIEW_REPORT.md), and [`constant_propagation/REVIEW_REPORT.md`](constant_propagation/REVIEW_REPORT.md).
- [x] Current core-contract boundary addendum reviewed; its detailed findings are recorded in [`../REVIEW_REPORT.md`](../REVIEW_REPORT.md) and [`../../core/contracts/REVIEW_REPORT.md`](../../core/contracts/REVIEW_REPORT.md).
- [x] Validation status recorded honestly: no build, CTest, formatter, tidy, PyGhidra, or fixture compilation command was run because the request was read-only.

## Findings

### Critical

#### AUDIT-CRITICAL-001: Complete Microsoft mdemangler is not ported

- [ ] Remediation status: open.
- Severity: critical.
- Affected component: `NEW/services/analyzers/demangler_microsoft/src/demangler_microsoft.cppm`.
- Technical evidence: the current module is a 407-line compact parser and structured record; original `Ghidra/Features/MicrosoftDemangler` contains the full `mdemangler` parser/object/context/options implementation and broad tests.
- Expected behavior: complete original grammar, object hierarchy, context-sensitive C-style handling, output options, errors, and exact rendering.
- Actual behavior: narrow grammar with `unknown` fallback and no full object/options model.
- Impact: Microsoft decorated symbols can be fabricated, lost, or incorrectly typed; the requested complete demangler port is absent.
- Reproduction or failure scenario: original `MicrosoftDemanglerExtraTest.java#L181-L450` cases have no equivalent native result contract.
- Root cause: a partial handwritten replacement was used instead of porting `mdemangler`.
- Recommended fix: port the complete package and original vectors before parity claims.
- Regression risks: exact name/signature output is compatibility-sensitive.
- Relevant tests or validation: detailed evidence is in [`demangler_microsoft/REVIEW_REPORT.md`](demangler_microsoft/REVIEW_REPORT.md).

### High

#### AUDIT-HIGH-001: Primary listing mutations are replaced by evidence records in multiple analyzers

- [ ] Remediation status: open.
- Severity: high.
- Affected components: Apply Data Archives, Call-Fixup Installer, Windows PE x86 External Parameters, and Windows Resource Reference.
- Technical evidence: archive code records an unapplied error instead of opening/applying a GDT; call-fixup code adds `SymbolRecord` instead of function injection; parameter code adds symbol/bookmark evidence instead of comments/data; resource code heuristically scans operands instead of decompiler argument P-code. See the linked module reports for exact lines and original references.
- Expected behavior: preserve the original user-visible database/listing, function, decompiler, comment, data, reference, and flow mutations.
- Actual behavior: native boundary records partial metadata and explicitly stops at unavailable services.
- Impact: successful native analysis does not produce the original observable program state.
- Reproduction or failure scenario: the generated Java fixture reports show signature, call-fixup, EOL-comment, or DATA-reference changes that native tests do not assert or cannot create.
- Root cause: missing native datatype, compiler-spec, listing/comment, decompiler, and clear/repair APIs.
- Recommended fix: implement the missing model boundaries and port mutations before treating evidence records as equivalent.
- Regression risks: these mutations affect downstream analyzer scheduling and must be transactionally tested.
- Relevant tests or validation: module-local reports enumerate each mismatch and test gap.

#### AUDIT-HIGH-002: Call-fixup flow repair does not implement ClearFlowAndRepairCmd

- [ ] Remediation status: open.
- Severity: high.
- Affected component: `NEW/services/analyzers/call_fixup_installer/src/call_fixup_installer.cppm` and `NEW/services/analyzers/shared/src/analyzer_context.cppm`.
- Technical evidence: exact original `ClearFlowAndRepairCmd.java` is at `Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/clear/ClearFlowAndRepairCmd.java` and performs clearing, protected-set handling, bookmark cleanup, disassembly, data-reference analysis, and function repair across 1,022 lines. Native only sets `CALL_RETURN` and recomputes known CFG blocks.
- Expected behavior: clear and repair all affected flow and dependent program state.
- Actual behavior: stale instructions/data/bookmarks and unmodeled destinations remain.
- Impact: call-fixup/no-return callers retain wrong bodies and fall-through behavior.
- Root cause: local CFG rebuild is not the original command.
- Recommended fix: port or expose equivalent clear/disassemble/repair transactions.
- Regression risks: function deletion/recreation and downstream events can change.
- Relevant tests or validation: detailed evidence is in [`call_fixup_installer/REVIEW_REPORT.md`](call_fixup_installer/REVIEW_REPORT.md).

#### AUDIT-HIGH-003: X86 and constant propagation omit original symbolic/branch behavior

- [ ] Remediation status: open.
- Severity: high.
- Affected components: `constant_propagation/src/constant_propagation.cppm` and `x86_constant_reference/src/x86_constant_reference.cppm`.
- Technical evidence: original `ConstantPropagationAnalyzer` uses `SymbolicPropogator`/`ConstantPropagationContextEvaluator`; original `X86Analyzer` adds x86 evaluator hooks, and `PropagateX86ConstantReferences.java` performs computed-branch/address-table exploration. Native uses a local opcode switch and a single LEA scan based on append-only facts.
- Expected behavior: path-aware symbolic propagation, references/data effects, x86 flow hooks, and switch/table recovery.
- Actual behavior: subset arithmetic facts and one LEA DATA-reference pass.
- Impact: realistic memory, branch, switch, and path-join behavior is missing.
- Root cause: native local maps/evidence were substituted for the original symbolic model.
- Recommended fix: port the symbolic and x86 evaluator boundaries and original fixtures.
- Regression risks: propagation changes can affect all downstream analyzers.
- Relevant tests or validation: detailed evidence is in [`constant_propagation/REVIEW_REPORT.md`](constant_propagation/REVIEW_REPORT.md) and [`x86_constant_reference/REVIEW_REPORT.md`](x86_constant_reference/REVIEW_REPORT.md).

### Medium

#### AUDIT-MEDIUM-001: Fixture reports are original-Java oracles and are not CTest parity tests

- [ ] Remediation status: open.
- Severity: medium.
- Affected component: all requested module `tests/data/run_ghidra.py` scripts and module CMake files.
- Technical evidence: module CMake files register native GoogleTest executables only; PyGhidra scripts generate Markdown reports but are not connected to CTest. The native tests generally use synthetic records or narrow cases, and `shared/test_support/analyzer_test_support.cppm#L24-L32` hard-codes `x86-64.sla` for every fixture, including x86-built fixtures.
- Expected behavior: native tests should verify the native implementation's claimed contract, with separate wrapper-driven original oracles explicitly labeled and compared.
- Actual behavior: generated original reports can disagree with native expectations without failing native tests.
- Impact: focused green tests do not establish 1:1 behavior.
- Root cause: fixture/oracle and native test graphs are separate and architecture support is shared/hard-coded.
- Recommended fix: add native integration/parity tests and architecture-aware fixture loading; keep wrapper execution through `TEST/run_ghidra_python.bat`.
- Regression risks: environment-dependent PyGhidra tests require clear skip/failure policy.
- Relevant tests or validation: detailed module reports identify exact test validity limitations.

#### AUDIT-MEDIUM-002: Several analyzer input sets/options/lifecycles are narrower or invented

- [ ] Remediation status: open.
- Severity: medium.
- Affected components: strings, apply archives, demangler, external entries, call fixups, resource references, external parameters, and x86.
- Technical evidence: current implementations often use PE exports or prior instruction operands where originals use symbol tables, decompiler HighFunctions, compiler specs, chooser options, or analyzer subclasses. Current descriptors also collapse distinct original lifecycle contracts in places such as CallFixupAnalyzer/CallFixupChangeAnalyzer.
- Expected behavior: exact original source sets, option defaults, analyzer types, priorities, one-time/reanalysis semantics, and event boundaries.
- Actual behavior: approximation contracts are exposed as ports and some defaults differ, including Apply Data Archives default-disabled versus original default-enabled for non-Go programs.
- Impact: normal pipeline results can differ even where an individual helper appears reasonable.
- Root cause: missing framework boundaries and replacement behavior not isolated under separate names.
- Recommended fix: preserve source/lifecycle contracts or document each component as intentionally partial.
- Regression risks: scheduler changes can reorder downstream mutations.
- Relevant tests or validation: module-local reports contain exact descriptor/source-set references.

### Low

#### AUDIT-LOW-001: Port evidence is inconsistent with implementation status

- [ ] Remediation status: open.
- Severity: low.
- Affected component: module `README.md`/`GHIDRA_PORT.md` files and `constant_propagation`, which lacks `GHIDRA_PORT.md`.
- Technical evidence: several existing port documents describe preserved behavior while also noting unavailable APIs; prior review reports claimed focused tests/formatting passed, but this audit did not run them and the source comparison confirms major omissions.
- Expected behavior: documentation must clearly mark complete, partial, intentionally different, and pending areas and report only validations actually performed.
- Actual behavior: documentation and earlier reports can read as stronger parity claims than the source supports.
- Impact: reviewers and maintainers can misclassify a boundary/evidence test as a complete port.
- Root cause: documentation was synchronized to the replacement boundary rather than the exact original contract.
- Recommended fix: update port evidence and module reports whenever status changes; add the missing constant-propagation port document.
- Regression risks: documentation drift can reintroduce false completion claims.
- Relevant tests or validation: all module-local reports include current validation status.

## Verified Strengths

- [x] Requested original paths were located and inspected with direct source reads and grep/glob searches.
- [x] All requested current modules have implementation/test/fixture/build/doc evidence inspected.
- [x] The native code consistently uses explicit source references and often records unavailable APIs instead of silently fabricating full state.

## Reviewed Areas With No Findings

- [x] No fixture runner or Markdown oracle was edited during remediation.
- [x] No secrets or environment values were read into the report; `.env` is absent in this worktree.
- [x] `TEST/run_ghidra_python.bat` was inspected; it validates `GHIDRA_INSTALL_DIR` and PyGhidra before execution.

## Validation Results

- [x] Direct original Java/C/script versus native source comparison completed.
- [x] Native tests, fixture sources/scripts/reports, Markdown, CMake, module/root wrappers, and shared context/test support inspected.
- [x] `NEW\build.bat analyzer`: 30/30 analyzer tests passed after remediation.
- [x] Complete `ctest --test-dir NEW\build --output-on-failure`: 42/42 tests passed.
- [x] `NEW\format.bat analyzer` completed.
- [ ] `NEW\tidy.bat analyzer --check`: blocked by MSVC `.ifc` module-consumer parsing; three non-module fixture sources also report diagnostics.
- [ ] PyGhidra fixture scripts remain disconnected from CTest and were not rerun because no fixture was strengthened during this pass.

## Unresolved Questions

- [ ] Which native framework boundaries will expose datatype managers, compiler-spec injection, listing comments/data, HighFunction argument recovery, symbol-table external entries, and ClearFlow repair is not resolved by the current sources.

## Residual Risks

- [ ] Native CTest results still do not prove Java parity because PyGhidra reports are disconnected from CTest and several native tests are evidence-only.
- [ ] Existing staged/user changes in unrelated analyzer modules were not reverted or modified.

## Follow-Up

- [x] Verified baseline defects were fixed: aggressive terminal-flow handling and embedded JPEG parsing.
- [x] Shared code/data collision, maximum-address insertion, PDB symbol ordering, Ghidra priorities/defaults, and address-table pointer materialization were corrected with regression coverage.
- [ ] Critical/high parity gaps remain: complete Microsoft demangler, datatype archives, call-fixup repair, symbolic x86 propagation, structured decompiler results, full AddressTable, and PDB application phases.
- [ ] Final follow-up decision: these remaining parity gaps require separate infrastructure ports; they must not be hidden by weakening tests.

## Post-Audit Remediation

- [x] `NEW/services/analyzers/aggressive_instruction_finder/REVIEW_REPORT.md` records the terminal-flow failure classification and remaining ARM/scheduler gaps.
- [x] `NEW/services/analyzers/embedded_media/REVIEW_REPORT.md` records the JPEG failure classification and remaining datatype-parser gaps.
- [x] `NEW/services/analyzers/create_address_tables/REVIEW_REPORT.md` records the corrected pointer-per-entry expectation and incomplete Java algorithm.
- [x] `NEW/services/analyzers/shared_return_calls/REVIEW_REPORT.md` records the lifecycle/options gaps.
- [x] Shared, PE Loader, Sleigh Runtime, Decompiler, Function ID, and analyzer-module review reports remain linked from their module directories.
- [x] The three decompiler-backed analyzer adapters now reuse `newghidra::decompiler::make_x86_64_architecture()`; the legacy mutable analyzer context/domain migration remains open.
