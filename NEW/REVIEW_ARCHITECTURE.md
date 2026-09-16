# Architecture Review

## Review Metadata

- **Scope:** `NEW/ARCHITECTURE.md`, the current `NEW/` CMake/module/test tree, and the current Sleigh, decompiler, PE loader, Function ID, and analyzer implementations.
- **Reviewed revision:** Working-tree contents on 2026-09-16. `ARCHITECTURE.md` and source files were not modified.
- **Reviewer:** Kilo, senior architecture review.
- **Path convention:** Paths in this document are relative to `NEW/` unless prefixed with `../`.
- **Validation limit:** This was a read-only architecture review. No build, CTest run, format pass, or tidy pass was run because no implementation change was requested.

## Executive Assessment

The proposed direction is viable, but it is not yet a safe implementation blueprint. The strongest parts are the rejection of a giant mutable `ProgramDB` equivalent, the separation of services from runtime infrastructure, the recognition that Sleigh and the decompiler need one native translation substrate, and the explicit distinction between commands, responses, events, and projections.

The main risks are not naming or layout preferences. They are unresolved contracts that can change observable analysis results or make persistence impractical at GTA5 scale:

- The event log has no compaction, supersession, or derived-analysis retention policy.
- The snapshot, analyzer proposal, conflict, and commit model does not define read-your-writes or order-sensitive analyzer behavior.
- Projection application and event-bus delivery have overlapping ownership in the prose and diagrams.
- The event catalog is not demonstrably complete for the mutable state already present in `AnalysisContext`.
- The dependency graph still wires analyzers and Function ID to concrete feature libraries while the layer rules require contract-oriented composition.
- The current Sleigh implementation serializes all decodes for a shared SLA, while the target workload assumes bulk parallelism.
- Replay depends on external artifacts and resources whose identity and retention policy are optional rather than part of the reproducibility contract.

No Critical finding was confirmed. The Major findings should be resolved in `ARCHITECTURE.md` before the target tree is used as a coding plan.

## Findings: Critical

### No Findings

No issue was judged likely to make the entire project fundamentally impossible. The Major findings below are still implementation-blocking because they affect correctness, performance, or durable compatibility.

## Findings: Major

### MAJOR-001: Append-Only History Has No Growth or Supersession Strategy

- **Severity:** Major.
- **Problem:** `ARCHITECTURE.md` makes `events.log` authoritative and persists materialized instruction, reference, function, symbol, fact, and analysis changes, but it specifies no snapshotting, log compaction, retention, or supersession model. Batch events reduce framing overhead but do not solve unbounded growth when analysis is repeated or when a large body is revised. The current context already owns large collections in [`services/analyzers/shared/src/analyzer_context.cppm:437-461`](services/analyzers/shared/src/analyzer_context.cppm), and re-analysis can revisit those collections in [`services/analyzers/shared/src/analyzer_manager.cppm:247-297`](services/analyzers/shared/src/analyzer_manager.cppm).
- **Why it matters:** GTA5-scale analysis can produce a large history of derived p-code, instructions, references, and facts. Re-running analysis or changing a signature can append more derived state without removing or superseding old state. Rebuild time, disk usage, backup size, and projection startup time then grow with every analysis run rather than with the current model. A checksum-protected log is durable, but durability alone does not make the storage operationally bounded.
- **Concrete architectural improvement:** Separate immutable import/user history from derived analysis history. Define a versioned projection snapshot or checkpoint manifest, event retention rules, supersession/tombstone events for replaceable derived facts, and a compaction protocol that retains a snapshot plus an event tail. Define whether old analysis runs remain auditable, compacted, or deleted. Benchmark import, re-analysis, replay, and disk usage with a representative large binary before freezing the event granularity.
- **Trade-offs:** Snapshots and compaction add formats, recovery states, migration tooling, and implementation complexity. Deleting derived history reduces forensic detail unless analysis-run exports are retained separately. Keeping all history is simpler and more auditable but requires explicit storage quotas and archival behavior.

### MAJOR-002: Analyzer Snapshot and Commit Semantics Are Not Precise Enough for Order-Sensitive Ghidra Behavior

