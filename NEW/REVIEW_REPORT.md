# Architecture Review Report

## Review Metadata

- **Scope:** [`ARCHITECTURE.md`](ARCHITECTURE.md), the current `NEW/` CMake/module/test tree, and the current Sleigh, decompiler, PE loader, Function ID, and analyzer implementation boundaries.
- **Reviewed revision/diff:** Working-tree `NEW/ARCHITECTURE.md` and current source tree on 2026-09-16. No implementation diff was applied by this review.
- **Date:** 2026-09-16.
- **Reviewer:** Kilo.
- **Severity mapping:** `high` corresponds to a Major finding in [`REVIEW_ARCHITECTURE.md`](REVIEW_ARCHITECTURE.md); `medium` corresponds to Minor. No Critical or low findings were retained.
- **Assumptions:** Paths are relative to `NEW/` unless prefixed with `../`. The architecture is intended to be an implementable specification, not only a conceptual sketch.

## Review Status

- [x] Scope confirmed.
- [x] Current source and dependency boundaries inspected.
- [x] Current CMake and test registration inspected.
- [x] Architecture-to-implementation inconsistencies recorded.
- [x] No source, CMake, or `ARCHITECTURE.md` file was modified.
- [x] Requested review written to [`REVIEW_ARCHITECTURE.md`](REVIEW_ARCHITECTURE.md).
- [ ] Build, CTest, formatting, and tidy validation: not run because this was a read-only architecture review.

## Findings: Critical

### No Findings

No confirmed Critical finding was identified. The High findings below must still be resolved before implementation begins because they affect correctness, persistence, or scalability.

## Findings: High

### HIGH-001: Append-Only History Has No Growth or Supersession Strategy

- [ ] **Remediation status:** Open.
- **Reference:** [`ARCHITECTURE.md:26-27`](ARCHITECTURE.md), [`ARCHITECTURE.md:921-953`](ARCHITECTURE.md), [`ARCHITECTURE.md:1017-1044`](ARCHITECTURE.md); [`features/analyzers/shared/src/analyzer_context.cppm:437-461`](features/analyzers/shared/src/analyzer_context.cppm).
- **Affected component:** Event log, projection rebuild, repeated analysis.
- **Technical evidence:** The design persists materialized instruction, reference, function, symbol, fact, and analysis changes in an append-only log. Batch framing is specified, but no snapshot, compaction, retention, or supersession protocol is specified. The current context owns large mutable collections, and [`features/analyzers/shared/src/analyzer_manager.cppm:247-297`](features/analyzers/shared/src/analyzer_manager.cppm) can revisit them during re-analysis.
- **Expected behavior:** Storage and replay cost must remain operationally bounded for large binaries and repeated analysis while preserving an explicit audit policy.
- **Actual behavior:** The architecture defines durable append and replay but no policy for old derived results, replacement, compaction, or run-level retention.
- **Impact:** Disk use, backup size, replay time, and projection rebuild time can grow without bound; stale derived state can remain in history indefinitely.
- **Failure scenario:** Re-run analysis after a signature or flow change several times on a large executable. Each run appends another set of derived mutations, and reopening a stale projection requires replaying the full history.
- **Root cause:** Event durability and event lifecycle/retention were designed together as if batching solved both problems.
- **Recommended fix:** Separate user/import history from derived analysis history; define snapshots, event tails, compaction, retention, tombstones, and supersession semantics; benchmark large-binary replay.
- **Regression risks:** Compaction can reduce forensic history and introduces snapshot migration/recovery cases.
- **Relevant validation:** Current mutable state and re-analysis behavior were inspected; no runtime implementation or scale test currently exists.

### HIGH-002: Analyzer Snapshot and Commit Semantics Are Incomplete

