# Independent Two-Commit Code Review

## Review Metadata

- [x] **Scope confirmed:** exactly `HEAD~2..HEAD`, commits `778c5d87ad` (`feat(architecture): add core runtime services and native facade`) and `3c123d1fda` (`feat(architecture): move features to services`).
- [x] **Review date:** 2026-09-16.
- [x] **Reviewer:** Kilo, independent read-only review.
- [x] **Scope areas:** feature-to-services migration, CMake graph, core contracts, service dependencies, tests/fixtures, lifecycle, persistence, and runtime behavior.
- [x] **Assumption:** the committed range is reviewed as one change; paths below are relative to `NEW/`.
- [x] **Source and test inspection completed.** Existing generated CMake/CTest metadata was inspected read-only; it is not treated as a successful build or test result.
- [x] **No production source or test implementation was changed.** Review reports are the only files required by the repository review policy.

## Review Status

- [x] Scope confirmation against `git log` and `git diff HEAD~2..HEAD`.
- [x] Source, CMake, module ownership, service lifetime, and dependency inspection.
- [x] Test and fixture registration inspection.
- [ ] Build, CTest, sanitizer, race, fault-injection, and runtime execution validation were not run because this was explicitly read-only.

## Findings: Critical

### CRITICAL-001: Queued service tasks can call destroyed service objects

- [ ] **Remediation status:** Open.
- **Source references:** `services/decompiler/decompiler_service.cppm:89-103` (`DecompilerService::decompile`), `services/sleigh/sleigh_service.cppm:48-72` (`SleighService::decode_batch`), `services/function_id/function_id_service.cppm:65-80` (`FunctionIdService::identify`), `runtime/project/project_manager.cppm:38-50` (`ProjectManager::close`), `runtime/project/runtime_core.cppm:52-59` (`RuntimeCore::shutdown`).
- **Affected component:** Worker-pool task lifetime and all asynchronous service adapters.
- **Technical evidence:** Each service submits a lambda capturing raw `[this]`. The returned `Task` owns only a future/control block, not the service. Project close removes and releases the session before runtime shutdown joins workers; the worker pool continues executing queued items while stopping.
- **Expected behavior:** A queued task must hold a service-state lease, or project shutdown must cancel and remove queued work before releasing service state.
- **Actual behavior:** A queued decompilation, batch decode, or Function ID request can execute through a dangling service pointer after project/session destruction.
- **Impact:** Use-after-free, access violation, corrupted results, or process termination during close or runtime shutdown.
- **Reproduction/failure scenario:** Occupy a single-worker pool, queue `ProjectFacade::decompile`, call `ProjectFacade::close`, then release the worker. The queued lambda invokes `decompile_now` on the destroyed `DecompilerService`.
- **Root cause:** Task lifetime and service lifetime are independent, with no shutdown barrier or service lease.
- **Recommended fix:** Capture shared immutable service state in each task and coordinate project cancellation/drain before releasing services. Define whether close cancels, drains, or waits for in-flight work.
- **Regression risks:** Task completion and cancellation semantics will change; add deterministic destruction-before-start and shutdown-race tests.
- **Relevant validation gap:** No service-destruction, close-while-queued, or sanitizer/race test was run or found.

## Findings: High

### HIGH-001: The default application still bypasses the service-platform runtime

