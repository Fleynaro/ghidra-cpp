# Strict Cross-Boundary Audit Report

## Scope

- [x] Reviewed the current staged analyzer worktree as of 2026-09-15; no implementation or configuration files were changed by this audit.
- [x] Reviewed [`src/analyzer_types.cppm`](src/analyzer_types.cppm), [`src/analyzer_context.cppm`](src/analyzer_context.cppm), [`src/analyzer_manager.cppm`](src/analyzer_manager.cppm), [`src/analyzer_base.cppm`](src/analyzer_base.cppm), and [`src/analyzer_registry.cppm`](src/analyzer_registry.cppm).
- [x] Traced the shared API into the requested analyzer set under [`../`](../), plus the PE Loader, Sleigh Runtime, Decompiler, and Function ID public APIs.
- [x] Compared behavior with the original Ghidra contracts in `Ghidra/Features/Base`, `Ghidra/Features/Decompiler`, `Ghidra/Features/PDB`, `Ghidra/Features/FunctionID`, and `Ghidra/Framework/SoftwareModeling`.
- [x] Reviewer: Kilo.
- [x] Review method: source inspection, caller tracing, original-source comparison, `git diff --check`, environment verification, and CTest registration inspection.
- [x] Recommendations below are model, API, or scheduler changes. No analyzer-local workaround is proposed.

## Findings

### Critical

No findings.

### High

#### SHARED-HIGH-001: The listing model permits simultaneous code and data at one address