- [ ] **Remediation status:** Open.
- **Reference:** [`ARCHITECTURE.md:644-672`](ARCHITECTURE.md), [`ARCHITECTURE.md:1185-1200`](ARCHITECTURE.md), [`ARCHITECTURE.md:2064`](ARCHITECTURE.md); [`features/analyzers/shared/src/analyzer_manager.cppm:205-222`](features/analyzers/shared/src/analyzer_manager.cppm).
- **Affected component:** Analyzer scheduler, mutation proposals, deterministic compatibility.
- **Technical evidence:** The target analyzer contract returns commands from an immutable snapshot, while the current manager invokes one analyzer, drains its events, and immediately schedules downstream work. The architecture does not define read-your-writes, proposal read sets, multi-entity atomicity, or the commit order for concurrent analyzers.
- **Expected behavior:** Order-sensitive Ghidra behavior must be preserved while allowing only explicitly safe parallel work.
- **Actual behavior:** A numeric priority and DAG are specified, but analyzers can still be admitted from the same revision without a complete conflict and ordering contract.
- **Impact:** Function-body carving, flow overrides, symbol priority, and no-return propagation can become completion-order dependent or suffer excessive retries.
- **Failure scenario:** Two analyzers read revision `R`, both propose changes to one function or reference set, and both pass a project-level revision check. The second commit either overwrites the first or forces an undefined retry path.
- **Root cause:** The migration replaces direct mutation with proposals without specifying a transaction model for the behavior that direct mutation previously provided.
- **Recommended fix:** Classify analyzers as serial/order-sensitive, parallel proposal, or commutative/idempotent; add entity-version/read-set preconditions, all-or-nothing batches, deterministic tie-breaking, and local write overlays where intermediate state is required.
- **Regression risks:** Strong conflict checking reduces parallelism and may expose missing dependencies in existing analyzer ports.
- **Relevant validation:** Current direct mutation and event-drain ordering were inspected; the architecture itself lists analyzer concurrency as unresolved.

### HIGH-003: Projection and Event-Bus Ownership Is Duplicated

- [ ] **Remediation status:** Open.
- **Reference:** [`ARCHITECTURE.md:955-992`](ARCHITECTURE.md), [`ARCHITECTURE.md:1194-1197`](ARCHITECTURE.md), [`ARCHITECTURE.md:1307-1310`](ARCHITECTURE.md).
- **Affected component:** Commit coordinator, projection, event bus, scheduler triggers.
- **Technical evidence:** The commit protocol directly applies the projection before publishing. The event-bus section nevertheless lists a projection subscriber, and the diagrams separately show store-to-projection and store-to-bus delivery.
- **Expected behavior:** Exactly one component must own projection application and checkpoint advancement, with a defined read-after-commit barrier.
- **Actual behavior:** An implementation can interpret the projection as either a direct coordinator participant or a bus consumer.
- **Impact:** Duplicate application, ambiguous checkpoint ownership, stale scheduler reads, and inconsistent crash recovery are possible.
- **Failure scenario:** A committed event is applied by the coordinator and then delivered to the projection subscriber, or the process crashes after log append but before bus delivery. The resulting checkpoint and notification behavior depends on which path is authoritative.
- **Root cause:** Direct commit orchestration and event-driven projection delivery were both retained without choosing one consistency model.
- **Recommended fix:** Prefer append, projection transaction/checkpoint, then publication to non-projection subscribers. If the bus owns projections, define a durable inbox/checkpoint and command-completion semantics instead.
- **Regression risks:** Reordering changes analyzer trigger timing; add ordering, duplicate-delivery, and crash-window tests.
- **Relevant validation:** The contradiction is internal to the cited architecture sections; no runtime bus or projection implementation exists.

### HIGH-004: The Event Schema Is Not Closed Over Existing Mutable State

- [ ] **Remediation status:** Open.
- **Reference:** [`ARCHITECTURE.md:876-921`](ARCHITECTURE.md), [`ARCHITECTURE.md:1017-1044`](ARCHITECTURE.md); [`features/analyzers/shared/src/analyzer_context.cppm:437-461`](features/analyzers/shared/src/analyzer_context.cppm), [`features/analyzers/shared/src/analyzer_context.cppm:1012-1398`](features/analyzers/shared/src/analyzer_context.cppm).
- **Affected component:** Event payloads, replay, derived facts and projection tables.
- **Technical evidence:** Current state includes strings, bookmarks, constants, external symbols, archives, address tables, embedded media, PDB records, candidate starts, stack variables, function flags, flow overrides, and signatures. The initial event list does not explicitly define all of their create/update/remove/replace behavior.
- **Expected behavior:** Replay must reconstruct every authoritative current value, and derived values must have explicit invalidation or rebuild semantics.
- **Actual behavior:** Broad event names such as `FunctionFlagsChanged` and `ConstantFactAdded` are listed without a complete state-to-event matrix or retraction/supersession contract.
- **Impact:** Replay can lose state or retain stale facts and signatures after re-analysis; a future knowledge projection cannot reliably interpret provenance.
- **Failure scenario:** A PDB symbol, stack variable, or constant fact changes between analysis runs. The projection receives only an add-like event, so the old record remains active or the replay lacks enough information to reproduce the current context.
- **Root cause:** Event categories were specified before mapping every current mutator and collection to a durable identity and lifecycle.
- **Recommended fix:** Add a state-to-event matrix with identities, source priority, expected revisions, idempotency, replacement, and deletion for every collection; classify transient/derived state explicitly.
- **Regression risks:** A finer event taxonomy increases codec and schema maintenance; coarse replacement events increase payload size.
- **Relevant validation:** Current context fields and mutators were read directly; no projection can currently prove replay equivalence.