- [ ] **Remediation status:** Open.
- **Source references:** `CMakeLists.txt:20-25` (`new_ghidra_app` links), `src/main.cpp:1-6` (legacy imports), `src/main.cpp:81-105` (direct `AnalysisContext`/`AutoAnalysisManager` path).
- **Affected component:** Default executable composition root.
- **Technical evidence:** The application links `NewGhidra::DecompilerFrontend` and `NewGhidra::Analyzer`, constructs the legacy analyzer context directly, and never opens `RuntimeCore` or `ProjectFacade`.
- **Expected behavior:** The application entry point should compose PE, Sleigh, analyzer, persistence, and decompiler services through core contracts and the native facade.
- **Actual behavior:** The normal application path bypasses event history, projections, runtime scheduling, service lifecycle, and canonical service adapters.
- **Impact:** The migration can pass service/integration tests while the shipped/default executable continues to run the old feature pipeline and does not exercise the new architecture.
- **Reproduction/failure scenario:** Run `new_ghidra_app` with a PE and SLA; execution enters `AnalysisContext` and `AutoAnalysisManager`, not `ProjectFacade::open/load/analyze`.
- **Root cause:** The new facade was added as a parallel test path rather than replacing the existing composition root.
- **Recommended fix:** Replace the application composition path with `RuntimeCore`/`ProjectFacade`, retaining the legacy path only as an explicitly named compatibility diagnostic.
- **Regression risks:** CLI output and failure behavior will change; preserve a separate raw analyzer diagnostic if needed.
- **Relevant validation gap:** No test asserts that the default executable uses the service graph.

### HIGH-002: Feature analyzers are not registered in the runtime service scheduler, and commands are discarded

- [ ] **Remediation status:** Open.
- **Source references:** `runtime/project/project_session.cppm:55-57` (`ProjectSession::open`), `runtime/project/CMakeLists.txt:19-26`, `services/analyzers/CMakeLists.txt:68-74`, `services/analyzers/analyzer_builtin.cpp:34-72`, `runtime/analysis/analysis_scheduler.cppm:31-52`, `runtime/project/project_session.cppm:190-200`.
- **Affected component:** Feature analyzer migration and project analysis lifecycle.
- **Technical evidence:** Project startup registers only `EntryMaterializationAnalyzer`. The new analyzer service target contains only `entry_materialization.cppm`; the 34 legacy analyzers are registered by `analyzer_builtin.cpp` into the old `AutoAnalysisManager`, not the new `IAnalyzer` registry. `AnalysisScheduler::run` collects mutation commands, but `ProjectSession::analyze` commits only start/finish lifecycle events and never commits `report.commands`.
- **Expected behavior:** Migrated analyzers must be registered through `IAnalyzer`, receive immutable snapshots, and have returned mutation commands committed through the project lane.
- **Actual behavior:** Facade analysis normally executes only the entry-materialization check, while any future analyzer command returned by the scheduler is silently dropped.
- **Impact:** PE/Sleigh/decompiler/FID analyzer functionality is absent from the service runtime; analysis can report success without applying feature results.
- **Reproduction/failure scenario:** Open a project through `ProjectFacade`, call `analyze`, and inspect `AnalysisSummary::executed_analyzers` and the projection. The runtime registry contains only `runtime.entry_materialization`; a command-producing analyzer would have its commands ignored.
- **Root cause:** Physical relocation and legacy aggregate compilation were completed without the runtime registration/commit bridge.
- **Recommended fix:** Add contract-facing adapters/registrations for migrated analyzers, wire them into the runtime registry, and convert every `MutationCommand` into durable events before reporting analysis completion.
- **Regression risks:** Analyzer ordering, prerequisite, cancellation, and event-trigger semantics need parity tests during registration.
- **Relevant validation gap:** Existing integration coverage checks only that analysis returns; it does not assert feature analyzer execution or mutation persistence.

### HIGH-003: Event commits are not atomic across the event log, memory projection, and SQLite

