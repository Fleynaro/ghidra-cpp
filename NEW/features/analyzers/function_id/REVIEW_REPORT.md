# Strict Audit Report: Function ID

- [x] Scope confirmed: [`src/function_id.cppm`](src/function_id.cppm), [`tests/function_id_analyzer_tests.cppm`](tests/function_id_analyzer_tests.cppm), [`tests/data/run_ghidra.py`](tests/data/run_ghidra.py), [`tests/data/test_function_id.md`](tests/data/test_function_id.md), [`CMakeLists.txt`](CMakeLists.txt), the native FunctionID library, and the checked-in PE/FID fixtures.
- [x] Exact Ghidra sources inspected: [`FidAnalyzer.java`](../../../../Ghidra/Features/FunctionID/src/main/java/ghidra/feature/fid/analyzer/FidAnalyzer.java), [`ApplyFidEntriesCommand.java`](../../../../Ghidra/Features/FunctionID/src/main/java/ghidra/feature/fid/cmd/ApplyFidEntriesCommand.java), and [`FidService.java`](../../../../Ghidra/Features/FunctionID/src/main/java/ghidra/feature/fid/service/FidService.java), plus native [`database.cppm`](../../function_id/src/database.cppm) relation/filter logic.
- [x] Review date: 2026-09-15. No commit was supplied; the working tree was audited as-is.
- [x] Reviewer: Kilo.
- [x] Review method: source-to-source comparison, matching/markup inspection, fixture assertion inspection, and CMake/test-registration inspection.
- [ ] Native build, test, formatter, and tidy commands were not run because this was requested as a strict read-only audit.

## Findings

### Critical

No findings.

### High

#### HIGH-001: Analyzer lifecycle and full-executable gating do not match Ghidra