### HIGH-005: Service Dependency Direction Contradicts the Layer Rules

- [ ] **Remediation status:** Open.
- **Reference:** [`ARCHITECTURE.md:251-260`](ARCHITECTURE.md), [`ARCHITECTURE.md:1450-1491`](ARCHITECTURE.md); [`features/analyzers/CMakeLists.txt:1-7`](features/analyzers/CMakeLists.txt), [`features/analyzers/CMakeLists.txt:56-60`](features/analyzers/CMakeLists.txt), [`features/function_id/CMakeLists.txt:25-27`](features/function_id/CMakeLists.txt).
- **Affected component:** Build graph, optional services, runtime composition.
- **Technical evidence:** The layer rules describe service contracts below runtime composition, but the graph contains `FID --> SL`, `AN --> SL`, `AN --> DEC`, and `AN --> PE`. The current analyzer shared library publicly links PE and Sleigh, and the aggregate analyzer links every analyzer library.
- **Expected behavior:** Services should depend on stable contracts; concrete services should be connected by runtime composition or explicitly isolated adapters.
- **Actual behavior:** Analyzers and Function ID remain coupled to concrete feature targets and their value types.
- **Impact:** Alternate implementations and isolated tests require broad aggregate links; optional analyzer loading and dependency enforcement become difficult, and future runtime cycles are likely.
- **Failure scenario:** Introduce a runtime-created decoder provider or a second loader. The analyzer target still imports the concrete Sleigh/PE modules and cannot substitute the new implementation without rebuilding or changing feature code.
- **Root cause:** The target architecture identifies both contract injection and direct service links but does not reconcile them.
- **Recommended fix:** Make feature targets depend on core contracts and narrow capability interfaces; inject concrete services in runtime/provider factories and document any unavoidable native exceptions.
- **Regression risks:** More adapters can temporarily duplicate conversions and require composition fixtures.
- **Relevant validation:** Current CMake targets and the proposed dependency graph were compared directly.

### HIGH-006: `core/domain` Is Too Broad to Remain a Stable Kernel

- [ ] **Remediation status:** Open.
- **Reference:** [`ARCHITECTURE.md:270-360`](ARCHITECTURE.md), [`ARCHITECTURE.md:421`](ARCHITECTURE.md); [`features/decompiler/src/decompiler.cppm:7-531`](features/decompiler/src/decompiler.cppm), [`features/analyzers/shared/src/analyzer_types.cppm:17-419`](features/analyzers/shared/src/analyzer_types.cppm).
- **Affected component:** Core API, public facade, cross-feature compile-time coupling.
- **Technical evidence:** The proposed core owns full function snapshots, decompiler artifacts, FID scoring/options/results, type graphs, architecture details, variables, and analysis facts. The current project keeps these vocabularies in separate decompiler, analyzer, and Function ID modules.
- **Expected behavior:** Core values should contain only semantics stable enough for multiple independent services and persistence consumers.
- **Actual behavior:** Feature policies and outputs are promoted into the foundational domain before their contracts and lifecycle semantics are proven.
- **Impact:** Unrelated service changes become core and binding changes; a large value aggregate recreates the coupling the architecture is trying to remove.
- **Failure scenario:** Change FID scoring evidence or decompiler text/cache policy. The change requires altering core/domain, event codecs, projections, and public bindings even when PE and Sleigh behavior is unchanged.
- **Root cause:** “Canonical” was used as a remedy for duplicate value types without a promotion criterion for core ownership.
- **Recommended fix:** Keep a minimal address/storage/p-code/instruction/reference/provenance kernel and keep FID, decompiler, PE, and analyzer-specific DTOs in feature contracts until cross-service semantics stabilize.
- **Regression risks:** Adapters add conversion work and temporary representation duplication.
- **Relevant validation:** Current public APIs and the proposed core catalog were inspected; the concern is coupling scope, not the use of value types itself.