- [ ] **Remediation status:** Open.
- **Source references:** `runtime/event_store/append_only_log.cppm:38-79` (`AppendOnlyLog::append`), `runtime/projections/projection_coordinator.cppm:24-48` (`ProjectionCoordinator::commit`), `runtime/storage/sqlite_projection_store.cppm:47-62`.
- **Affected component:** Durable project commit boundary.
- **Technical evidence:** `append` writes each frame as it iterates and validates the draft project inside that loop. A later invalid draft can therefore leave earlier drafts in the log. The coordinator then applies the appended batch in memory and persists SQLite event-by-event; failures return without compensating the already-written log or applied memory state.
- **Expected behavior:** A failed commit must leave all authoritative and projection stores at the same revision, or recovery must have an explicit durable transaction protocol.
- **Actual behavior:** Partial log frames, advanced in-memory projection, and stale/partial SQLite checkpoints are possible after one batch failure.
- **Impact:** Queries, replay, revision checks, and future idempotency decisions can disagree; a caller may retry a commit that already partially changed history.
- **Reproduction/failure scenario:** Submit two drafts where the second belongs to another project, or inject a SQLite failure while persisting a multi-event batch. The first event is already appended/applied when the error is returned.
- **Root cause:** Append, projection application, and SQLite persistence are separate non-transactional stages with no rollback/rebuild barrier.
- **Recommended fix:** Validate the complete batch before writing, then use a transactional append/projection protocol or durable recovery marker; make SQLite persistence and in-memory application compensatable.
- **Regression risks:** Idempotency and replay behavior must be revalidated for retries and crash recovery.
- **Relevant validation gap:** Replay tests cover successful commits only; no mixed-validity batch or persistence-fault test exists.

### HIGH-004: Decompiler service ignores the selected SLA and architecture metadata

- [ ] **Remediation status:** Open.
- **Source references:** `runtime/project/project_session.cppm:155-156`, `services/decompiler/decompiler_service.cppm:80-87`, `services/decompiler/decompiler_service.cppm:105-134`, `services/decompiler/decompiler_service.cppm:191-194`, `services/pe_loader/pe_loader_service.cppm:137-144`.
- **Affected component:** Decompiler service-to-service boundary and non-x86 runtime behavior.
- **Technical evidence:** `ProjectSession` passes `config_.sleigh_specification` and the loaded architecture to the project, but `DecompilerService` stores `sla_path_` and never uses it. `decompile_now` constructs a fixed x86-64 architecture, register set, and `__cdecl` convention. PE loading also maps every non-amd64 machine to an x86-32 language ID.
- **Expected behavior:** Decompilation must consume the project-selected language/compiler specification and architecture contract, or reject unsupported architectures explicitly.
- **Actual behavior:** ARM/ARM64 and x86-32 inputs are decoded/decompiled through fixed x86-64 metadata; a supplied non-default SLA has no effect on the native decompiler architecture.
- **Impact:** Incorrect register/storage interpretation, wrong calling convention, invalid control/data-flow results, or misleading successful output for unsupported binaries.
- **Reproduction/failure scenario:** Load an ARM SLA or 32-bit PE and request decompilation; the native architecture still reports x86-64 with 64-bit registers.
- **Root cause:** The adapter was implemented around the x86 fixture rather than translating `ArchitectureDescription`/SLA metadata into the native provider.
- **Recommended fix:** Build native architecture/provider metadata from the canonical architecture resource and selected SLA; return an explicit unsupported diagnostic where conversion is unavailable.
- **Regression risks:** x86 fixture output must remain byte-for-byte compatible while architecture-specific tests are added.
- **Relevant validation gap:** All project/service fixtures are x86-64; no alternate architecture or non-default SLA test exists.

### HIGH-005: SQLite projection tables are declared but never materialized

- [ ] **Remediation status:** Open.
- **Source references:** `runtime/storage/sqlite_projection_store.cppm:27-36` and `runtime/storage/sqlite_projection_store.cppm:47-61`, `runtime/projections/software_model_projection.cppm:88-154`.
- **Affected component:** Durable SQLite projection and query model.
- **Technical evidence:** SQLite creates `instructions`, `functions`, and `memory_regions`, but `persist()` inserts only `applied_events` and `projection_checkpoint`; current queries read in-memory maps instead.
- **Expected behavior:** Declared domain tables are populated transactionally, or the implementation explicitly documents SQLite as only a checkpoint/cache.
- **Actual behavior:** SQLite reports an advanced checkpoint while domain rows remain absent.
- **Impact:** Durable query projection and independent SQLite verification are unavailable.
- **Reproduction/failure scenario:** Load a project and inspect `projection.sqlite` after a successful commit; domain tables contain no materialized instruction/function rows.
- **Root cause:** Event-specific SQL projection application was not implemented.
- **Recommended fix:** Apply each state event to domain tables in the same transaction as applied-event/checkpoint updates, or remove the unused schema.
- **Regression risks:** Row replacement/deletion and schema migration semantics.
- **Relevant validation gap:** Replay tests validate in-memory rebuild only and do not query SQLite domain rows.

