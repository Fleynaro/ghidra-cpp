# Services Module Review Report

## Review Metadata

- [x] **Scope:** exactly `HEAD~2..HEAD` (`778c5d87ad` and `3c123d1fda`), covering PE, Sleigh, decompiler, Function ID, analyzers, translation-engine ownership, service CMake, tests, and fixtures.
- [x] **Date:** 2026-09-16.
- [x] **Reviewer:** Kilo, independent read-only pass.
- [x] **Assumption:** native algorithms may remain private compatibility implementations, but runtime-facing calls must honor core contracts and service lifetime rules.

## Review Status

- [x] Scope confirmation.
- [x] Service source, CMake, dependency, fixture, and test inspection.
- [x] Core contract conversion inspection.
- [x] Validation status recorded.
- [x] No implementation fixes applied.

## Findings: Critical

### CRITICAL-001: Queued service tasks can call destroyed service objects

- [ ] **Remediation status:** Open.
- **References:** `decompiler/decompiler_service.cppm:89-103`, `sleigh/sleigh_service.cppm:48-72`, `function_id/function_id_service.cppm:65-80`.
- **Affected component:** Asynchronous service adapters.
- **Technical evidence:** Each queued lambda captures raw `[this]`, while the returned `Task` does not own the service. Runtime project shutdown can release these services before the worker drains the queue.
- **Expected behavior:** Tasks own a service-state lease or cannot outlive the service.
- **Actual behavior:** A worker may invoke a method on freed service storage.
- **Impact:** Use-after-free and shutdown crashes.
- **Failure scenario:** Queue decompilation or batch decoding behind a busy worker, close the project, then wait for the queued task.
- **Root cause:** Service and task lifetimes are independent.
- **Recommended fix:** Capture shared state and enforce a project shutdown cancellation/drain barrier.
- **Regression risks:** Changed task and close semantics.
- **Relevant validation:** No service lifetime race test exists.

## Findings: High

### HIGH-001: Decompiler adapter ignores the configured architecture and SLA resource

- [ ] **Remediation status:** Open.
- **References:** `decompiler/decompiler_service.cppm:80-87`, `decompiler/decompiler_service.cppm:105-134`, `decompiler/decompiler_service.cppm:191-194`, `../runtime/project/project_session.cppm:155-156`.
- **Affected component:** Decompiler service contract adapter.
- **Technical evidence:** `sla_path_` is stored but unused; `decompile_now` creates fixed x86-64 spaces, registers, pointer size, and `__cdecl` metadata.
- **Expected behavior:** Native provider metadata derives from the project architecture and selected SLA, or unsupported architectures fail explicitly.
- **Actual behavior:** Non-x86 and 32-bit projects use x86-64 semantics, and a custom SLA does not affect native decompiler setup.
- **Impact:** Incorrect decompilation semantics and misleading successful results.
- **Failure scenario:** Load ARM/32-bit metadata and request decompilation; the native adapter still uses x86-64 registers and calling convention.
- **Root cause:** Fixture-specific architecture was hard-coded in the adapter.
- **Recommended fix:** Translate canonical architecture/resource metadata into native provider objects and add unsupported diagnostics.
- **Regression risks:** Existing x86 output must remain compatible.
- **Relevant validation:** Only x86-64 happy-path fixtures are used.

### HIGH-002: Moved feature analyzers are not service implementations consumed by runtime

- [ ] **Remediation status:** Open.
- **References:** `analyzers/CMakeLists.txt:1-15`, `analyzers/CMakeLists.txt:68-74`, `analyzers/analyzer_builtin.cpp:34-72`, `../runtime/project/project_session.cppm:55-57`.
- **Affected component:** Analyzer feature-to-service migration.
- **Technical evidence:** The moved analyzer libraries still link `NewGhidra::PeLoader`, `NewGhidra::SleighRuntime`, and `NewGhidra::DecompilerFrontend` and implement the old mutable `AnalysisContext`/`AutoAnalysisManager` model. The new analyzer service target contains only `EntryMaterializationAnalyzer`; project startup registers only that new `IAnalyzer`.
- **Expected behavior:** Migrated analyzers consume immutable core snapshots/contracts and are registered by runtime analysis.
- **Actual behavior:** The physical move hides the old feature coupling; the default runtime does not execute the moved feature analyzers.
- **Impact:** Alternate service implementations cannot be substituted and feature analysis is absent from the facade path.
- **Failure scenario:** Provide a mock `IPCodeDecoder` or call facade analysis expecting the moved analyzer set; native legacy targets/registrations remain in use or are not invoked.
- **Root cause:** Directory relocation preceded extraction of analyzer algorithms from the legacy aggregate/context.
- **Recommended fix:** Introduce contract-facing analyzer adapters and register them through `NewGhidra::AnalyzerServices`/runtime registry; keep legacy aggregate only as an explicit diagnostic target.
- **Regression risks:** Priority/prerequisite and mutation behavior require parity coverage.
- **Relevant validation:** Existing tests exercise legacy analyzer targets individually, not facade registration/substitution.