### HIGH-007: Decoder Resource Concurrency Is Not Specified for Bulk Work

- [ ] **Remediation status:** Open.
- **Reference:** [`ARCHITECTURE.md:490-520`](ARCHITECTURE.md), [`ARCHITECTURE.md:750-755`](ARCHITECTURE.md); [`features/sleigh_runtime/sleigh_runtime_adapter.cppm:745-768`](features/sleigh_runtime/sleigh_runtime_adapter.cppm), [`features/sleigh_runtime/sleigh_runtime_adapter.cppm:817-850`](features/sleigh_runtime/sleigh_runtime_adapter.cppm).
- **Affected component:** Sleigh resource manager, worker pool throughput, native parser safety.
- **Technical evidence:** The current implementation shares one mutable `ghidra::Sleigh` per cached SLA and holds a mutex through the full decode. The architecture lists worker-local state, leases, and a mutex-protected decoder as alternatives but does not choose capacity or memory behavior.
- **Expected behavior:** Bulk decoding must have a measured, bounded concurrency strategy that does not starve interactive decoding.
- **Actual behavior:** The target can be implemented with a globally serialized decoder or with one full parser per worker, both without a specified performance or memory bound.
- **Impact:** GTA5-scale decoding can fail to scale or consume excessive memory/startup time; lease contention is invisible without required metrics.
- **Failure scenario:** Queue millions of instructions on one SLA. Every worker reaches the same mutex and throughput is effectively single-threaded, or each worker reparses the SLA and exhausts memory.
- **Root cause:** Immutable parsed tables and mutable per-decode parser state have not been separated in the native design.
- **Recommended fix:** Define immutable SLA tables plus worker-local parser/context state, or a bounded decoder pool with lease quotas, interactive reservation, and benchmarks.
- **Regression risks:** Worker-local state may require native refactoring and increases memory use.
- **Relevant validation:** The current mutex scope was read directly; no bulk concurrency benchmark exists.

### HIGH-008: External Artifact and Resource Retention Weakens Reproducibility

- [ ] **Remediation status:** Open.
- **Reference:** [`ARCHITECTURE.md:1084-1099`](ARCHITECTURE.md), [`ARCHITECTURE.md:1121-1148`](ARCHITECTURE.md), [`ARCHITECTURE.md:2083-2093`](ARCHITECTURE.md); [`features/analyzers/shared/src/analyzer_context.cppm:437-475`](features/analyzers/shared/src/analyzer_context.cppm).
- **Affected component:** Project reopen, replay, re-analysis, resource manager.
- **Technical evidence:** External executable references and optional managed copies are both allowed; the open path loads the artifact before replay. SLA, compiler, FID, PDB, and archive resources are not all required to have content hashes and version identities.
- **Expected behavior:** The project must state whether it guarantees only historical projection access or deterministic re-analysis, and must enforce the required input identity for that mode.
- **Actual behavior:** The log can outlive the bytes and resources needed to reproduce it, while the current context and decoder require those inputs to operate.
- **Impact:** Reanalysis can silently use changed resources or fail after a path move; replay and current projection are not equivalent to a reproducible analysis environment.
- **Failure scenario:** Reopen a project after the executable moves or its SLA is replaced. The log remains valid, but loading or extending analysis either fails or uses a different input without a clear mode transition.
- **Root cause:** History durability, input artifact retention, and reproducibility were treated as optional independent policies.
- **Recommended fix:** Add a manifest with content hashes and tool/resource versions; provide explicit portable and history-only modes; refuse incompatible reanalysis.
- **Regression risks:** Managed resource copies increase disk use and import time.
- **Relevant validation:** Project layout, open sequence, assumptions, and current path-owned resource state were inspected.

### HIGH-009: Revision-Stamped Snapshots Have No Defined Physical Consistency Model