### HIGH-006: Decompiler fallback reports native failure as `complete`

- [ ] **Remediation status:** Open.
- **Source references:** `services/decompiler/decompiler_service.cppm:156-181`, `runtime/project/tests/project_tests.cppm:128-129`.
- **Affected component:** Public decompilation status and downstream consumers.
- **Technical evidence:** Native exception paths enter `fallback()`, which emits listing-backed assembly comments and sets `DecompilationStatus::complete`; the integration test asserts that status without forcing a fallback.
- **Expected behavior:** A listing fallback has a distinct partial/degraded status.
- **Actual behavior:** Native failure is presented as a complete semantic decompilation unless callers inspect diagnostics.
- **Impact:** Callers can persist or analyze comment-only output as valid native decompiler output.
- **Reproduction/failure scenario:** Force a native provider/flow exception for a selected function and inspect the returned status.
- **Root cause:** Fallback reused the normal success status to preserve the happy-path pipeline.
- **Recommended fix:** Add a distinct fallback status or return failure with a separate listing artifact and test both paths.
- **Regression risks:** Consumers must handle degraded results explicitly.
- **Relevant validation gap:** No test distinguishes native completion from fallback completion.

## Findings: Medium

### MEDIUM-001: The service aggregate target is incomplete and `services --no-test` selects a non-existent target

- [ ] **Remediation status:** Open.
- **Source references:** `services/CMakeLists.txt:13-24` (`new_ghidra_services`), `build.bat:257-263` (`--no-test` target overrides).
- **Affected component:** CMake target graph and prescribed build wrapper.
- **Technical evidence:** `new_ghidra_services` depends on legacy engine targets but omits `new_ghidra_translation_engine`, `new_ghidra_pe_loader_service`, `new_ghidra_sleigh_service`, `new_ghidra_function_id_service`, `new_ghidra_decompiler_service`, and the service test target's other composition dependencies. In `build.bat`, the `services` no-test assignment is first set to `new_ghidra_services` and immediately overwritten with `new_ghidra_service_tests`; with `BUILD_TESTING=0`, that target is not created.
- **Expected behavior:** The services aggregate should build every service adapter, and `services --no-test` should build that aggregate with tests disabled.
- **Actual behavior:** The aggregate can omit the migrated adapters, while the prescribed no-test mode requests an unavailable test target.
- **Impact:** Clean/compile-only service validation is unreliable and can falsely omit the actual migration targets.
- **Reproduction/failure scenario:** Invoke `NEW\build.bat services --no-test`; CMake is configured with testing disabled, then Ninja is asked for `new_ghidra_service_tests`.
- **Root cause:** The aggregate target and wrapper were not updated together with the new service target names.
- **Recommended fix:** Make the aggregate depend on all public service targets and remove the later test-target override.
- **Regression risks:** Focused build modes may become more complete and take longer; verify target aliases and CTest registration.
- **Relevant validation gap:** No read-only target-graph check covered every build.bat mode.

### MEDIUM-002: Operation-context cancellation is disconnected from returned service tasks