- **Severity:** Major.
- **Problem:** The target contract changes analyzers from direct `AnalysisContext` mutation to immutable snapshots and `MutationCommand` proposals, but it does not define read-your-writes, proposal conflict sets, multi-entity atomicity, or the ordering guarantees when multiple analyzers run from the same revision. The current implementation is explicitly order-sensitive: [`services/analyzers/shared/src/analyzer_manager.cppm:205-222`](services/analyzers/shared/src/analyzer_manager.cppm) invokes an analyzer, drains events, and immediately schedules downstream work; analyzer implementations mutate the context through methods such as [`services/analyzers/shared/src/analyzer_context.cppm:936-1010`](services/analyzers/shared/src/analyzer_context.cppm). The architecture leaves concurrent analyzer eligibility as an open question at `ARCHITECTURE.md:2064`.
- **Why it matters:** Numeric priority and a dependency DAG do not fully reproduce Ghidra behavior when body carving, primary-symbol selection, no-return propagation, flow overrides, and source-priority rules depend on mutations made by earlier work. Two proposals based on revision `R` can both validate against the project revision while conflicting on the same function body or symbol. If completion order becomes event order, output can vary between runs; if every proposal requires the whole project revision, unrelated edits cause excessive retries.
- **Concrete architectural improvement:** Define analyzer execution classes: strictly serial/order-sensitive, parallel read-only/proposal, and commutative/idempotent batch analyzers. Give every proposal an explicit read set and entity-version precondition, define all-or-nothing commit for multi-entity batches, and define a deterministic tie-breaker for ready analyzers. For analyzers that need intermediate state, use a local write overlay or split the analyzer into phases rather than silently relying on a later event. Require the scheduler to reject or retry a stale proposal without partially applying it.
- **Trade-offs:** Strong ordering and conflict tracking reduce parallelism and require more metadata in commands. A local overlay increases memory and conversion work. Allowing completion order is faster but weakens reproducibility and makes compatibility regressions harder to diagnose.

### MAJOR-003: Projection and Event-Bus Ownership Is Duplicated

- **Severity:** Major.
- **Problem:** The commit protocol says the coordinator appends, applies the projection, advances its checkpoint, and then publishes at [`ARCHITECTURE.md:955-966`](ARCHITECTURE.md). The event-bus contract nevertheless lists a projection subscriber at [`ARCHITECTURE.md:981-992`](ARCHITECTURE.md), while the pipeline and GTA5 sequence diagrams separately show store-to-projection and store-to-bus delivery at [`ARCHITECTURE.md:1194-1197`](ARCHITECTURE.md) and [`ARCHITECTURE.md:1307-1310`](ARCHITECTURE.md). It is not clear whether the projection is directly coordinated or is an event-bus consumer.
- **Why it matters:** Two projection owners create duplicate-application, checkpoint, and recovery paths. If the coordinator applies the event and the bus applies it again, idempotence becomes a correctness requirement for every projection operation. If the bus is the real projection path, the command response cannot claim the projection is current until delivery completes, and a crash between append and delivery needs a durable inbox/outbox protocol. An analyzer can also observe a committed event before or after the query model depending on which interpretation an implementer chooses.
- **Concrete architectural improvement:** Choose one model and make it normative. The simpler in-process model is: the project commit coordinator appends the event, applies the projection transaction, durably advances the projection checkpoint, then publishes only to non-projection subscribers. If bus-driven projections are required, replace the direct apply step with a durable subscriber checkpoint/inbox and define command completion, replay, duplicate delivery, and crash recovery around it.
- **Trade-offs:** Direct coordination couples `ProjectSession` to projection lifecycle but gives clear read-after-commit behavior. A durable bus/outbox is more extensible and decoupled, but adds another persisted state machine, latency, and recovery cases.

### MAJOR-004: The Event Schema Is Not Closed Over the Existing Mutable Analysis State