- [ ] **Remediation status:** Open.
- **Reference:** [`ARCHITECTURE.md:405-418`](ARCHITECTURE.md), [`ARCHITECTURE.md:1017-1068`](ARCHITECTURE.md), [`ARCHITECTURE.md:1185-1199`](ARCHITECTURE.md).
- **Affected component:** `IProjectQuery`, SQLite projection, long-running native tasks.
- **Technical evidence:** `snapshot(selection)` and revision-stamped views are promised, but the logical schema is mainly a current-state schema with selected revision columns and one checkpoint. No historical row retention or snapshot lifetime/connection contract is defined.
- **Expected behavior:** A task must observe a coherent cross-table state at one revision and release all storage resources before project close.
- **Actual behavior:** Capturing a revision number alone does not stop later reads from seeing newer rows; a long SQLite read transaction and an owned DTO snapshot have materially different behavior, neither of which is selected.
- **Impact:** Mixed-revision decompilation and stale mutation proposals can produce incorrect results or unbounded WAL/resource retention.
- **Failure scenario:** A task reads a function at `R`, then reads references after a user mutation at `R+1`; the task combines incompatible body and reference state while reporting revision `R`.
- **Root cause:** Logical revision metadata was specified without a physical snapshot mechanism.
- **Recommended fix:** Define owned immutable snapshots or bounded read-transaction leases, including coherence, size, lifetime, close, and conflict semantics. Do not promise arbitrary historical queries without history storage.
- **Regression risks:** DTO snapshots cost memory; pinned reads create SQLite pressure; historical tables increase storage.
- **Relevant validation:** Proposed query contracts and schema tables were compared; no projection implementation currently resolves the choice.

### HIGH-010: Single-Writer Persistence Is Not Enforceable Across Processes

- [ ] **Remediation status:** Open.
- **Reference:** [`ARCHITECTURE.md:416-418`](ARCHITECTURE.md), [`ARCHITECTURE.md:945-979`](ARCHITECTURE.md), [`ARCHITECTURE.md:1064-1068`](ARCHITECTURE.md), [`ARCHITECTURE.md:1084-1097`](ARCHITECTURE.md).
- **Affected component:** Event log, projection files, project manager, crash recovery.
- **Technical evidence:** The design requires one logical writer but defines an in-process `Runtime` and separate log/projection files without an OS-level lock or read-only open mode.
- **Expected behavior:** The single-writer invariant must hold across processes and after crash/restart, not only between objects sharing one `Runtime`.
- **Actual behavior:** Two runtimes or test processes can both open the same project and append to the log; SQLite's lock does not protect the independent log writer.
- **Impact:** Concurrent append or recovery can corrupt history and create log/projection revision divergence.
- **Failure scenario:** Start two CLI processes against one project. Both validate the same revision and append frames while each believes it owns the project lock that is only logical/in-process.
- **Root cause:** Ownership scope was defined at the project abstraction but not at the filesystem/process boundary.
- **Recommended fix:** Add an OS-level project lock, explicit read-only mode, atomic manifest updates, fsync guarantees, format migrations, and crash-injection tests.
- **Regression risks:** Platform-specific locking adds code and prevents convenient concurrent writers.
- **Relevant validation:** Physical layout and runtime ownership sections were inspected; no storage implementation or lock test exists.

### HIGH-011: Async Operation Lifetime and Shutdown Ownership Are Underspecified

- [ ] **Remediation status:** Open.
- **Reference:** [`ARCHITECTURE.md:377-395`](ARCHITECTURE.md), [`ARCHITECTURE.md:1121-1134`](ARCHITECTURE.md), [`ARCHITECTURE.md:1388-1418`](ARCHITECTURE.md).
- **Affected component:** Task handles, progress/cancellation, project close, bindings.
- **Technical evidence:** `OperationContext` contains a non-owning `ProgressSink*`; queued task handles and project close are both public concepts, but task drop, sink lifetime, project generation, and post-close commit rejection are not specified.
- **Expected behavior:** No task may dereference a destroyed caller object or commit against a closed project; all leases must be released before `Closed`.
- **Actual behavior:** A task can outlive a progress sink or project facade, and the close protocol only says that active tasks reach a safe cancellation point.
- **Impact:** Use-after-lifetime failures and writes to closed stores can occur during GUI cancellation, binding garbage collection, or process shutdown.
- **Failure scenario:** Submit a decompilation, destroy the progress callback, and close the project while native work is active. The task later reports progress or reaches the commit path.
- **Root cause:** Async ownership is described in terms of non-borrowed snapshots but not in terms of the task's full operation state and shutdown generation.
- **Recommended fix:** Own shared cancellation/progress/commit state per task, define handle-drop semantics, add a project generation token, and reject commits after close.
- **Regression risks:** Shared operation state adds synchronization and close can wait longer for non-interruptible native calls.
- **Relevant validation:** Contract pseudocode and lifecycle diagrams were inspected; current manager cancellation is synchronous and cannot validate the future task behavior.