- [ ] **Remediation status:** Open.
- **Source references:** `core/contracts/operation.cppm:108-114`, `runtime/workers/worker_pool.cppm:45-85`, `services/decompiler/decompiler_service.cppm:89-103`, `services/sleigh/sleigh_service.cppm:48-72`, `services/function_id/function_id_service.cppm:65-80`.
- **Affected component:** Cancellation and task error contracts.
- **Technical evidence:** `WorkerPool::submit` always creates a new `OperationControl`, ignoring `OperationContext::operation` and `OperationContext::cancellation`. If that new task is cancelled before start, the worker stores an exception in the promise rather than a `Result` cancellation value. Service lambdas check only the worker-created token.
- **Expected behavior:** Context cancellation and the returned task must refer to one operation, and cancellation should follow the service's `Result` contract.
- **Actual behavior:** Cancelling the supplied context does not stop queued work; pre-start cancellation makes `Task::get()` throw instead of returning `DiagnosticCode::cancelled`.
- **Impact:** Scheduler/project cancellation is ineffective and callers need exception handling outside the declared result contract.
- **Reproduction/failure scenario:** Cancel `context.operation` after dispatch, or cancel a queued task before a worker starts it, then observe continued execution or an exception from `get()`.
- **Root cause:** Worker submission owns cancellation state independently from the operation context and uses exception completion for cancellation.
- **Recommended fix:** Allow submission to adopt a supplied control/token and normalize pre-start cancellation to the documented result type.
- **Regression risks:** Status transitions and task-sharing semantics need concurrent tests.
- **Relevant validation gap:** Existing worker tests do not test pre-start cancellation or context/task identity.

### MEDIUM-003: Translation-engine target is a nominal alias layer, not the shared native engine

- [ ] **Remediation status:** Open.
- **Source references:** `services/translation_engine/CMakeLists.txt:1-19`, `services/translation_engine/native/*.cppm`, `services/sleigh/sleigh_service.cppm:3-6`, `services/decompiler/decompiler_service.cppm:3-6`, `services/decompiler/CMakeLists.txt:103-127`.
- **Affected component:** Shared translation boundary and duplicate native implementation ownership.
- **Technical evidence:** The translation target exports aliases to core values and a metadata struct, but no native `Address`, `AddrSpace`, `Translate`, `LoadImage`, or opcode implementation. Sleigh service code imports `sleigh_runtime`; decompiler code imports the legacy `decompiler` module, and the decompiler frontend links `SleighRuntime` rather than the translation-engine target.
- **Expected behavior:** The shared target should own the common native translation substrate or explicitly remain a core-value adapter until that extraction is complete.
- **Actual behavior:** The graph advertises a shared engine while the two service implementations retain separate native low-level ports and do not consume the shared module for behavior.
- **Impact:** Native semantic drift and duplicate implementation ownership remain possible; alternate providers cannot substitute at the advertised boundary.
- **Reproduction/failure scenario:** Change shared translation metadata or provide an alternate translation implementation; neither native service behavior changes because their imports remain concrete legacy modules.
- **Root cause:** Target relocation preceded extraction of the common native implementation.
- **Recommended fix:** Either move the common native classes behind the translation target and import them from both services, or narrow/document the target as metadata-only until extraction is complete.
- **Regression risks:** Native module consolidation can expose duplicate symbols and ordering dependencies.
- **Relevant validation gap:** No target-level test proves both services consume the shared translation implementation.

### MEDIUM-004: Function ID service does not preserve relation-aware scoring and still spawns unmanaged threads

- [ ] **Remediation status:** Open.
- **Source references:** `services/function_id/function_id_service.cppm:84-121`, especially line 114, `services/function_id/src/database.cppm:266-278`, `services/analyzers/function_id/src/function_id.cppm:212-231`.
- **Affected component:** Function ID service behavior and runtime scheduling.
- **Technical evidence:** `FunctionIdService::identify_now` constructs `fid::FunctionContext{*hash, {}, {}}`, so parent/inferior relations are always empty despite the relation-aware service description. The moved database parser and legacy analyzer use `std::async` for database/relation work rather than the shared bounded worker pool.
- **Expected behavior:** Function ID matching should pass the project function/reference family into native scoring and all expensive service work should use runtime-owned scheduling.
- **Actual behavior:** Relation scoring is disabled at the service boundary and database fan-out can create unmanaged threads outside project fairness/cancellation.
- **Impact:** Match ranking can differ from the original contract and resource-heavy projects can oversubscribe or outlive project lifecycle controls.
- **Reproduction/failure scenario:** Supply a function with known parent/child relations and compare candidates; the service passes empty relation sets. Load multiple FID databases and observe `std::async` outside the worker pool.
- **Root cause:** The service adapter ports hash/database calls but not the project graph/fan-out integration.
- **Recommended fix:** Build `FunctionHashFamily` from the immutable project query and route database warm-up/fan-out through runtime resource and worker services.
- **Regression risks:** Relation limits, scoring thresholds, and database load timing require parity fixtures.
- **Relevant validation gap:** Valid packed-database tests do not assert relation scoring or worker-pool use.