- **Severity:** Major.
- **Problem:** The initial event list at [`ARCHITECTURE.md:876-921`](ARCHITECTURE.md) names broad categories, but it does not define how every current state mutation is represented or classified as derived and discardable. `AnalysisContext` contains strings, bookmarks, constant facts, external symbols, data archives, address tables, embedded media, PDB symbols and types, candidate function starts, stack variables, function flags, flow overrides, and signature progress in [`services/analyzers/shared/src/analyzer_context.cppm:437-461`](services/analyzers/shared/src/analyzer_context.cppm) and mutation methods through [`services/analyzers/shared/src/analyzer_context.cppm:1012-1398`](services/analyzers/shared/src/analyzer_context.cppm). The proposed projection tables at [`ARCHITECTURE.md:1017-1044`](ARCHITECTURE.md) likewise do not define replacement or retraction semantics for all of these records.
- **Why it matters:** A replayable projection must receive enough information to reconstruct exactly the current state. Generic events such as `FunctionFlagsChanged` or `AnalysisDiagnosticRecorded` are not sufficient until their payload, identity, source-priority, replacement, and deletion rules are specified. Without retraction or supersession, rerunning analysis can leave stale facts, old signatures, or obsolete references in the current projection. The planned future facts/hypotheses projection also needs provenance and invalidation semantics, not only append-only `FactAdded` records.
- **Concrete architectural improvement:** Create a state-to-event matrix covering every current collection and mutator. For each state type, define entity identity, create/update/remove/replace events, source priority, expected revision, idempotency key, and whether it is authoritative, derived, or transient. Add explicit invalidation/supersession events for facts, signatures, references, and analysis artifacts, or state that those values are rebuilt from a retained run snapshot and are not part of the authoritative log.
- **Trade-offs:** A complete event taxonomy increases schema and codec maintenance. Coarse replacement events are easier to replay but write larger payloads; fine-grained events are smaller and more auditable but create more versioning and ordering cases.

### MAJOR-005: The Dependency Graph Still Couples Services to Concrete Feature Implementations

- **Severity:** Major.
- **Problem:** The layer table says services depend on core contracts and private native code, and runtime composes services. The dependency graph additionally declares `FID --> SL`, `AN --> SL`, `AN --> DEC`, and `AN --> PE` at [`ARCHITECTURE.md:1450-1468`](ARCHITECTURE.md). The current build has the same coupling: `analyzer_shared` publicly links `NewGhidra::PeLoader` and `NewGhidra::SleighRuntime` in [`services/analyzers/CMakeLists.txt:1-7`](services/analyzers/CMakeLists.txt), the aggregate analyzer links every analyzer library at [`services/analyzers/CMakeLists.txt:56-60`](services/analyzers/CMakeLists.txt), and Function ID publicly links Sleigh in [`services/function_id/CMakeLists.txt:25-27`](services/function_id/CMakeLists.txt).
- **Why it matters:** Concrete links make optional analyzers, alternate decoders, and isolated service tests harder to build. They also let feature modules bypass the intended runtime composition boundary and encourage direct access to service-local value types. A future `IProjectQuery` or decoder implementation cannot be substituted without rebuilding a broad aggregate, and dependency cycles become likely when runtime factories are introduced.
- **Concrete architectural improvement:** Make analyzer and Function ID targets depend on core contracts and narrow service contracts only. Inject PE memory, decoder, decompiler, and FID database capabilities through runtime-created provider bundles. Keep concrete service dependencies in composition-root targets or explicit adapter targets. If an analyzer truly requires a concrete native service, document that as a capability-specific exception and prevent the dependency from leaking into the common analyzer contract.
- **Trade-offs:** More adapters and factory wiring are required, and small standalone builds may need a composition fixture. The resulting graph has clearer optionality and permits alternate implementations, but it is less convenient than linking one aggregate library.

### MAJOR-006: `core/domain` Is Becoming a Cross-Feature Choke Point Rather Than a Small Stable Kernel

- **Severity:** Major.
- **Problem:** The stated core rule is a small shared vocabulary, but the proposed domain includes complete `FunctionSnapshot` values, decompiler output and text artifacts, FID scoring/options/results, data-type graphs, architecture/provider details, analysis facts, variables, and project-facing request values across [`ARCHITECTURE.md:270-360`](ARCHITECTURE.md). This is substantially broader than the current independently evolving vocabularies in [`services/decompiler/src/decompiler.cppm:7-531`](services/decompiler/src/decompiler.cppm), [`services/analyzers/shared/src/analyzer_types.cppm:17-419`](services/analyzers/shared/src/analyzer_types.cppm), and the Function ID modules.
- **Why it matters:** A change to decompiler output, FID scoring policy, type archive identity, or analyzer evidence becomes a core contract change that affects projections, bindings, and unrelated services. The full `FunctionSnapshot` also risks recreating a large object graph in value form, despite the stated goal of avoiding a central model. This freezes feature-specific decisions before their compatibility requirements and persistence policies are known.
- **Concrete architectural improvement:** Keep a minimal core kernel for identifiers, address spaces, storage locations, p-code, instructions, references, and generic provenance. Keep decompiler result DTOs, FID policy/results, PE details, and analyzer-specific fact payloads in feature contracts or service modules. Promote a type into core only when at least two independent services require the same semantics and its serialization/identity contract is stable. Use explicit adapters at the runtime and public-facade boundaries.
- **Trade-offs:** Adapters introduce conversion code and may temporarily duplicate value representations. The benefit is lower compile-time coupling and freedom to evolve decompiler/FID behavior without changing the foundational model. This also makes the future facts/hypotheses layer an additive projection rather than a reason to freeze every analysis result in core.