## Findings: Medium

### MEDIUM-001: Stable Address-Space and Entity Identity Encoding Is Not Defined

- [ ] **Remediation status:** Open.
- **Reference:** [`ARCHITECTURE.md:278-291`](ARCHITECTURE.md), [`ARCHITECTURE.md:313-324`](ARCHITECTURE.md), [`ARCHITECTURE.md:1028-1036`](ARCHITECTURE.md); [`features/analyzers/shared/src/analyzer_types.cppm:17-32`](features/analyzers/shared/src/analyzer_types.cppm).
- **Affected component:** Event keys, address spaces, instruction/function identity.
- **Technical evidence:** Public values require stable space/entity IDs, but projection examples use `(space,address)` and the current model uses raw integer addresses. No deterministic ID derivation or delete/recreate policy is specified.
- **Expected behavior:** Reopen, resource migration, and event replay must preserve logical identity.
- **Actual behavior:** Numeric space assignment or generated entity IDs could change when an SLA or projection is rebuilt.
- **Impact:** References and events can target the wrong logical entity or lose identity after migration.
- **Failure scenario:** Rebuild address spaces in a different order and replay an event containing a numeric space ID.
- **Root cause:** Identity types were named before their persistence encoding and lifecycle were defined.
- **Recommended fix:** Persist space mappings, define entity ID creation/deletion/recreation rules, and use stable names or content-derived IDs where numeric IDs are not stable.
- **Regression risks:** Stable IDs add metadata and migration work.
- **Relevant validation:** Proposed domain and projection keys were compared with current raw-address types.

### MEDIUM-002: Observability Is Optional Where the Runtime Needs a Contract

- [ ] **Remediation status:** Open.
- **Reference:** [`ARCHITECTURE.md:706-723`](ARCHITECTURE.md), [`ARCHITECTURE.md:724-767`](ARCHITECTURE.md), [`ARCHITECTURE.md:1183-1200`](ARCHITECTURE.md).
- **Affected component:** Worker pool, scheduler, persistence, diagnostics.
- **Technical evidence:** Metrics/logging are optional, while the design introduces queue fairness, decoder leases, projection lag, replay, and conflict retries. Correlation IDs exist, but required measurements for those paths are not specified.
- **Expected behavior:** Operators and tests must distinguish starvation, I/O delay, decoder serialization, projection lag, and repeated conflicts.
- **Actual behavior:** A large analysis can fail or stall with only generic diagnostics and no standard latency/backlog evidence.
- **Impact:** Performance goals and concurrency regressions cannot be verified or diagnosed on large binaries.
- **Failure scenario:** An interactive decode waits behind bulk work; without lease-wait and queue-latency metrics it is indistinguishable from a hung decoder.
- **Root cause:** Observability was treated as an optional runtime feature instead of part of the scheduler/storage contract.
- **Recommended fix:** Define replaceable structured logging, counters, durations, progress, and diagnostics keyed by project/task/analyzer/revision.
- **Regression risks:** Instrumentation adds small runtime and output costs.
- **Relevant validation:** Runtime ownership and scheduling sections were inspected; no runtime implementation exists.

### MEDIUM-003: Integration and Replay Test Ownership Is Not Fully Mapped to the Build