### HIGH-003: Decompiler fallback is reported as a complete result

- [ ] **Remediation status:** Open.
- **References:** `decompiler/decompiler_service.cppm:156-181`, `../runtime/project/tests/project_tests.cppm:128-129`.
- **Affected component:** Decompiler service status contract.
- **Technical evidence:** Native provider failures enter `fallback()`, which emits assembly comments and sets `DecompilationStatus::complete`.
- **Expected behavior:** Degraded listing output has a distinct status.
- **Actual behavior:** Native failure can be consumed as a complete semantic result.
- **Impact:** Downstream consumers may persist or analyze comment-only output as native decompilation.
- **Failure scenario:** Force a native flow/provider exception and inspect the returned status.
- **Root cause:** Fallback reused the success status.
- **Recommended fix:** Add a partial/fallback status and test native versus fallback paths.
- **Regression risks:** Existing callers must handle degraded results.
- **Relevant validation:** Current integration tests assert only complete/non-empty output.

## Findings: Medium

### MEDIUM-001: Service aggregate and no-test build mode do not cover the migrated service targets

- [ ] **Remediation status:** Open.
- **References:** `CMakeLists.txt:13-24`, `../build.bat:257-263`.
- **Affected component:** Service CMake target graph.
- **Technical evidence:** `new_ghidra_services` depends on legacy engine targets but omits the public adapter libraries and translation engine. The `services --no-test` wrapper assignment is overwritten with `new_ghidra_service_tests`, which is not defined when `BUILD_TESTING=0`.
- **Expected behavior:** The aggregate builds every public service and the no-test mode builds that aggregate.
- **Actual behavior:** Compile-only service validation can omit adapters or request a disabled test target.
- **Impact:** Migration target health is not reliably validated by the prescribed workflow.
- **Failure scenario:** Invoke `NEW\build.bat services --no-test`.
- **Root cause:** Target-name migration was not synchronized between CMake and wrapper logic.
- **Recommended fix:** Add all service targets to the aggregate and remove the later test-target override.
- **Regression risks:** Focused build time and target dependency ordering.
- **Relevant validation:** No wrapper-mode execution was performed.

### MEDIUM-003: Translation-engine target is nominal and does not own shared native semantics

- [ ] **Remediation status:** Open.
- **References:** `translation_engine/CMakeLists.txt:1-19`, `translation_engine/native/*.cppm`, `sleigh/sleigh_service.cppm:3-6`, `decompiler/decompiler_service.cppm:3-6`, `decompiler/CMakeLists.txt:103-127`.
- **Affected component:** Sleigh/decompiler native dependency boundary.
- **Technical evidence:** Translation modules are aliases to core values plus metadata; Sleigh imports `sleigh_runtime`, while decompiler imports `decompiler` and its frontend links `SleighRuntime`. The target does not contain the common native address/space/translate/load-image implementation described by its documentation.
- **Expected behavior:** One shared native substrate is consumed by both services, or the target is explicitly metadata-only.
- **Actual behavior:** Duplicate native implementation ownership remains behind a nominal shared target.
- **Impact:** Semantic drift and non-substitutable service dependencies remain possible.
- **Failure scenario:** Change shared translation metadata or supply an alternate translation implementation; service behavior is unchanged because their concrete imports bypass it.
- **Root cause:** Physical target creation preceded native extraction.
- **Recommended fix:** Consolidate common native modules into the target and import them from both services, or narrow/document the target contract.
- **Regression risks:** Native module duplicate symbols and ordering.
- **Relevant validation:** No test proves both services use shared native translation code.

### MEDIUM-004: Function ID adapter drops relation context and uses unmanaged async work

- [ ] **Remediation status:** Open.
- **References:** `function_id/function_id_service.cppm:84-121`, especially line 114; `function_id/src/database.cppm:266-278`; `analyzers/function_id/src/function_id.cppm:212-231`.
- **Affected component:** Function ID matching and resource scheduling.
- **Technical evidence:** `FunctionContext` is built with empty parent/inferior relation sets. The moved database and legacy analyzer paths use `std::async`, outside the bounded project worker pool.
- **Expected behavior:** Relation-aware scoring receives the project function graph and expensive database work uses runtime-owned scheduling.
- **Actual behavior:** Relation scoring is disabled and database fan-out bypasses cancellation/fairness/lifecycle control.
- **Impact:** Match ranking regressions and thread oversubscription or late work after project close.
- **Failure scenario:** Match a function with known call-graph relations or load multiple databases concurrently.
- **Root cause:** Hash/database adaptation was implemented without project-graph and runtime-resource integration.
- **Recommended fix:** Build relation families from `IProjectQuery` and route warm-up/fan-out through runtime resources/workers.
- **Regression risks:** FID parity and timing behavior.
- **Relevant validation:** Valid packed-database tests do not cover relations or worker ownership.