### MAJOR-007: The Proposed Worker Pool Does Not Yet Have a Scalable Decoder Resource Strategy

- **Severity:** Major.
- **Problem:** The target runtime assumes bulk Sleigh work can use a shared worker pool, but the current implementation places one mutable `ghidra::Sleigh` behind a mutex in [`services/sleigh/sleigh_runtime_adapter.cppm:745-768`](services/sleigh/sleigh_runtime_adapter.cppm) and locks it for the entire decode in [`services/sleigh/sleigh_runtime_adapter.cppm:817-850`](services/sleigh/sleigh_runtime_adapter.cppm). The architecture permits a per-worker lease, thread-local decoder, or serialized decoder at [`ARCHITECTURE.md:750-755`](ARCHITECTURE.md) and [`ARCHITECTURE.md:494-520`](ARCHITECTURE.md), but it does not choose or specify the required capacity and memory behavior.
- **Why it matters:** If every decoder instance shares the current cached runtime, bulk decoding for one SLA remains serialized and the worker pool cannot deliver the expected throughput. If each worker reparses the SLA, startup latency and memory use can become excessive. A single global lease can also let a large disassembly monopolize the only decoder while interactive decode waits.
- **Concrete architectural improvement:** Define a decoder resource contract with an explicit concurrency policy. Prefer immutable parsed SLA tables plus worker-local parser/context state; if the native port cannot support that split, use a bounded pool of decoder instances with a lease quota and an interactive reservation. Specify whether decoder construction is eager or lazy, the maximum number of instances, cancellation points, and the benchmark that gates the choice.
- **Trade-offs:** Worker-local state consumes more memory and may require native refactoring. A bounded decoder pool is simpler but adds queueing and lease management. Keeping one mutex is memory-efficient and easiest to preserve, but it should be documented as a deliberate throughput limit rather than presented as scalable bulk execution.

### MAJOR-008: Artifact and Resource Retention Does Not Match the Reproducibility Promise

- **Severity:** Major.
- **Problem:** The project layout permits an external primary executable and optional managed artifact copy at [`ARCHITECTURE.md:1084-1099`](ARCHITECTURE.md). Opening a project loads the artifact before replay and verifies a hash only if configured at [`ARCHITECTURE.md:1121-1132`](ARCHITECTURE.md). SLA, compiler-specification, FID, PDB, and data-archive resources are also represented mainly by paths or resource identities. The assumptions explicitly allow input bytes not to be copied at [`ARCHITECTURE.md:2083-2093`](ARCHITECTURE.md).
- **Why it matters:** The event history can describe a prior analysis while the bytes, SLA, compiler specification, or FID database required to reproduce or extend it are missing, moved, or changed. The current `AnalysisContext` owns a parsed PE image and a decoder constructed from a path in [`services/analyzers/shared/src/analyzer_context.cppm:437-475`](services/analyzers/shared/src/analyzer_context.cppm), so a projection replay alone is not an executable analysis environment. A path plus size and timestamp is not a sufficient identity for deterministic reanalysis.
- **Concrete architectural improvement:** Make the project manifest record content hashes, format/parser versions, architecture/compiler resource identities, and required/optional status for every analysis input. Define two explicit modes: portable/reproducible projects with content-addressed managed artifacts, and history-only projects that can query existing projections but cannot reanalyze without reacquiring matching resources. On mismatch, fail or disable reanalysis explicitly rather than silently using a new resource.
- **Trade-offs:** Managed artifacts and resource bundles consume disk space and slow project creation. Hashing large resources costs I/O. Allowing external references is convenient and avoids duplication, but it must be treated as a weaker portability and reproducibility mode.

### MAJOR-009: Revision-Stamped Query Snapshots Have No Implementable Physical Semantics Yet