### MEDIUM-005: PE and Sleigh adapters drop fields promised by canonical contracts

- [ ] **Remediation status:** Open.
- **Source references:** `core/contracts/pe_loader.cppm:35-43`, `services/pe_loader/pe_loader_service.cppm:121-147`, `services/sleigh/sleigh_service.cppm:86-118`, `core/domain/operand.cppm:23-31`.
- **Affected component:** Core contract conversion.
- **Technical evidence:** `PeLoadResult` exposes structured imported symbols, exported symbols, and relocations, but `make_result` fills only textual `details.imports/exports`; the structured vectors remain empty. Sleigh conversion sets `architecture` to only `x86` or `unknown`, stores address operands as a generic scalar, and never fills the canonical operand `address`/`register_value` fields.
- **Expected behavior:** Adapters must preserve the observable structured facts required by the core contract, or reject/mark unsupported conversions explicitly.
- **Actual behavior:** Downstream services receive incomplete symbol/relocation and operand/architecture data even when the legacy parser/decoder has it.
- **Impact:** Reference, relocation, architecture, and Function ID/decompiler consumers cannot reliably use the canonical service values.
- **Reproduction/failure scenario:** Load a PE with imports/exports or decode an address operand; inspect `PeLoadResult::imported_symbols`/`exported_symbols` or `InstructionOperand::address`, which remain empty.
- **Root cause:** The migration adapter copies a subset of legacy fields into the new DTOs.
- **Recommended fix:** Map all contract fields with explicit architecture/address-space/register rules and add unsupported-field diagnostics where source metadata is insufficient.
- **Regression risks:** Serialization and equality expectations for existing fixtures will change.
- **Relevant validation gap:** Service tests assert only non-empty regions/architecture and one x86 `ret` instruction.

### MEDIUM-006: Project load failures leave lifecycle status stuck at `loading`

- [ ] **Remediation status:** Open.
- **Source references:** `runtime/project/project_session.cppm:97-137` (`ProjectSession::load_primary_binary`).
- **Affected component:** Project lifecycle and retry/error behavior.
- **Technical evidence:** The method sets `state_.status = loading`, but decoder-open failure at lines 126-129 and entry-point failure at lines 130-133 return directly without setting `failed`; only earlier PE failures and decode-body failure set `failed`.
- **Expected behavior:** Every load failure should leave a stable failed/closed state with a diagnostic, or deliberately restore the prior ready state.
- **Actual behavior:** A missing SLA or invalid entry can return an error while the session remains `loading` and retains partially committed PE events.
- **Impact:** Callers observe an impossible lifecycle state and retries can run against partially initialized services.
- **Reproduction/failure scenario:** Configure a nonexistent SLA, call `load`, inspect `ProjectFacade::state().status` after the error.
- **Root cause:** Failure-state updates are not guarded by a single scope-exit/error transition.
- **Recommended fix:** Centralize load failure cleanup/status transition and define whether already-committed input events are retained or compensated.
- **Regression risks:** Retry semantics and persisted load events need explicit tests.
- **Relevant validation gap:** Only successful load paths are covered.

### MEDIUM-007: Function ID malformed input escapes the `Result` contract