### MEDIUM-005: PE and Sleigh adapters do not preserve all canonical fields

- [ ] **Remediation status:** Open.
- **References:** `pe_loader/pe_loader_service.cppm:121-147`, `sleigh/sleigh_service.cppm:86-118`, `../core/contracts/pe_loader.cppm:35-43`, `../core/domain/operand.cppm:23-31`.
- **Affected component:** Service-to-core DTO conversion.
- **Technical evidence:** PE `make_result` puts imports/exports only into textual details and leaves structured symbol/relocation vectors empty. Sleigh conversion reports only `x86`/`unknown`, stores address operands as generic scalars, and does not fill canonical address/register fields.
- **Expected behavior:** All observable structured facts promised by core contracts are mapped or explicitly rejected.
- **Actual behavior:** Downstream consumers receive incomplete architecture, symbol, relocation, and operand values.
- **Impact:** Reference, architecture, FID, and decompiler behavior can silently degrade.
- **Failure scenario:** Load an imported/exported PE or decode an address operand and inspect the canonical result fields.
- **Root cause:** Adapter copied only the fields needed by the x86 happy-path fixtures.
- **Recommended fix:** Complete field mapping with explicit address-space/register rules and diagnostics for unsupported source data.
- **Regression risks:** Existing serialized fixture expectations.
- **Relevant validation:** Service tests assert only regions/architecture and one x86 `ret` decode.

### MEDIUM-007: Function ID malformed input escapes the `Result` contract

- [ ] **Remediation status:** Open.
- **Reference:** `function_id/function_id_service.cppm:36-44`, `FunctionIdDatabaseService::query`.
- **Affected component:** Public Function ID database contract.
- **Technical evidence:** `std::stoull(hashes.full_hash, nullptr, 16)` is not caught or validated in a method returning `core::Result`.
- **Expected behavior:** Invalid or oversized hash text returns a structured core diagnostic.
- **Actual behavior:** `std::invalid_argument` or `std::out_of_range` escapes.
- **Impact:** Callers bypass normal error handling and may terminate a task unexpectedly.
- **Failure scenario:** Query with `full_hash = "not-hex"`.
- **Root cause:** Native parser preconditions were assumed at the contract edge.
- **Recommended fix:** Use bounded `from_chars` conversion and map failures to `core::Error`.
- **Regression risks:** Hash normalization compatibility.
- **Relevant validation:** No malformed contract-input test.

## Findings: Low

### No findings

## Verified Strengths

- [x] Source, tests, CLIs, compiled-SLA resources, FID databases, and analyzer fixtures are physically represented under `services`.
- [x] PE/Sleigh/decompiler runtime-facing adapters use core values rather than exposing native pointers.
- [x] Sleigh access is mutex-protected and batch results are deterministic by address.
- [x] Focused native parity tests remain registered after relocation.

## Reviewed Areas With No Findings

- [x] PE parser internals and PE fixture relocation, apart from the service DTO adapter.
- [x] Sleigh native parser/decoder internals and compiled-SLA fixture relocation, apart from the service DTO adapter.
- [x] Decompiler native algorithm tests and Function ID valid-database tests, apart from service integration gaps.
- [x] Analyzer fixture paths and individual legacy analyzer test registrations.

## Validation Results

- [x] Service source/CMake/dependency/test registration inspected read-only.
- [x] Existing generated CTest metadata and build target metadata inspected.
- [x] `ctest --test-dir NEW/build --output-on-failure`: 49/49 passed.
- [x] `git diff HEAD~2..HEAD --check`: passed.
- [ ] No sanitizer, alternate-provider, non-x86, malformed-input, or destruction-race validation was run.

## Unresolved Questions And Residual Risks

- [ ] Decide whether moved legacy analyzer libraries are compatibility diagnostics or production services.
- [ ] Complete translation-engine extraction before claiming one native substrate.
- [ ] Define resource ownership and service leases for all queued operations.

## Follow-Up Decision

- [ ] Service findings `CRITICAL-001`, `HIGH-001` through `HIGH-003`, and `MEDIUM-001`, `MEDIUM-003` through `MEDIUM-005`, and `MEDIUM-007` remain open.
- [ ] No fixes were authorized or applied.