- **Severity:** Major.
- **Problem:** `IProjectQuery` promises current queries and `snapshot(selection)` at [`ARCHITECTURE.md:405-418`](ARCHITECTURE.md), and the scheduler promises immutable revision-stamped snapshots at [`ARCHITECTURE.md:1185-1199`](ARCHITECTURE.md). The proposed SQLite schema is primarily a current-state schema with a checkpoint and selected row revisions at [`ARCHITECTURE.md:1017-1044`](ARCHITECTURE.md); it does not define historical row retention or how a long-running native task holds a coherent cross-table view.
- **Why it matters:** Capturing only revision `R` and then querying mutable current tables can mix rows from revisions `R` and `R+1`. Holding a SQLite read transaction across a long decompilation can pin WAL pages and connections, while copying a full function snapshot can be expensive for large functions. Without a defined choice, different implementations will provide different consistency guarantees while all claiming to be immutable snapshots.
- **Concrete architectural improvement:** Define `snapshot(selection)` as either an owned immutable DTO graph captured at one checkpoint, or a bounded read-transaction/connection lease with explicit lifetime and resource limits. State which queries are guaranteed coherent, how large snapshots are chunked, and how a task reports that its source revision is no longer commit-compatible. Do not imply arbitrary historical queries unless projection history or snapshots are retained.
- **Trade-offs:** Owned snapshots use memory and copy time but isolate native work from storage. Read transactions reduce copying but increase database resource pressure and complicate close/recovery. Historical projection tables provide stronger time-travel queries but greatly increase storage and schema complexity.

### MAJOR-010: Single-Writer Persistence Is Not an Enforceable Process Boundary

- **Severity:** Major.
- **Problem:** The design says one logical event writer and one projection writer per project at [`ARCHITECTURE.md:416-418`](ARCHITECTURE.md), [`ARCHITECTURE.md:955-966`](ARCHITECTURE.md), and [`ARCHITECTURE.md:1064-1068`](ARCHITECTURE.md), but `Runtime` is an in-process object and no operating-system project lock, read-only open mode, or multi-process behavior is specified. The physical layout has independent `project.json`, `events.log`, and `projection.sqlite` files at [`ARCHITECTURE.md:1084-1097`](ARCHITECTURE.md).
- **Why it matters:** Two application processes, test runners, or crashed/restarted runtimes can both believe they own the project. SQLite can coordinate its own file, but it cannot make an unrelated append-only log writer safe. A tail truncation or concurrent append can corrupt the history even if each individual writer is internally serialized. Projection and log durability can also diverge without a project-level recovery marker.
- **Concrete architectural improvement:** Specify exclusive project ownership using an OS-level lock file or equivalent, with an explicit read-only mode for inspection. Define atomic manifest updates, log/projection recovery states, fsync/flush guarantees for each durability policy, format migration/version compatibility, and behavior after a process dies during append or projection application. Add multi-process and crash-injection tests to the runtime target.
- **Trade-offs:** Platform-specific locking and crash tests add implementation and CI complexity. Exclusive ownership prevents convenient concurrent writers, but it makes the stated single-writer invariant enforceable instead of advisory.

### MAJOR-011: Asynchronous Operation Lifetime and Shutdown Ownership Are Underspecified

- **Severity:** Major.
- **Problem:** `OperationContext` contains a non-owning `ProgressSink*` and a cancellation value at [`ARCHITECTURE.md:377-395`](ARCHITECTURE.md). The public facade returns task handles and permits project close while tasks exist at [`ARCHITECTURE.md:1390-1415`](ARCHITECTURE.md) and [`ARCHITECTURE.md:1121-1134`](ARCHITECTURE.md), but it does not define who owns progress delivery, what happens when a task handle is dropped, or how a task is prevented from committing after project shutdown.
- **Why it matters:** A queued task can outlive the caller's progress object, runtime facade, or project handle. A task that reaches the commit path after `Closing` can append events to a closed store or retain native resources indefinitely. These are lifetime failures, not merely API inconvenience, and are likely to surface only under cancellation, GUI navigation, or application shutdown.
- **Concrete architectural improvement:** Give each task an owned/shared operation state containing cancellation, progress, project generation, and completion status. Make progress delivery a bounded queue or owned sink rather than a raw pointer. Define task-handle drop semantics, project-close ordering, commit rejection for stale project generations, and the guarantee that all native resource leases are released before `Closed`.
- **Trade-offs:** Shared operation state adds allocation and synchronization to every queued task. Explicit shutdown barriers can delay close while native calls finish. The cost is justified by deterministic ownership and prevents use-after-close behavior across bindings.