- [ ] **Remediation status:** Open.
- **Source reference:** `services/function_id/function_id_service.cppm:36-44`, `FunctionIdDatabaseService::query`.
- **Affected component:** Public Function ID database contract.
- **Technical evidence:** `std::stoull(hashes.full_hash, nullptr, 16)` is called without validation or exception conversion inside a method returning `core::Result`.
- **Expected behavior:** Invalid or oversized hash text returns a structured `core::Error`.
- **Actual behavior:** `std::invalid_argument` or `std::out_of_range` escapes the service boundary.
- **Impact:** Callers bypass normal diagnostics and task-level error handling.
- **Reproduction/failure scenario:** Query with `FunctionHashFamily{.full_hash = "not-hex"}` or an oversized hexadecimal value.
- **Root cause:** The native packed-database precondition was assumed at the canonical contract edge.
- **Recommended fix:** Parse with bounded `std::from_chars` and map invalid input to `invalid_argument` or `parse_failure`.
- **Regression risks:** Hash normalization and packed-database compatibility.
- **Relevant validation gap:** No malformed Function ID contract-input test exists.

### MEDIUM-008: Event subscriber exceptions escape after durable commit

- [ ] **Remediation status:** Open.
- **Source references:** `runtime/event_bus/event_bus.cppm:32-42`, `runtime/projections/projection_coordinator.cppm:45-48`.
- **Affected component:** Event publication and command result handling.
- **Technical evidence:** The coordinator persists state and then synchronously invokes callbacks; callback exceptions are not contained or represented as diagnostics.
- **Expected behavior:** Non-projection subscriber failure does not turn a completed durable commit into an uncaught exception.
- **Actual behavior:** A throwing subscriber escapes after the log, projection, and SQLite have advanced.
- **Impact:** Process termination or caller-visible failure for a mutation that already committed.
- **Reproduction/failure scenario:** Subscribe a callback that throws and commit one event.
- **Root cause:** No event-bus subscriber failure policy exists.
- **Recommended fix:** Catch/aggregate callback errors or publish asynchronously through durable delivery state.
- **Regression risks:** Notification timing and retry behavior.
- **Relevant validation gap:** No throwing-subscriber test exists.

### MEDIUM-009: Project analysis records an invalid resource identity

- [ ] **Remediation status:** Open.
- **Source reference:** `runtime/project/project_session.cppm:179-185`.
- **Affected component:** Resource reproducibility and analysis snapshots.
- **Technical evidence:** The primary resource is recorded with `byte_size = 0` and a caller-provided hash, although the loaded executable is non-empty and `ResourceManager` validates exact size/content identity.
- **Expected behavior:** Analysis snapshots contain actual file size/content identity or reject unresolved resources.
- **Actual behavior:** A loaded artifact can be represented as zero bytes with placeholder hash text.
- **Impact:** Resource validation fails when wired in or silently accepts a non-reproducible identity.
- **Reproduction/failure scenario:** Validate the snapshot resource with `ResourceManager::require()` after loading a non-empty executable.
- **Root cause:** Project ingestion does not compute/propagate artifact metadata.
- **Recommended fix:** Compute the actual size/digest during load and persist it before analysis.
- **Regression risks:** Load cost and event metadata compatibility.
- **Relevant validation gap:** End-to-end tests use placeholder artifact metadata.

## Findings: Low

### LOW-001: `tidy.bat` skips every named C++ module rather than checking it

- [ ] **Remediation status:** Open.
- **Source reference:** `tidy.bat:57-72`.
- **Affected component:** Static-analysis validation wrapper.
- **Technical evidence:** The skip regex matches `import`, `export import`, or `export module`; every production `.cppm` contains at least `export module`, so it is classified as a module consumer and no `clang-tidy` invocation occurs. Skips are not failures.
- **Expected behavior:** The wrapper should check supported translation units and clearly report only unsupported consumer cases.
- **Actual behavior:** A successful tidy run can process zero production module files.
- **Impact:** Required static analysis is silently absent from migration validation.
- **Reproduction/failure scenario:** Run `tidy.bat services --check`; service module interfaces match `export module` and are skipped.
- **Root cause:** Module-interface detection is broader than the unsupported-consumer condition.
- **Recommended fix:** Distinguish module interfaces from importing consumers and report a nonzero/error outcome when all files are skipped.
- **Regression risks:** clang-tidy support varies by compiler/module mode; retain an explicit exclusion report.
- **Relevant validation gap:** The wrapper was not executed in this read-only review.

