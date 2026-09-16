# Strict Function ID API Audit

## Scope

- [x] Reviewed [`src/types.cppm`](src/types.cppm), [`src/hasher.cppm`](src/hasher.cppm), [`src/database.cppm`](src/database.cppm), the Function ID analyzer adapter, and shared analyzer inputs.
- [x] Compared program identity and call-neighborhood behavior with [`FidAnalyzer.java`](../../../Ghidra/Features/FunctionID/src/main/java/ghidra/feature/fid/analyzer/FidAnalyzer.java) and [`FidProgramSeeker.java`](../../../Ghidra/Features/FunctionID/src/main/java/ghidra/feature/fid/service/FidProgramSeeker.java).
- [x] Reviewer: Kilo.
- [x] Review date: 2026-09-15.
- [x] Review type: performance profile, implementation optimization, and compatibility validation.
- [x] Recommendations target the shared program identity/call-graph and scheduler APIs, not analyzer-local workarounds.

## Findings

### Critical

#### FID-CRITICAL-001: Repeated packed-database parsing dominated the test suite

- [x] Profiling completed before the optimization.
- [x] Bottleneck measured rather than inferred.
- Severity: critical performance finding for the Function ID test workflow.
- Environment: Windows `win32`, Visual Studio Developer Command Prompt `18.11.0-insiders`, MSVC `14.34.31933` x64, Ninja/CMake preserved build at `NEW/build`, Debug configuration (`/Od /RTC1` before scoped parser optimization), local checked-in `.fidb` files.
- Baseline: the requested benchmark for the five acceptance tests was `53.07 s`. Direct profiling of the complete test process measured `14` database opens and approximately `64.7 s` of `Database::open` time under profiling instrumentation; the acceptance-only group measured `18.87 s` after the first cache implementation but before suite warm-up.
- Source: `src/database.cppm:133-334`, `src/buffer_file.cppm:73-124`, `tests/function_id_tests.cpp:55-72` and `:197-220` before optimization.
- Affected component: packed FID database storage/parsing and the test harness's serialized independent database opens.
- Measured parser sections: raw read/unpack was `0.05-0.15 s` per database; master metadata was below `0.003 s`; strings were `0.35-0.83 s`; relations were `0.3-1.6 s`; Functions-table materialization was the dominant section at `1.4-11.6 s` per database depending on file and machine load. Hashing, Sleigh decoding, matching, and assertions were below the database parse cost.
- Measured record work: the largest database contained about `143,444` function records and `51,469` distinct full-hash index buckets. The fixed-record loop's string lookup/index/vector work was only roughly `10-25%` of its measured section; B-tree traversal and per-record decoding dominated.
- Estimated impact: repeated database parsing accounted for more than `90%` of the original acceptance-only runtime and approximately `64.7 s` in the instrumented complete-process profile.
- Why unnecessary: `.fidb` parsed tables are immutable after `Database::open`; tests and analyzer events repeatedly opened the same path set without changing file metadata. Independent database paths also had no ordering dependency during initial parsing.
- Fixes applied:
  - `Database` now stores immutable parsed state in `std::shared_ptr<const Storage>` and uses a process-local cache keyed by normalized path, size, and modification time.
  - `FunctionIdAnalyzer` reuses its parsed database vector and opens independent configured databases concurrently while retaining path-order result collection and failed-open behavior.
  - The acceptance/database test helpers open independent databases concurrently and warm the same four immutable inputs once, without removing any test or assertion.
  - Fixed-width FunctionID records are decoded directly after the existing 52-byte/schema validation instead of allocating a `vector<variant>` for every record.
  - B-tree cycle tracking uses bounded visitation storage and generic recursion instead of repeated ordered/set/type-erased lookup overhead; relation parsing runs concurrently with the independent Functions table.
  - MSVC parser translation units use scoped `#pragma optimize` under the repository's Debug `/Od` test configuration; no global build mode or analyzer behavior was disabled.
- Compatibility safeguards: cache entries invalidate on path/size/mtime changes; database ordering, schema validation, record traversal, hash/index construction, relation contents, matching/scoring, and all test assertions remain intact. A failed `runtime_checks` pragma experiment was reverted because it regressed runtime.
- Before/after: baseline acceptance benchmark `53.07 s`; final clean runs varied from `12.5-19.3 s` with machine load, with a best direct run of `11.05 s` and latest CTest validation at `15.42 s`. The sustained reduction is approximately `64-76%`; the strict `5-10 s` target was approached but not consistently reached on this Debug/MSVC environment.
- Remaining bottleneck: one cold parse of the largest database still approaches `~10-11 s`; subsequent opens are cache hits. Further reduction would require parallel B-tree subtree parsing or a Release/RelWithDebInfo benchmark, neither of which was introduced without a broader compatibility design.
- Compatibility experiment: a temporary per-mode `RelWithDebInfo` build was tested to separate compiler overhead from parser work; it produced a FunctionID test segmentation fault and was fully reverted. The repository remains on its normal Debug build path.
- Validation: [x] complete `function_id_tests` passed after every significant optimization stage; [x] final suite passed all tests; [x] no profiling output remains in production/test code; [x] focused behavior and matching assertions remain unchanged.