## Findings: Minor

### MINOR-001: Stable Address-Space and Entity Identity Rules Need a Deterministic Encoding Policy

- **Severity:** Minor.
- **Problem:** The domain requires stable `AddressSpaceId`, `EntityId`, and instruction/function identities at [`ARCHITECTURE.md:278-291`](ARCHITECTURE.md) and [`ARCHITECTURE.md:313-324`](ARCHITECTURE.md), while the projection examples key instructions by `(space, address)` at [`ARCHITECTURE.md:1028-1036`](ARCHITECTURE.md). The current implementation still uses raw `std::uint64_t` addresses in [`services/analyzers/shared/src/analyzer_types.cppm:17-32`](services/analyzers/shared/src/analyzer_types.cppm). The document does not define how IDs are derived, persisted, or preserved when an SLA changes space order or a function is removed and recreated.
- **Why it matters:** Numeric space IDs or generated entity IDs can change across reopen, resource replacement, or migration. Events and references then point to different logical objects even when their printed addresses match. The mismatch between entity identity and address-only projection keys also leaves rename, replacement, and deletion behavior ambiguous.
- **Concrete architectural improvement:** Define a canonical identity policy: persist the project address-space mapping, derive or store entity IDs at first materialization, define delete/recreate semantics, and state which address changes are identity-preserving. Use stable serialized names or content-derived IDs where numeric assignment is not guaranteed.
- **Trade-offs:** Stable IDs add metadata and make migrations more deliberate. Address-only keys are simpler for the initial PE case, but they should be explicitly limited to a projection optimization rather than the public identity model.

### MINOR-002: Observability Is Optional Where the New Runtime Needs It as a Contract

- **Severity:** Minor.
- **Problem:** Metrics and logging are listed as optional runtime ownership at [`ARCHITECTURE.md:706-723`](ARCHITECTURE.md), while the scheduler, worker pool, event store, resource manager, and replay path introduce many failure and latency modes. Correlation and causation IDs exist in the event envelope, but there is no required structured telemetry for queue latency, lease contention, projection lag, replay progress, event-log recovery, or analyzer retry/conflict counts.
- **Why it matters:** A GTA5-scale analysis can appear hung because of decoder serialization, a blocked SQLite read snapshot, event-bus backpressure, or repeated optimistic conflicts. Without standard diagnostics, these conditions are difficult to distinguish and regressions cannot be measured against the architecture's fairness and performance goals.
- **Concrete architectural improvement:** Define a small observability contract for structured logs, counters, durations, progress, and diagnostics keyed by project, task, analyzer, revision, and correlation ID. Require metrics for worker utilization, decoder lease wait, event append/flush, projection checkpoint lag, replay throughput, and conflict/retry counts. Keep the sink replaceable and avoid making business algorithms depend on a concrete telemetry library.
- **Trade-offs:** Instrumentation adds code paths, storage/output volume, and a small runtime cost. Making it structured and sampled avoids requiring a heavyweight logging dependency in core.

### MINOR-003: Integration and Replay Test Ownership Is Not Fully Mapped to the Build

- **Severity:** Minor.
- **Problem:** The target tree lists `runtime/tests` and `tests/integration`, `tests/replay`, and `tests/fixtures` at [`ARCHITECTURE.md:1681-1769`](ARCHITECTURE.md), but it does not assign CMake/CTest ownership, fixture manifests, or a required test target for those root directories. The current root build only adds `features` and the smoke executable in [`CMakeLists.txt:11-25`](CMakeLists.txt), while existing tests are feature-local such as [`services/analyzers/tests/CMakeLists.txt:1-14`](services/analyzers/tests/CMakeLists.txt). There is also no current runtime or projection implementation to exercise the proposed crash/replay behavior.
- **Why it matters:** The highest-risk architectural boundaries can be implemented with only unit tests for individual parsers and native algorithms. That would miss event ordering, projection idempotence, snapshot consistency, decoder concurrency, close races, and replay equivalence. A target tree without build ownership makes those tests easy to defer indefinitely.
- **Concrete architectural improvement:** Add a test ownership table to the architecture: target name, CMake owner, CTest registration, fixture source, scale, and required oracle/fingerprint. Define focused runtime tests for log framing/recovery, projection replay/idempotence, scheduler conflict ordering, task shutdown, and decoder lease concurrency, plus one bounded large-binary integration test.
- **Trade-offs:** Integration tests increase configuration and runtime cost and may require platform-specific crash injection. Keeping them focused and using deterministic fixtures limits CI impact while preserving the required architectural coverage.