- [ ] Remediation status: open.
- Severity: high.
- Title: native Function ID runs on function events instead of Ghidra's full executable one-time pass.
- Exact references: [`src/function_id.cppm:107-187`](src/function_id.cppm#L107-L187), symbols `FunctionIdAnalyzer::descriptor` and `FunctionIdAnalyzer::analyze`; Ghidra [`FidAnalyzer.java:97-161`](../../../../Ghidra/Features/FunctionID/src/main/java/ghidra/feature/fid/analyzer/FidAnalyzer.java#L97-L161), especially `isFullExecutable` at lines 170-180.
- Affected component: candidate timing, function-body completeness, and repeat analysis.
- Technical evidence: Ghidra declares a byte analyzer with one-time semantics, verifies `FidService.canProcess`, requires the event set to contain the execute or loaded/initialized set, calls `ApplyFidEntriesCommand` once, and then notifies `functionModifierChanged`. Native triggers on `function_added`/`function_changed`, has no full-executable check, no `canProcess` gate, no thunk check, and does not notify downstream function modifiers.
- Expected behavior: identify functions only after the required executable set and proper body analysis are available, then run the command's post-markup invalidation/update behavior.
- Actual behavior: native can hash and label a partial function as soon as it is added and can rerun on every function change; thunks are not excluded by the analyzer.
- Impact: premature or unstable matches, repeated expensive database scans, and stale downstream analysis after labels change.
- Reproduction or failure scenario: emit a `function_added` event for a short/incompletely discovered function with a matching prefix; native considers it immediately if it meets its local instruction threshold, while Java defers until the full executable set.
- Root cause: the event-driven native descriptor has no equivalent of Ghidra's byte-analyzer one-time/full-set lifecycle and the native `Function` model lacks thunk/full-analysis state.
- Recommended fix: represent full-set/one-time analyzer state, `canProcess`, thunk exclusion, and downstream function-modifier notification before applying matches.
- Regression risks: delaying Function ID can change the order of label-dependent analyzers; add scheduler integration tests.
- Relevant tests or validation: [`tests/function_id_analyzer_tests.cppm:14-36`](tests/function_id_analyzer_tests.cppm#L14-L36) invokes `analyze` directly with no event set or partial-body case.

#### HIGH-002: Match filtering and markup semantics are substantially different

- [ ] Remediation status: open.
- Severity: high.
- Title: native `apply_match` labels low-confidence/multiple matches and overwrites trusted names without Ghidra's conflict analysis.
- Exact references: [`src/function_id.cppm:84-103`](src/function_id.cppm#L84-L103), symbol `apply_match`; [`src/function_id.cppm:163-184`](src/function_id.cppm#L163-L184), database aggregation; Ghidra [`ApplyFidEntriesCommand.java:110-153`](../../../../Ghidra/Features/FunctionID/src/main/java/ghidra/feature/fid/cmd/ApplyFidEntriesCommand.java#L110-L153), [`:243-285`](../../../../Ghidra/Features/FunctionID/src/main/java/ghidra/feature/fid/cmd/ApplyFidEntriesCommand.java#L243-L285), and [`:346-379`](../../../../Ghidra/Features/FunctionID/src/main/java/ghidra/feature/fid/cmd/ApplyFidEntriesCommand.java#L346-L379).
- Affected component: labels, plate comments, bookmarks, trusted symbol preservation, and conflicts.
- Technical evidence: Ghidra runs `MatchNameAnalysis`, refuses multiple conflicting names below `multiNameScoreThreshold`, applies labels only when no imported/user symbol exists unless `Always Apply FID Labels` is enabled, generates detailed comments/libraries, and applies conflict symbols/bookmarks. Native deduplicates names across databases, adds every limited name immediately, hard-codes `30.0F` for deciding only the primary function name, always adds a Function ID bookmark when enabled, and never checks trusted/imported symbols or creates conflict labels/comments.
- Expected behavior: the same score thresholds, name normalization, trusted-symbol policy, comments, multiple-match labels, conflict cleanup, and bookmark categories must be preserved.
- Actual behavior: low-score multiple matches can still add native symbols/bookmarks, and a user/imported function name can be replaced through `set_function_name` regardless of the Java option.
- Impact: incorrect labels and loss of trusted program metadata are user-visible and can mislead all later analysis.
- Reproduction or failure scenario: create two equal-name-different-library matches below the multi-name threshold, or seed an imported symbol at the function entry. Native still adds match symbols/bookmarks and can rename the function; Java returns before markup or skips it unless forced.
- Root cause: `ApplyFidEntriesCommand`'s `MatchNameAnalysis` and option set were collapsed into a simple names vector and hard-coded score check.
- Recommended fix: port name/library/conflict analysis and expose `Always Apply FID Labels`, multi-match threshold, ignore-filter, and comment/bookmark policy options in `AnalysisContext`.
- Regression risks: native symbol primary replacement and existing analyzer events must be coordinated with conflict cleanup.
- Relevant tests or validation: [`tests/function_id_analyzer_tests.cppm:14-36`](tests/function_id_analyzer_tests.cppm#L14-L36) covers one single exact match only; no trusted/multiple/low-score case exists.

#### HIGH-003: Query context omits call-neighborhood and database-filter facts

- [ ] Remediation status: open.
- Severity: high.
- Title: native always queries with empty parent/child hashes and null compiler/source filters.
- Exact references: [`src/function_id.cppm:136-168`](src/function_id.cppm#L136-L168), especially line 163; native [`../../function_id/src/types.cppm:89-94`](../../function_id/src/types.cppm#L89-L94) and [`../../function_id/src/database.cppm:305-345`](../../function_id/src/database.cppm#L305-L345); Ghidra [`FidService.java:164-170`](../../../../Ghidra/Features/FunctionID/src/main/java/ghidra/feature/fid/service/FidService.java#L164-L170) and [`:217-222`](../../../../Ghidra/Features/FunctionID/src/main/java/ghidra/feature/fid/service/FidService.java#L217-L222).
- Affected component: relation-constrained matching and compiler/source database filters.
- Technical evidence: native constructs `fid::FunctionContext{*hash, {}, {}}` and `ProgramInfo{language, nullopt, nullopt, false}`. The existing native database explicitly scores `children`/`parents`, rejects `force_relation` candidates with no related units, and applies compiler/source filters when present. Ghidra's `FidProgramSeeker`/`FidService.processProgram` supplies the complete program context and selected filter policy.
- Expected behavior: candidate scoring and filtering must include hashed call parents/children and the program's compiler/source/ignore-filter state.
- Actual behavior: relation-required records are skipped or scored without their relation contribution, while libraries with compiler/source mismatches can pass because native supplies no corresponding filter values.
- Impact: false negatives for relation-constrained libraries and false positives across compiler/source variants.
- Reproduction or failure scenario: query a database record with `force_relation` or a compiler-specific library. Native's empty neighborhoods/null filters produce a different result from Java's seeker.
- Root cause: the analyzer adapter hashes only each individual function and does not build a program call graph or populate `ProgramInfo` options.
- Recommended fix: build the required call-neighborhood hashes, propagate compiler/source metadata and ignore-filter options, and test forced relation/filter cases.
- Regression risks: neighborhood hashing can be expensive and must be bounded like Ghidra's seeker.
- Relevant tests or validation: [`tests/function_id_analyzer_tests.cppm:16-29`](tests/function_id_analyzer_tests.cppm#L16-L29) uses a database with a byte-identical single-match fixture and no relation/filter assertion.

### Medium

#### MEDIUM-001: The fixture report is Java evidence and native CTest coverage is single-path

- [ ] Remediation status: open.
- Severity: medium.
- Title: generated Function ID Markdown does not validate the native analyzer and current native tests omit option/negative paths.
- Exact references: [`tests/function_id_analyzer_tests.cppm:14-47`](tests/function_id_analyzer_tests.cppm#L14-L47); [`tests/data/run_ghidra.py:305-347`](tests/data/run_ghidra.py#L305-L347); [`tests/data/test_function_id.md:21-33`](tests/data/test_function_id.md#L21-L33) and [`:120-124`](tests/data/test_function_id.md#L120-L124); [`CMakeLists.txt:7-13`](CMakeLists.txt#L7-L13).
- Affected component: parity evidence and regression infrastructure.
- Technical evidence: the Python script creates/populates a Ghidra packed FID database through Java `FidService`, runs Ghidra `FidAnalyzer`, then directly invokes Java `FidAnalyzer`/`ApplyFidEntriesCommand` paths. The C++ test opens the same artifact through the native database and checks one label/bookmark plus disabled behavior. It does not test trusted labels, multiple matches, score thresholds, relation filters, thunks, or full-set scheduling; CMake does not register the Python report generation.
- Expected behavior: native integration tests should observe native labels/comments/bookmarks under each public option and the Java report should be clearly marked as reference-only unless wired into CI.
- Actual behavior: the report's two Java labels/comments and direct Java query counts cannot prove native `apply_match` behavior; the native test can pass while high-severity markup gaps remain.
- Impact: the most important behavior differences are not regression-protected.
- Reproduction or failure scenario: remove native trusted-symbol checking or relation context; the two native tests still pass.
- Root cause: one byte-identical single-match fixture was used as the complete analyzer contract.
- Recommended fix: add native multi-match/trusted/filter/relation/partial-body tests and register reference fixture generation through the required wrapper if retained.
- Regression risks: packed FID fixture generation is version-sensitive; keep a stable native artifact plus structural assertions.
- Relevant tests or validation: source inspection only; no commands were run in this audit.

#### MEDIUM-002: Native public options do not cover Ghidra's Function ID options

- [ ] Remediation status: open.
- Severity: medium.
- Title: score, multi-match, trusted-label, ignore-filter, and source/compiler options are not represented by the analyzer context.
- Exact references: [`../../shared/src/analyzer_types.cppm:315-363`](../shared/src/analyzer_types.cppm#L315-L363), `AnalysisOptions`; [`src/function_id.cppm:89-101`](src/function_id.cppm#L89-L101); Ghidra [`FidAnalyzer.java:42-81`](../../../../Ghidra/Features/FunctionID/src/main/java/ghidra/feature/fid/analyzer/FidAnalyzer.java#L42-L81) and [`:182-213`](../../../../Ghidra/Features/FunctionID/src/main/java/ghidra/feature/fid/analyzer/FidAnalyzer.java#L182-L213).
- Affected component: user configuration and reproducible analysis behavior.
- Technical evidence: Ghidra registers five Function ID-specific options. Native exposes only generic `function_id_minimum_instructions`, `function_id_maximum_matches`, `create_analysis_bookmarks`, and database paths; `apply_match` hard-codes the multiple threshold and has no trusted-label/ignore-filter option.
- Expected behavior: every behavior-changing Java option must have an equivalent native value or be explicitly rejected as unsupported.
- Actual behavior: changing any missing Java option has no native effect because no corresponding state exists.
- Impact: users cannot reproduce or control native results using the documented Ghidra option contract.
- Root cause: the adapter reused a minimal shared options structure designed for the simple fixture.
- Recommended fix: add typed Function ID options and pass them to database identification/markup logic.
- Regression risks: defaults must match `FidService` and old option names should not be silently ignored.
- Relevant tests or validation: no current test enumerates or changes the missing option set.

### Low

No findings.

## Verified Strengths

- [x] Native reuses the existing packed FID parser, hash implementation, relocation model, and score calculation instead of duplicating those subsystems.
- [x] The direct native fixture uses a real packed `.fidb` artifact and a byte-identical function body.
- [x] Disabled-option behavior is explicitly tested.

## Reviewed Areas With No Findings

- [x] The native analyzer's basic single-match label/bookmark path is internally coherent for the checked fixture.
- [x] The local CMake target and GoogleTest registration are internally consistent.

## Validation

- [x] Original analyzer/command/service, native adapter/database relation logic, native tests, fixture script/report, and CMake registration were inspected.
- [ ] Focused build/test was not run by this audit.
- [ ] `NEW\format.bat analyzer` and `NEW\tidy.bat analyzer --check` were not run by this audit.
- [x] No implementation, test, fixture, or build source was edited.

## Unresolved Questions And Residual Risks

- [ ] Native Function ID must decide whether the existing shared event scheduler will support a true one-time/full-executable byte-analyzer phase.
- [ ] The checked-in `.fidb` artifact is generated by a Ghidra runtime version and does not exercise every schema/filter/relationship combination.

## Follow-Up Decision

- [ ] Highest-priority open items are `HIGH-001`, `HIGH-002`, and `HIGH-003`; `MEDIUM-001` must be addressed before the fixture report is treated as native proof.