### High

#### FID-HIGH-001: Function ID queries use an incorrect language identity for supported PE machines

- [ ] Remediation complete.
- Severity: high.
- Title: `language_id` maps every non-amd64 PE machine to x86 32-bit.
- Source: [`../analyzers/function_id/src/function_id.cppm:79-82`](../analyzers/function_id/src/function_id.cppm#L79-L82), [`../pe_loader/src/pe_loader.cppm:88-113`](../pe_loader/src/pe_loader.cppm#L88-L113), symbols `language_id` and `pe::Machine`.
- Affected component: FID database filtering and matching for ARM, ARM64, RISC-V, MIPS, PowerPC, and every non-amd64 PE image.
- Technical evidence: the PE loader explicitly recognizes many machine values, but the analyzer returns only `x86:LE:64:default` for amd64 and `x86:LE:32:default` for all other machines. The original FID analyzer passes the program's actual `LanguageID` through `FidProgramID` at [`FidAnalyzer.java:138-145`](../../../Ghidra/Features/FunctionID/src/main/java/ghidra/feature/fid/analyzer/FidAnalyzer.java#L138-L145).
- Expected behavior: FID `ProgramInfo` must contain the actual language ID, compiler specification, and source-language filters for the loaded program.
- Actual behavior: an ARM64 PE is queried as x86 32-bit, so matching databases are rejected or unrelated x86 records can be considered when filters are ignored.
- Impact: valid library functions are missed, and forced/ignored-filter queries can produce architecture-invalid labels.
- Reproduction or failure scenario: load an ARM64 PE with a matching ARM64 `.fidb`; `language_id()` returns `x86:LE:32:default`, causing `Database::identify` to apply the wrong program filter.
- Root cause: the shared PE/analyzer boundary has no authoritative language/compiler identity; the adapter derives a two-case string from the machine enum.
- Recommended fix: add a validated program-language identity object at the loader/context boundary and pass its language ID, compiler spec, source languages, and pointer model into `fid::ProgramInfo`.
- Regression risks: x86 fixture IDs must remain byte-for-byte unchanged; add ARM, ARM64, and non-default compiler-spec filter tests.
- Relevant tests or validation: current Function ID analyzer tests use x86 fixtures only and do not exercise the other `pe::Machine` values.

#### FID-HIGH-002: The Function ID analyzer discards the required parent/child hash neighborhood

- [ ] Remediation complete.
- Severity: high.
- Title: Every analyzer query passes empty `children` and `parents` vectors.
- Source: [`../analyzers/function_id/src/function_id.cppm:146-164`](../analyzers/function_id/src/function_id.cppm#L146-L164), [`src/types.cppm:89-94`](src/types.cppm#L89-L94), symbols `FunctionIdAnalyzer::analyze` and `fid::FunctionContext`.
- Affected component: FID relation scoring, `force_relation` records, parent/child disambiguation, and duplicate library matches.
- Technical evidence: the adapter constructs `const fid::FunctionContext query{*hash, {}, {}}`. The public FID model has explicit `children` and `parents`, and the original `FidProgramSeeker` builds them from call references at [`FidProgramSeeker.java:117-139`](../../../Ghidra/Features/FunctionID/src/main/java/ghidra/feature/fid/service/FidProgramSeeker.java#L117-L139) and [`FidProgramSeeker.java:192-204`](../../../Ghidra/Features/FunctionID/src/main/java/ghidra/feature/fid/service/FidProgramSeeker.java#L192-L204).
- Expected behavior: each query includes the hashes of resolved callees and callers, subject to the original thunk/common-function rules, before database scoring.
- Actual behavior: relation scoring always sees empty neighborhoods; database records requiring a relation can be skipped and otherwise-distinguishable matches receive no child/parent score.
- Impact: FID labels differ from Ghidra precisely in the cases where relation evidence is intended to resolve collisions.
- Reproduction or failure scenario: query a database record with `force_relation` set or two equal body hashes distinguished by a child relation; the native query supplies no relation and cannot select the same result as Ghidra.
- Root cause: `AnalysisContext` exposes raw references and functions but no shared call-graph/hash-family service, and the analyzer adapter does not have a model-level way to obtain one.
- Recommended fix: add a shared call-graph view and FID hash-family builder that preserves caller/callee direction, thunk policy, unresolved names, and stable hash deduplication; pass that structured context to `Database::identify`.
- Regression risks: graph construction depends on current function/reference scheduling; test partial graphs, cycles, thunks, unresolved external calls, and relation-required records.
- Relevant tests or validation: current tests verify a byte-identical single-function match but do not assert parent/child scoring or `force_relation` behavior.

### Medium

#### FID-MEDIUM-001: FID application has no shared source-priority or label-modifier transaction

- [ ] Remediation complete.
- Severity: medium.
- Title: FID labels are applied through generic symbol/name setters without the original source and modifier semantics.
- Source: [`../analyzers/function_id/src/function_id.cppm:84-103`](../analyzers/function_id/src/function_id.cppm#L84-L103), [`../analyzers/shared/src/analyzer_context.cppm:1031-1049`](../analyzers/shared/src/analyzer_context.cppm#L1031-L1049), symbols `apply_match` and `AnalysisContext::add_symbol`.
- Affected component: imported/user labels, multiple FID matches, function renaming, bookmarks, and follow-up FID invalidation.
- Technical evidence: `apply_match` marks the first candidate primary and calls `set_function_name`; the shared symbol model has no `SourceType`, `alwaysApplyFidLabels` policy, or atomic `functionModifierChanged` equivalent. Original FID documents the label precedence policy at [`FidAnalyzer.java:53-58`](../../../Ghidra/Features/FunctionID/src/main/java/ghidra/feature/fid/analyzer/FidAnalyzer.java#L53-L58) and notifies the manager after changes at [`FidAnalyzer.java:156-160`](../../../Ghidra/Features/FunctionID/src/main/java/ghidra/feature/fid/analyzer/FidAnalyzer.java#L156-L160).
- Expected behavior: FID application must honor user/imported source priority, preserve all candidate aliases, and notify the scheduler with the exact modified function set.
- Actual behavior: generic primary/name mutation can demote unrelated primary symbols and emits a broad `function_changed` event without source classification or a modifier set.
- Impact: user or PDB names can be replaced or FID can be rerun unnecessarily; final labels are order-dependent.
- Reproduction or failure scenario: apply a FID result to a function with an existing imported symbol and run a second analysis pass; no source-priority field tells the model whether replacement is allowed.
- Root cause: label application is not a first-class shared symbol transaction.
- Recommended fix: add symbol source priority, candidate/primary identity, and a FID-specific apply transaction that returns the modified function set to the scheduler.
- Regression risks: existing multiple-match output must retain all candidate names and bookmark behavior; test user/imported/analysis precedence.
- Relevant tests or validation: current analyzer tests assert the final name but not preservation of higher-priority labels or exact rerun scope.

### Low

No findings.

## Verified Strengths

- [x] Packed FID database schema and score calculations are isolated from the analyzer context.
- [x] Hashing preserves instruction masks, operand objects, and relocation metadata for the supported runtime fixtures.
- [x] Database filtering has explicit language/compiler/source fields in [`src/types.cppm:80-87`](src/types.cppm#L80-L87); the defect is at the adapter that populates them.
- [x] Database opening failures are skipped without mutating analyzer state.
- [x] Immutable database caching is invalidated by configured path and file metadata changes.
- [x] Cache warm-up and parallel opening preserve the original database set and acceptance expectations.

## Reviewed Areas With No Findings

- [x] FNV hash and packed-record parsing paths covered by existing Function ID unit tests.
- [x] Duplicate database path collection and deterministic path ordering.
- [x] Relocation width filtering for the PE relocation types currently exercised by tests.

## Validation

- [x] `git diff --check` completed without whitespace errors.
- [x] `ctest --test-dir NEW/build -N` listed `function_id_tests` and `analyzer_function_id_tests`.
- [x] `NEW\build.bat function_id` passed the full Function ID suite after the final optimization.
- [x] `NEW\build.bat analyzer_global_integration` passed after the final FunctionID storage/cache changes (`37.58 s`).
- [x] Direct benchmark and per-database/parser-section profiling were performed; temporary profiling instrumentation was removed.
- [ ] `NEW\tidy.bat function_id --check` was not clean because repository baseline clang-tidy/IFC diagnostics remain; no tidy fixes were applied.

## Unresolved Questions And Residual Risks

- [ ] The repository currently has no authoritative mapping from `pe::Machine` plus SLA/compiler specification to a Ghidra `LanguageID`.
- [ ] The shared function/reference model must define whether unresolved named callees contribute to FID relation hashes, matching the original name fallback.

## Follow-Up Decision

- [x] Performance fixes were applied and validated; semantic compatibility findings FID-HIGH-001 and FID-HIGH-002 remain separate follow-up work.
- [ ] Highest-priority remaining semantic Function ID findings are FID-HIGH-001 and FID-HIGH-002; performance follow-up remains if the 5-10 second target becomes mandatory on this Debug toolchain.