## Solid Decisions to Preserve

These decisions are supported by the current code and original Ghidra boundaries. They should not be changed without strong evidence:

- **Do not make a giant mutable `Program` or `ProgramDB` equivalent the core object.** The diagnosis of [`services/analyzers/shared/src/analyzer_context.cppm`](services/analyzers/shared/src/analyzer_context.cppm) is correct. Its PE image, decoder, listing maps, analysis collections, and transient events are too broad for a concurrent public model.
- **Keep PE parsing separate from program-side effects.** The checked, value-oriented parser in [`services/pe_loader/src/pe_loader.cppm`](services/pe_loader/src/pe_loader.cppm) is reusable and testable. Converting its result into project events is a better boundary than making the parser own listing or database mutation.
- **Consolidate the duplicated native translation substrate.** The proposed shared native target is justified by the duplicate address, space, translate, load-image, context, marshal, opcode, and varnode modules identified at [`ARCHITECTURE.md:471-492`](ARCHITECTURE.md). This is a behavior-preserving consolidation, not unnecessary abstraction.
- **Keep native decompiler state private and task-local.** Per-task `Architecture`/`Funcdata` ownership is the safe baseline for the mutable native engine and should remain separate from immutable project query values.
- **Separate commands, transient responses, and persistent events.** A decode result or failed task is not automatically project history, while a rename, function creation, signature assignment, or reference change needs a durable mutation path. The distinction at [`ARCHITECTURE.md:867-874`](ARCHITECTURE.md) is sound.
- **Use a replaceable projection behind query contracts.** SQLite is a reasonable initial indexed projection for millions of rows if it remains a projection and not a dependency of feature services. The event log should not become a SQL API for analyzers.
- **Retain Ghidra priority compatibility while adding an explicit readiness graph.** The current analyzer manager's lower-number-first ordering and prerequisite checks are valuable compatibility behavior. Replacing numeric priority with a DAG alone would lose useful ordering; the architecture is right to keep both, but must define concurrency classes as described in MAJOR-002.
- **Use bounded shared scheduling rather than unbounded per-feature threads.** Replacing current Function ID `std::async` loading in [`services/function_id/src/database.cppm:260-318`](services/function_id/src/database.cppm) with runtime-owned quotas and cancellation is the right direction. The decoder resource policy still needs the concrete decision identified above.
- **Preserve feature-local port evidence and focused tests during migration.** The current feature tests cover important PE, Sleigh, decompiler, Function ID, and analyzer compatibility behavior. The target architecture should add cross-boundary tests, not replace these suites with only end-to-end tests.

## Reviewed Areas With No Retained Finding

- The current absence of `core`, `runtime`, `services`, `bindings`, and `apps` layers is accurately documented at [`ARCHITECTURE.md:7-18`](ARCHITECTURE.md); it is a migration fact, not evidence that the target layering is invalid.
- The proposed direction of stable domain values below runtime/services is compatible with the repository's C++23 and independent-module goals, provided the core scope is narrowed as described.
- The distinction between immutable PE images, read-only FID resources, and mutable per-task decompiler sessions is a sound ownership model.
- The decision to keep RDF/facts/hypotheses as a future projection rather than making RDF the first persistence dependency preserves the current implementation goals.
- The architecture correctly identifies current parity limitations in Function ID and analyzer/decompiler integration at [`ARCHITECTURE.md:134-150`](ARCHITECTURE.md) instead of claiming that the current ports are complete.

## Recommended Resolution Order

1. Define the event/projection/bus commit model and close the event taxonomy, including invalidation and batch identity.
2. Specify immutable snapshot physical semantics and analyzer conflict/order/read-your-writes behavior.
3. Decide the event-log growth, artifact/resource reproducibility, and process-lock policies.
4. Resolve the service dependency graph and narrow the core domain before creating the proposed target tree.
5. Choose and benchmark the decoder resource strategy before promising bulk parallel analysis.
6. Add runtime integration, crash/replay, conflict, shutdown, and scale-test ownership to the build plan.

## Final Conclusion

The architecture has a strong conceptual center and should be refined rather than discarded. Its current risk is that several important choices are described as compatible alternatives even though they produce materially different correctness and performance behavior. Resolve the Major findings, preserve the listed strengths, and then use the resulting document as the migration contract.