- [ ] **Remediation status:** Open.
- **Reference:** [`ARCHITECTURE.md:1681-1788`](ARCHITECTURE.md); [`CMakeLists.txt:11-25`](CMakeLists.txt), [`features/analyzers/tests/CMakeLists.txt:1-14`](features/analyzers/tests/CMakeLists.txt).
- **Affected component:** Runtime/replay tests, CMake/CTest architecture.
- **Technical evidence:** The target tree lists runtime and root integration/replay/fixture directories but does not assign CMake/CTest ownership or fixture manifests. The current root CMake only adds feature targets and one smoke test; current tests are feature-local.
- **Expected behavior:** Event recovery, projection idempotence, snapshot consistency, shutdown, decoder concurrency, and scale behavior must have named targets and registered tests.
- **Actual behavior:** The architecture names the tests conceptually but leaves the build ownership and required test matrix open.
- **Impact:** The highest-risk new boundaries can be implemented without regression coverage.
- **Failure scenario:** All feature tests pass while a projection replay duplicates rows or a project close races a queued task because no runtime test target exercises those paths.
- **Root cause:** The target folder tree was specified before runtime test/build ownership was assigned.
- **Recommended fix:** Add a test ownership table with target, CMake owner, CTest name, fixture, scale, oracle/fingerprint, and crash/concurrency requirements.
- **Regression risks:** Integration tests increase CI time and may need platform-specific crash injection.
- **Relevant validation:** Current root/features CMake and target tree were inspected; no runtime implementation exists.

## Verified Strengths

- [x] The current `AnalysisContext` is correctly identified as a large mutable seam that should not become the final public program model.
- [x] Separating PE parsing from project/listing side effects preserves the reusable checked parser in [`features/pe_loader/src/pe_loader.cppm`](features/pe_loader/src/pe_loader.cppm).
- [x] Consolidating duplicated native translation classes is supported by the current Sleigh/decompiler modules and original shared native contracts.
- [x] Per-task mutable native decompiler state and immutable project providers are sound ownership decisions.
- [x] The command, transient response, persistent event, event-bus, and projection distinction is conceptually correct.
- [x] A replaceable SQLite projection is a practical initial query model if services cannot access it directly.
- [x] Retaining Ghidra-compatible numeric priorities while adding explicit prerequisites is a good compatibility baseline.
- [x] Replacing current unbounded Function ID `std::async` work in [`features/function_id/src/database.cppm:260-318`](features/function_id/src/database.cppm) with runtime scheduling is directionally correct.
- [x] Existing feature-local tests and port evidence should be preserved and supplemented with cross-boundary tests rather than replaced.

## Reviewed Areas With No Findings

- [x] The documented absence of final `core`, `runtime`, `services`, `bindings`, and `apps` layers matches the current tree.
- [x] The separation of future knowledge/facts/hypotheses projection from the first conventional projection is consistent with the stated project goals.
- [x] The current Function ID and analyzer/decompiler parity gaps are acknowledged rather than hidden.

## Validation Results

- [x] Read all 2,114 lines of [`ARCHITECTURE.md`](ARCHITECTURE.md).
- [x] Read current root/features CMake files and `vcpkg.json`.
- [x] Inspected current analyzer context/types/manager, Sleigh decoder ownership, decompiler provider API, PE loader boundary, and Function ID database loading.
- [x] Compared proposed event, projection, runtime, dependency, and lifecycle contracts with current implementation boundaries.
- [ ] Build, CTest, format, and tidy validation: not run for this read-only review.
- [ ] Full original Ghidra source parity verification: not performed; only the original-source mappings relevant to the architectural boundaries were used.

## Unresolved Questions and Residual Risks

- [ ] Choose the projection/event-bus ownership model before implementing either component.
- [ ] Define whether derived analysis history is fully auditable, compactable, or rebuildable from run manifests.
- [ ] Define snapshot physical semantics and analyzer conflict granularity before enabling parallel analysis.
- [ ] Establish content-addressed artifact/resource policy or explicitly support history-only projects.
- [ ] Benchmark decoder concurrency and event-log replay using a representative large executable.
- [ ] Validate platform-specific project locking and crash recovery on the supported Windows toolchain.

## Final Follow-Up Decision

- [ ] No implementation fixes were started.
- [ ] The architecture should be revised before implementation begins, starting with HIGH-003, HIGH-004, HIGH-002, and HIGH-001, followed by HIGH-005 through HIGH-011 and the Medium findings.
- [ ] The user may authorize fixes for the highest-priority findings in this order: HIGH-003, HIGH-004, HIGH-002, then HIGH-001.