### LOW-002: Architecture specification contains stale pre-migration build evidence

- [ ] **Remediation status:** Open.
- **Source reference:** `ARCHITECTURE.md:40-48`, especially line 44, which says the build adds `features` and lacks the core/runtime graph, contradicting the current `CMakeLists.txt:13-18`.
- **Affected component:** Architecture documentation and review traceability.
- **Technical evidence:** The document says the current build is still the former feature graph while the committed migration removed that graph and added core/runtime/services/tests.
- **Expected behavior:** Architecture evidence should describe the current committed source and target graph.
- **Actual behavior:** Readers can follow incorrect feature paths or infer that core/runtime were not implemented.
- **Impact:** Review and maintenance decisions may be based on obsolete dependency information.
- **Reproduction/failure scenario:** Follow the build-root evidence table from `ARCHITECTURE.md`; it points to behavior that no longer matches `NEW/CMakeLists.txt`.
- **Root cause:** Migration updated service documentation but not all architecture evidence sections.
- **Recommended fix:** Refresh stale tables and distinguish historical migration notes from current implementation facts.
- **Regression risks:** Documentation links and source references must be rechecked after future moves.
- **Relevant validation gap:** No documentation-link/source-consistency check was run.

## Verified Strengths

- [x] The physical source/test/fixture relocation is represented in the current `services` tree and service CMake subdirectories.
- [x] Core value types and service contracts avoid exposing native engine pointers or SQLite types.
- [x] Sleigh decode access is serialized by a mutex, and batch results are sorted by address.
- [x] PE/Sleigh/decompiler/FID/analyzer focused test registrations and root integration/replay targets are present in generated CTest metadata.
- [x] Event framing includes length boundaries and checksum validation, with incomplete-tail recovery code and tests.

## Reviewed Areas With No Confirmed Findings

- [x] Core identifier/address arithmetic and event-field encoding inspected; no migration-specific defect confirmed.
- [x] PE parser, Sleigh native decoder, decompiler native algorithm, and FID parser internals were reviewed as moved implementation sources; no additional relocation-only defect was confirmed beyond the adapters listed above.
- [x] Basic CMake subdirectory ordering and service-test aliases resolve in the existing generated graph; the finding is limited to the incomplete aggregate/no-test wrapper behavior.
- [x] No direct binding implementation change was found; `bindings/cpp` remains a thin facade consumer.

## Validation Results

- [x] `git log` and `git diff HEAD~2..HEAD` inspected for exactly the two requested commits.
- [x] Existing generated `NEW/build/CTestTestfile.cmake`, `build.ninja`, and `.ninja_log` inspected read-only.
- [x] `ctest --test-dir NEW/build --output-on-failure` executed during the review: **49/49 tests passed**.
- [x] `git diff HEAD~2..HEAD --check` passed.
- [ ] No sanitizer, race, fault-injection, multi-process lock, alternate-architecture, or standalone runtime stress validation was performed.

## Unresolved Questions And Residual Risks

- [ ] Define the authoritative projection policy: SQLite domain materialization versus event-log rebuild/checkpoint cache.
- [ ] Define service/task shutdown ordering and cancellation semantics before claiming runtime safety.
- [ ] Decide whether the legacy direct analyzer executable is intentionally retained as a diagnostic or must be removed from the default app.
- [ ] Add non-x86, malformed-contract, partial-commit, and service-lifetime fixtures.

## Follow-Up Decision

- [ ] No implementation fixes were authorized or applied.
- [ ] Proposed remediation order: `CRITICAL-001`, then `HIGH-001` through `HIGH-006`, followed by `MEDIUM-001` through `MEDIUM-009`.