- [ ] Remediation complete.
- Severity: high.
- Title: `define_instruction` and `disassemble_flow` do not consult existing data units.
- Source: [`src/analyzer_context.cppm:551-565`](src/analyzer_context.cppm#L551-L565), [`src/analyzer_context.cppm:570-600`](src/analyzer_context.cppm#L570-L600), [`src/analyzer_context.cppm:607-635`](src/analyzer_context.cppm#L607-L635), symbols `AnalysisContext::disassemble`, `AnalysisContext::disassemble_flow`, and `AnalysisContext::define_instruction`.
- Affected component: `Listing` code/data conflict handling, function creation, and all reference analyzers.
- Technical evidence: `add_data` rejects ranges intersecting `instructions_` at lines 972-981, but the reverse path never checks `data_`. `disassemble()` can therefore define an instruction at an address already present in `data_`, and `create_function()` can then use it as code.
- Expected behavior: a listing must contain one compatible code unit or data unit for an address range. Ghidra's `DisassembleCommand` only creates instructions at undefined locations and `Listing` rejects overlapping code units.
- Actual behavior: `context.add_data(DataObject{address, ...})` followed by `context.disassemble_flow(address)` retains both objects and emits both data and code events.
- Impact: analyzers receive contradictory state, functions can be built over data, and reference consumers cannot determine which code unit owns the bytes.
- Reproduction or failure scenario: create a mapped executable fixture, add a data object covering an executable address, call `disassemble_flow` at that address, then call `create_function`; both `data().contains(address)` and `instructions().contains(address)` are true.
- Root cause: conflict validation is implemented only in the data insertion direction; there is no shared code-unit arbitration operation.
- Recommended fix: make code-unit insertion and replacement go through one model-level conflict transaction that applies Ghidra's clear/reject policy, emits removal/change events, and never publishes overlapping units.
- Regression risks: changing conflict policy affects string, media, address-table, and aggressive-disassembly behavior; preserve the original initialized/undefined distinction and test reject versus clear semantics separately.
- Relevant tests or validation: no current shared test exercises `add_data` followed by `disassemble_flow`; [`Ghidra/Features/Base/src/main/java/ghidra/app/cmd/disassemble/DisassembleCommand.java`](../../../../Ghidra/Features/Base/src/main/java/ghidra/app/cmd/disassemble/DisassembleCommand.java) only disassembles undefined starts.

#### SHARED-HIGH-002: Reanalysis cannot remove or replace code, data, or references consistently

- [ ] Remediation complete.
- Severity: high.
- Title: The mutable context has add-only listing/reference state and no replayable deletion contract.
- Source: [`src/analyzer_context.cppm:929-1003`](src/analyzer_context.cppm#L929-L1003), [`src/analyzer_context.cppm:1323-1332`](src/analyzer_context.cppm#L1323-L1332), symbols `AnalysisContext::add_reference`, `AnalysisContext::add_data`, and `AnalysisContext::remove_function`.
- Affected component: reanalysis, code/data conflict repair, reference invalidation, and event-driven downstream analyzers.
- Technical evidence: the only removal mutator is `remove_function`; references and instructions have no removal operation, and data replacement at the same address overwrites the map entry without a `data_removed` or `data_changed` event. `strings_` is never reconciled when its backing `DataObject` is replaced.
- Expected behavior: clearing or replacing a Ghidra code unit removes dependent references and stale derived records, then schedules the corresponding removal/change events. A repeated analysis pass must be able to converge from the current state.
- Actual behavior: a larger `DataObject` can replace an existing object while old `StringRecord` entries remain; references retain indices into instruction records that cannot be removed; no event can tell consumers that the previous unit ceased to exist.
- Impact: stale symbols, strings, references, and function bodies survive reanalysis; conflict resolution is order-dependent and cannot reproduce `Listing.clearCodeUnits` behavior.
- Reproduction or failure scenario: add a string at `A`, replace its data object at `A` with a larger non-string object, then inspect `strings()` and `data()`; the string record remains although its listing object no longer describes a string. A reference cannot be removed at all.
- Root cause: the state model stores derived records in independent containers but lacks identity, dependency ownership, and transactional removal events.
- Recommended fix: add model-level code-unit/reference identities and transactional clear, replace, and remove operations with complete event coverage; reanalysis should replay current facts and removals, not only append `*_added` events.
- Regression risks: removal changes can trigger analyzer reruns and alter body carving; retain stable event ordering and test cancellation after partial transactions.
- Relevant tests or validation: `AutoAnalysisManager::re_analyze_all` at [`src/analyzer_manager.cppm:247-345`](src/analyzer_manager.cppm#L247-L345) replays only existing added-style facts and cannot replay a removed code/data/reference object.

#### SHARED-HIGH-003: Function ID scheduling lacks the original one-time/full-executable contract

- [ ] Remediation complete.
- Severity: high.
- Title: The scheduler has no one-time analyzer or analysis-set readiness state.
- Source: [`src/analyzer_types.cppm:401-407`](src/analyzer_types.cppm#L401-L407), [`src/analyzer_base.cppm:25-87`](src/analyzer_base.cppm#L25-L87), [`../function_id/src/function_id.cppm:107-184`](../function_id/src/function_id.cppm#L107-L184), symbols `AnalyzerDescriptor`, `Analyzer`, and `FunctionIdAnalyzer`.
- Affected component: Function ID execution frequency, stable call-neighborhood matching, and analyzer scheduling semantics.
- Technical evidence: `AnalyzerDescriptor` has only name, priority, triggers, and prerequisites. `FunctionIdAnalyzer` triggers on every `function_added` and `function_changed` event but scans every function in the context. The original [`FidAnalyzer.java`](../../../../Ghidra/Features/FunctionID/src/main/java/ghidra/feature/fid/analyzer/FidAnalyzer.java#L97-L160) declares one-time analysis, requires a full executable/initialized set, and notifies the manager after label changes.
- Expected behavior: a byte analyzer with one-time semantics runs once after the required address set is complete, with an explicit program analysis set and lifecycle state.
- Actual behavior: every function creation or signature/name change can rescan all functions, and applying a name emits `function_changed`, which schedules the FID analyzer again. The context does not expose whether the full executable set has arrived.
- Impact: repeated expensive database scans, unstable results while call graphs are incomplete, and possible event churn up to `maximum_events`.
- Reproduction or failure scenario: register the built-in pipeline, let FID label one function, and observe the emitted `function_changed` event. The same FID analyzer is eligible again and hashes the whole function map.
- Root cause: one-time analysis, full-set readiness, and post-application modifier notifications were omitted from the scheduler contract.
- Recommended fix: extend the shared analyzer lifecycle and scheduler with analyzer type/one-time state, address-set readiness, and explicit modifier-change invalidation semantics equivalent to `AutoAnalysisManager.functionModifierChanged`.
- Regression risks: changing execution eligibility can reorder FID relative to PDB, function discovery, and call-convention analyzers; test partial memory events, full-set events, and name-change invalidation.
- Relevant tests or validation: [`../function_id/tests/function_id_analyzer_tests.cppm`](../function_id/tests/function_id_analyzer_tests.cppm) invokes the analyzer directly and does not test repeated manager scheduling.

#### SHARED-HIGH-004: The shared function signature cannot commit structured decompiler or PDB results

- [ ] Remediation complete.
- Severity: high.
- Title: `Function` and `set_function_signature` retain only flattened text and single-location parameters.
- Source: [`src/analyzer_types.cppm:125-168`](src/analyzer_types.cppm#L125-L168), [`src/analyzer_context.cppm:1141-1160`](src/analyzer_context.cppm#L1141-L1160), symbols `FunctionParameter`, `Function`, and `AnalysisContext::set_function_signature`.
- Affected component: decompiler parameter ID, PDB/DIA signatures, calling-convention analysis, and source-priority conflict handling.
- Technical evidence: parameters contain one string storage plus offset/size; the function has no signature source, return storage, hidden return storage, split parameter pieces, custom storage, or commit source. The decompiler public API already accepts split parameter/return pieces in [`decompiler.cppm:255-285`](../../decompiler/src/decompiler.cppm#L255-L285), but there is no shared commit boundary for them.
- Expected behavior: a recovered signature must preserve data types, storage pieces, return storage, hidden return parameters, source priority, varargs, and commit status, and must not overwrite higher-priority user/imported signatures.
- Actual behavior: callers must flatten types to strings and discard storage/source details; PDB Universal creates parameters with empty storage at [`../pdb_universal/src/pdb_universal.cppm:1012-1025`](../pdb_universal/src/pdb_universal.cppm#L1012-L1025).
- Impact: signatures that are textually plausible can be structurally wrong, ABI-split values cannot be represented, and later analyzers cannot apply Ghidra's source-priority rules.
- Reproduction or failure scenario: supply a prototype with a two-piece parameter or hidden structure-return storage through the decompiler API; there is no `AnalysisContext` operation that can persist it, so the information is lost before it reaches the program model.
- Root cause: the shared model was designed around display strings rather than the structured `FunctionSignature`/`VariableStorage` contract.
- Recommended fix: introduce a shared structured prototype/type commit model with explicit source priority and atomic application to functions, parameters, returns, variables, and call-fixup/no-return metadata.
- Regression risks: existing aggregate initializers and simple string-based analyzers need migration; preserve deterministic equality and avoid silently treating missing storage as a valid committed signature.
- Relevant tests or validation: [`Ghidra/Features/Decompiler/src/main/java/ghidra/app/cmd/function/DecompilerParameterIdCmd.java:217-230`](../../../../Ghidra/Features/Decompiler/src/main/java/ghidra/app/cmd/function/DecompilerParameterIdCmd.java#L217-L230) commits parameters and returns through `HighFunctionDBUtil`/`HighParamID`, behavior not represented by the shared setters.

### Medium

#### SHARED-MEDIUM-001: Symbol identity and function-name state can diverge

- [ ] Remediation complete.
- Severity: medium.
- Title: Symbol deduplication ignores namespace/mangled identity, and `set_function_name` does not update the symbol table.
- Source: [`src/analyzer_types.cppm:207-216`](src/analyzer_types.cppm#L207-L216), [`src/analyzer_context.cppm:1031-1049`](src/analyzer_context.cppm#L1031-L1049), [`src/analyzer_context.cppm:1163-1171`](src/analyzer_context.cppm#L1163-L1171), symbols `SymbolRecord`, `AnalysisContext::add_symbol`, and `AnalysisContext::set_function_name`.
- Affected component: demangling, Function ID aliases, PDB namespaces, primary symbol resolution, and function naming.
- Technical evidence: duplicate detection compares only address, `demangled_name`, and `kind`; it ignores mangled name, namespace, external flag, and primary identity. `set_function_name` changes only `Function.name`, although the original `Function.setName` is a symbol-table operation and can reject duplicate names.
- Expected behavior: aliases and overloaded/namespace-qualified symbols retain stable identity, and a function's primary name and symbol record change atomically.
- Actual behavior: distinct symbols can be discarded as duplicates, while a direct function rename leaves `symbols()` with the old primary name.
- Impact: downstream consumers see contradictory function and symbol names and cannot reproduce Ghidra's source/alias selection.
- Reproduction or failure scenario: add two same-address symbols with different mangled or namespace names but equal demangled name/kind; one is rejected. Call `set_function_name` without adding a symbol and inspect the unchanged symbol vector.
- Root cause: symbol records have no stable identity/alias relation and function naming is modeled as an independent string mutation.
- Recommended fix: add symbol identity, namespace-aware alias semantics, source priority, and an atomic function-symbol rename operation.
- Regression risks: Function ID multiple-match labels and PDB aliases must remain observable; test primary replacement, non-primary aliases, and user/imported name precedence.
- Relevant tests or validation: no current shared test covers namespace collisions or function-name/symbol-table consistency.

#### SHARED-MEDIUM-002: Reference records omit Ghidra provenance and reference subtype state

- [ ] Remediation complete.
- Severity: medium.
- Title: `Reference` cannot preserve primary/source/user-defined/external-reference identity.
- Source: [`src/analyzer_types.cppm:64-97`](src/analyzer_types.cppm#L64-L97), symbols `ReferenceKind` and `Reference`.
- Affected component: reference conflict resolution, scalar/data/reference analyzers, external entries, and repeated analysis.
- Technical evidence: the native record stores source, target, kind, operand, fallthrough, override, analysis-source, and stack offset, but no primary flag, `SourceType`, symbol ID, external namespace, entry-point subtype, shifted/offset subtype, or user-defined state. The original [`Reference.java`](../../../../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/model/symbol/Reference.java#L20-L126) exposes those distinctions.
- Expected behavior: a reference manager must distinguish user/imported/analysis references and preserve subtype identity when stronger references conflict or are removed.
- Actual behavior: `add_reference` can only suppress a speculative relation using a local boolean; it cannot preserve or replay the original source/provenance decision.
- Impact: reanalysis can duplicate or suppress the wrong reference, and external/data/entry references collapse into the same address-pair model.
- Reproduction or failure scenario: add two references with identical endpoints but different source provenance or primary status; the model has no representable distinction for the difference.
- Root cause: `Reference` was reduced to a relation plus analyzer provenance instead of the full `ReferenceManager` contract.
- Recommended fix: model reference identity, source priority, primary/user-defined state, address-space subtype, and removal/replacement semantics in the shared reference manager.
- Regression risks: existing duplicate suppression intentionally protects stronger references; preserve that behavior while making the strength explicit and replayable.
- Relevant tests or validation: current reference tests cover endpoint/kind behavior but not primary/source/subtype persistence.

#### SHARED-MEDIUM-003: PDB and datatype records are sidecar facts rather than applied program state

- [ ] Remediation complete.
- Severity: medium.
- Title: `PdbTypeRecord`, `PdbSymbolRecord`, and `DataArchiveRecord` cannot represent or apply the original type-manager side effects.
- Source: [`src/analyzer_types.cppm:218-264`](src/analyzer_types.cppm#L218-L264), [`src/analyzer_context.cppm:1095-1129`](src/analyzer_context.cppm#L1095-L1129), [`../apply_data_archives/src/apply_data_archives.cppm:24-45`](../apply_data_archives/src/apply_data_archives.cppm#L24-L45), symbols `PdbTypeRecord`, `DataArchiveRecord`, `add_pdb_type`, and `add_data_archive`.
- Affected component: Apply Data Archives, PDB Universal, PDB MSDIA, data definitions, and PDB function internals.
- Technical evidence: PDB types contain only name, kind, size, and string field pairs; archives contain path, names, and diagnostics. The archive analyzer explicitly records `Native DataTypeManagerService is unavailable; archive selection was recorded but not applied` and sets no data types. There is no type manager, type identity graph, enum/bitfield/namespace model, or application transaction.
- Expected behavior: opening an archive or applying a PDB updates the program datatype manager, data objects, function signatures, symbols, namespaces, and related bookmarks/reports.
- Actual behavior: the native context retains metadata sidecars while `data()` and function signatures remain unaffected by archive types; PDB symbol application uses flattened text and does not attach type identities.
- Impact: a successful analyzer pass can report retained PDB/archive evidence while omitting the user-visible type and signature changes that the original analyzer performs.
- Reproduction or failure scenario: run Apply Data Archives on a valid `.gdt`; `data_archives()` grows but no type manager or applied data definition changes. Add two same-named PDB types from distinct scopes; the second is rejected by name-only deduplication.
- Root cause: the shared state boundary has no autonomous datatype manager or type-application transaction.
- Recommended fix: add a standalone datatype/type-identity subsystem and an application transaction that links archive/PDB types to data, symbols, parameters, returns, and namespaces; preserve failed/partial application diagnostics.
- Regression risks: type resolution changes can alter decompiler output and data overlap decisions; keep archive provenance and source priority explicit.
- Relevant tests or validation: [`Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/analysis/ApplyDataArchiveAnalyzer.java:95-114`](../../../../Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/analysis/ApplyDataArchiveAnalyzer.java#L95-L114) invokes `ApplyFunctionDataTypesCmd`, while the native test only checks that a record was retained.

### Low

#### SHARED-LOW-001: Maximum-address one-byte instructions are rejected

- [ ] Remediation complete.
- Severity: low.
- Title: `define_instruction` checks `address + length` instead of the inclusive instruction end.
- Source: [`src/analyzer_context.cppm:35-41`](src/analyzer_context.cppm#L35-L41), [`src/analyzer_context.cppm:607-609`](src/analyzer_context.cppm#L607-L609), symbols `instruction_end` and `AnalysisContext::define_instruction`.
- Affected component: boundary-address decoding and complete address-space tests.
- Technical evidence: `instruction_end` correctly adds `length - 1`, but `define_instruction` rejects when `address > max - length`; for `length == 1`, an instruction beginning at `max` has a valid inclusive end and is rejected.
- Expected behavior: accept a one-byte instruction at the maximum address if the backing memory model permits it; reject only when the inclusive end overflows.
- Actual behavior: the valid one-byte boundary case returns `false`.
- Impact: a narrow address-space edge case is lost and behavior is inconsistent with the helper used elsewhere.
- Reproduction or failure scenario: call `define_instruction` with `address == UINT64_MAX` and `length == 1`; it fails before insertion.
- Root cause: two different overflow formulas are used for the same inclusive range contract.
- Recommended fix: use the checked inclusive-end helper for insertion validation and add an exact-boundary regression test.
- Regression risks: callers near the boundary must still reject lengths greater than one; test both cases.
- Relevant tests or validation: no current test covers maximum-address instruction insertion.

## Verified Strengths

- [x] Priority queue ordering uses the numerically lower Ghidra priority first at [`src/analyzer_manager.cppm:95-105`](src/analyzer_manager.cppm#L95-L105).
- [x] Prerequisite names and priority direction are validated before dispatch at [`src/analyzer_manager.cppm:118-145`](src/analyzer_manager.cppm#L118-L145).
- [x] `re_analyze_all` replays the newly introduced symbol/archive/PDB/external-entry event kinds for unrestricted passes at [`src/analyzer_manager.cppm:247-297`](src/analyzer_manager.cppm#L247-L297).
- [x] `add_address_table` validates multiplication before publishing its record at [`src/analyzer_context.cppm:1052-1073`](src/analyzer_context.cppm#L1052-L1073).
- [x] Flow override conversion preserves conditional versus unconditional call kind and restores the saved kind when cleared at [`src/analyzer_context.cppm:1227-1265`](src/analyzer_context.cppm#L1227-L1265).
- [x] The environment variable `GHIDRA_INSTALL_DIR` was present and pointed to `C:\Users\Fleynaro\Desktop\GTA-5-Android\ghidra_12.1.3_PUBLIC` during the audit.

## Reviewed Areas With No Findings

- [x] C++ module import syntax in the reviewed shared files.
- [x] Priority comparison direction and deterministic tie breaking.
- [x] Event coalescing of equal event kinds and removal flags.
- [x] Address-table publication order after checked data insertion.

## Validation

- [x] `git diff --check` completed without whitespace errors; Git emitted only existing line-ending warnings for unrelated staged reports.
- [x] `ctest --test-dir NEW/build -N` completed and listed 42 registered tests, including shared, Sleigh, PE Loader, Decompiler, Function ID, and analyzer tests.
- [ ] Build or test execution was not run because this was a strict read-only audit and executing the suite may write test logs or fixture outputs.
- [ ] `NEW/tidy.bat analyzer --check` was not run for the same read-only constraint.
- [ ] Runtime behavior against fresh fixtures was not newly executed; conclusions are source-contract findings, not claims of a passing implementation.

## Unresolved Questions And Residual Risks

- [ ] The intended native policy for clearing an existing data unit when code is discovered must be defined from the exact Ghidra command path used by each analyzer.
- [ ] The native datatype manager and source-priority taxonomy are not present in the reviewed API, so PDB/archive/decompiler findings may expand when that subsystem is designed.
- [ ] Multi-address-space and processor-context behavior needs architecture fixtures beyond the current PE/x86-focused analyzer set.
- [ ] Existing analyzer-local `REVIEW_REPORT.md` files were not treated as evidence that the shared contract is complete.

## Follow-Up Decision

- [ ] No fixes were authorized or applied in this read-only audit.
- [ ] Highest-priority remediation candidates are SHARED-HIGH-001, SHARED-HIGH-002, SHARED-HIGH-003, and SHARED-HIGH-004.
