# Architecture Review Report

## Review metadata

- **Scope:** Research review of [`ARCHITECTURE.md`](ARCHITECTURE.md), compared with the current `NEW` tree, its CMake/module/test/documentation files, the task requirements in [`../TASKS/architecture.md`](../TASKS/architecture.md), and the relevant original Ghidra sources under [`../Ghidra/`](../Ghidra/).
- **Reviewed revision:** Working-tree `NEW/ARCHITECTURE.md` (untracked at review time), against repository commit `f6008f17d940c0425c80e7d660eb9fd455508aac`.
- **Date:** 2026-09-16.
- **Reviewer:** Kilo.
- **Assumptions:** Paths in the report are relative to `NEW/` unless prefixed with `../`. The architecture document is intended to be an implementable specification, not merely a conceptual sketch.

## Review status

- [x] Scope confirmed: architecture document, current implementation, original sources, task requirements, and build/test metadata were inspected.
- [x] Source inspection completed for the current Sleigh, decompiler, PE, Function ID, analyzer, CMake, and shared-state boundaries.
- [x] Test and fixture metadata inspected, including feature-local CMake files and analyzer integration documentation.
- [x] Original-source inspection completed for native translation/Sleigh contracts, Java address/listing/instruction contracts, Function ID, PE loading, and analysis scheduling.
- [x] No implementation files or `ARCHITECTURE.md` were modified during the review.
- [ ] Build/test execution: not performed because this was a research-only review and no implementation change was requested.

## Findings — critical

### No findings

No confirmed critical-severity defect was identified. The high-severity findings below still block an unambiguous implementation of the target architecture.

## Findings — high

### HIGH-001 — The mandated `NEW/framework` core is silently replaced by `core/domain`

- [ ] **Remediation status:** Open.
- **Reference:** [`ARCHITECTURE.md:16,54,142-146`](ARCHITECTURE.md); [`../AGENTS.md:8`](../AGENTS.md); [`../TASKS/architecture.md:99-109`](../TASKS/architecture.md).
- **Affected component:** Core model location and migration plan.
- **Technical evidence:** The architecture states that `NEW/framework` does not exist and makes `NEW/core/domain` the canonical core. The repository rules explicitly require `NEW/framework` to contain the currently limited p-code core and define its long-term facts/hypotheses direction. The current tree inspection also found no `NEW/framework` directory, so this is both an unresolved repository discrepancy and an architecture omission.
- **Expected behavior:** The specification must explicitly reconcile the required `framework` location with the proposed `core` layout: either preserve `framework` as the current p-code core and define its migration into `core/domain`, or document an approved replacement and its compatibility boundary.
- **Actual behavior:** An implementation agent is told to create `core/domain` while receiving no migration contract for the required framework/p-code model. The future knowledge section only describes a later projection and does not identify the existing required framework boundary.
- **Impact:** The implementation can violate the repository’s architectural rule, duplicate or discard the p-code core, and make the mandated RDF/fact/hypothesis evolution inconsistent with the proposed domain model.
- **Failure scenario:** An agent follows the target tree, creates `core/domain/pcode.cppm`, and leaves the required `NEW/framework` contract absent. A later framework-dependent feature has no specified import boundary or canonical p-code type.
- **Root cause:** The document follows the conceptual task tree but does not reconcile it with the repository-level `AGENTS.md` rule.
- **Recommended fix:** Add an explicit current/final mapping for `NEW/framework`, identify its actual expected contents, and state whether `core/domain` is a rename, a successor, or a façade over framework types. Preserve the p-code-focused boundary and its tests in the migration order.
- **Regression risks:** Moving the boundary without explicit adapters can create a third p-code vocabulary and break Sleigh/decompiler/FID conversions.
- **Relevant validation:** Directory search found no `NEW/framework/**`; [`../AGENTS.md:8`](../AGENTS.md) and [`../TASKS/architecture.md:107`](../TASKS/architecture.md) both require that concept to be considered.

### HIGH-002 — `core/contracts` and `core/events` form a dependency cycle

- [ ] **Remediation status:** Open.
- **Reference:** [`ARCHITECTURE.md:123-129`](ARCHITECTURE.md), [`ARCHITECTURE.md:264-268`](ARCHITECTURE.md), [`ARCHITECTURE.md:701-710`](ARCHITECTURE.md), and the target tree [`ARCHITECTURE.md:1351-1377`](ARCHITECTURE.md).
- **Affected component:** Event-store, projection, and event-schema contracts.
- **Technical evidence:** The layer table permits `core/contracts` to depend on `core/domain` but not on `core/events`; it permits `core/events` to depend on contracts. However, `IEventStore` and `IProjection` are declared in `core/contracts` with `EventEnvelope`, `EventStream`, and `EventBatch` types, while `EventEnvelope` and event payloads are placed in `core/events`. `EventStream` and `EventBatch` do not receive defining modules in the target tree.
- **Expected behavior:** The dependency graph must remain acyclic and every contract parameter/result must have an owning module. Event transport/schema values must live below both contracts and event payload definitions, or the dependency direction must be changed consistently.
- **Actual behavior:** Implementing the shown interfaces requires either importing `core/events` into `core/contracts` (contrary to the layer rules) or importing contracts into events and creating a cycle.
- **Impact:** The stated C++ module graph cannot be implemented without violating the forbidden-dependency rules or inventing an additional unrecorded layer.
- **Failure scenario:** Implement `event_store.cppm` exactly as line 266 specifies. `EventEnvelope` is only available from `core/events/event.cppm`; importing it makes contracts depend on events. Implement event payload modules as specified; they depend on contracts. MSVC module dependency scanning then exposes a cycle or forces duplicated envelope declarations.
- **Root cause:** Persistent event payloads, the event envelope/stream transport types, and persistence interfaces were placed in mutually dependent layers without a lower-level shared schema boundary.
- **Recommended fix:** Split immutable event schema/transport values (`EventEnvelope`, `EventBatch`, `EventStream`, event keys) into a lower acyclic module, or make contracts own the schema and make `core/events` depend only on domain/schema values. Define all batch/stream ownership and serialization explicitly.
- **Regression risks:** Moving envelope types can affect codec lookup, replay compatibility, event IDs, and public command responses; add compile-time module dependency checks and replay fixtures.
- **Relevant validation:** The contradiction is visible by comparing the layer rules with the `IEventStore`/`IProjection` signatures and the event-module tree; no build was run because this review did not modify code.

### HIGH-003 — Event publication is ordered before projection application

- [ ] **Remediation status:** Open.
- **Reference:** [`ARCHITECTURE.md:768-802`](ARCHITECTURE.md), especially [`ARCHITECTURE.md:780-787`](ARCHITECTURE.md), and the conflicting sequences at [`ARCHITECTURE.md:606-618`](ARCHITECTURE.md) and [`ARCHITECTURE.md:1125-1133`](ARCHITECTURE.md).
- **Affected component:** Event store, event bus, projection, and analyzer scheduler consistency.
- **Technical evidence:** The append protocol says: flush the event log, publish committed envelopes to the event bus, then apply them to the current projection. The bus requirements say the projection is a subscriber and the analysis scheduler consumes post-commit triggers. The sequence diagrams instead show projection application before bus publication.
- **Expected behavior:** A subscriber that receives event revision `R` must be able to query a projection at least through `R`, or the architecture must define a revision-aware read/queue protocol. The durable projection apply and checkpoint must precede scheduler/UI publication, or an equivalent transactional outbox must be specified.
- **Actual behavior:** The normative prose and diagrams prescribe different orders. Following the append protocol lets analyzers react to an event before SQLite or an immutable query snapshot contains the event’s state.
- **Impact:** An analyzer can read stale state, issue duplicate proposals, fail optimistic revision checks, or observe a function/reference event without its corresponding instruction/function row. The same ambiguity affects UI subscribers and crash recovery.
- **Failure scenario:** Append `FunctionCreated` at revision `R`; publish it; the scheduler captures a projection snapshot still at `R-1` and cannot find the function; projection application then completes. The scheduler either drops the trigger or retries nondeterministically.
- **Root cause:** The document treats event-log durability, projection application, and in-process publication as separate steps but does not define their consistency barrier.
- **Recommended fix:** Make the canonical order explicit: append durable events, apply the projection transaction and checkpoint, then publish; define recovery for a crash between each step. If publication must precede projection, require every consumer to read/replay from the event store at its event revision and document that cost.
- **Regression risks:** Reordering can change analyzer scheduling and observable event timing; add ordering, crash-window, replay, stale-read, and duplicate-delivery tests.
- **Relevant validation:** The three cited sections contain directly conflicting orders; no runtime implementation currently exists to resolve the contradiction.

### HIGH-004 — Public contracts expose service-specific or undeclared types

- [ ] **Remediation status:** Open.
- **Reference:** [`ARCHITECTURE.md:225-269`](ARCHITECTURE.md), [`ARCHITECTURE.md:376-385`](ARCHITECTURE.md), [`ARCHITECTURE.md:445-459`](ARCHITECTURE.md), [`ARCHITECTURE.md:475-489`](ARCHITECTURE.md), and the target tree [`ARCHITECTURE.md:1321-1367`](ARCHITECTURE.md).
- **Affected component:** `core/contracts` API completeness and public C++ facade.
- **Technical evidence:** `IPELoader` is a core contract but its conceptual `LoadResult` contains `PeLoadDetails`, explicitly described as service-specific. `IDecompiler` returns `core::Decompilation` and `core::VariableDescription`, but no corresponding domain modules exist in the target tree. `IPCodeDecoder` uses `ProcessorContext`, `DecodeRequest`, `DecodeBatchRequest`, and `DecodeBatchResult` without defining their final ownership. `IAnalyzer` uses `AnalysisSnapshot`, `EventBatch`, and `MutationCommand`; the only shown `AnalysisSnapshot` location is under runtime analysis. The decompiler provider mapping additionally names `IDataTypeProvider`, `IPrototypeProvider`, `ICommentProvider`, `IVariableProvider`, `IFlowOverrideProvider`, and `IInjectionProvider`, but these modules are absent from the contracts tree.
- **Expected behavior:** Every contract-visible type must have an exact module, layer, ownership, serialization, error, and lifetime definition. Core contracts must not return concrete PE-service details or depend on runtime implementation types.
- **Actual behavior:** The signatures are not implementable from the proposed tree without adding undocumented modules, importing runtime into core, or leaking service-specific types into the contract layer.
- **Impact:** Different implementation agents will invent incompatible APIs; the promised stable native facade and language bindings cannot be generated from this specification.
- **Failure scenario:** Implement `IPELoader` in `core/contracts/pe_loader.cppm`: either define `PeLoadDetails` in core (violating the stated service boundary) or return an opaque/PE type not described by the contract. Implement `IAnalyzer`: importing `runtime/analysis/analysis_snapshot.cppm` violates the core-to-runtime dependency rule.
- **Root cause:** Conceptual method signatures were written before the type catalog and final folder tree were reconciled.
- **Recommended fix:** Add a complete contract type map, move generic result types into core/domain or core/contracts, keep PE details behind a typed opaque/service result boundary, and add all justified provider modules to the tree. For each interface, specify whether its result is canonical domain data, a service-local detail, or an event/mutation proposal.
- **Regression risks:** Normalizing provider types can expose behavior lost from the existing decompiler provider vocabulary; retain compatibility adapters and tests for storage pieces, flow overrides, injections, and PE parse diagnostics.
- **Relevant validation:** Current implementations contain the missing source vocabularies in [`features/sleigh_runtime/sleigh_runtime.cppm`](features/sleigh_runtime/sleigh_runtime.cppm), [`features/decompiler/src/decompiler.cppm`](features/decompiler/src/decompiler.cppm), and [`features/pe_loader/src/pe_loader.cppm`](features/pe_loader/src/pe_loader.cppm), but the proposed final contract tree does not map them all.

### HIGH-005 — Source references are not valid relative paths, and one cited original file does not exist

- [ ] **Remediation status:** Open.
- **Reference:** [`ARCHITECTURE.md:38-77`](ARCHITECTURE.md), [`ARCHITECTURE.md:366-393`](ARCHITECTURE.md), and [`ARCHITECTURE.md:1312-1558`](ARCHITECTURE.md).
- **Affected component:** Port traceability, navigation, and source-to-original mapping.
- **Technical evidence:** The document is located in `NEW/`. References such as `NEW/features/...` therefore resolve to `NEW/NEW/features/...` when treated as relative documentation paths; current files are under `features/...`. References such as `Ghidra/...` resolve to `NEW/Ghidra/...`; the original tree is at `../Ghidra/...`. Line 368 lists `src/address.cppm` and related paths without the feature prefix, but `NEW/src/address.cppm` does not exist. Line 62 lists `error.cc`; the original native tree contains `error.hh` but no `error.cc`.
- **Expected behavior:** Architecture evidence must use valid relative paths from `NEW/ARCHITECTURE.md`, or explicitly declare repository-root notation and use links that resolve from the document.
- **Actual behavior:** A reader following the evidence cannot navigate to many current or original sources, and at least one original source reference is factually invalid.
- **Impact:** Port behavior cannot be independently traced, and an implementation agent may search for nonexistent files or miss the actual Java/native source.
- **Failure scenario:** Follow `NEW/features/sleigh_runtime/...` from the document and land at a nonexistent `NEW/NEW/features/...`; follow `Ghidra/Features/Decompiler/.../error.cc` and search for a file absent from the repository.
- **Root cause:** Paths were written as repository-root paths in a file whose documentation rules require relative references, and the original native file list was not checked against the actual tree.
- **Recommended fix:** Normalize current references to `features/...`, original references to `../Ghidra/...`, and replace `error.cc` with the actual header/source ownership after inspecting `error.hh`. Convert important references to verified Markdown links where practical.
- **Regression risks:** Bulk path normalization can accidentally change paths intended for source comments or command examples; validate every link from the document’s directory.
- **Relevant validation:** Glob checks confirmed the actual current duplicate files under `features/sleigh_runtime/src` and `features/decompiler/src`, confirmed `Ghidra/Features/Decompiler/src/decompile/cpp/error.hh`, and returned no `error.cc`.

### HIGH-006 — The dependency graph reverses the layer rules

- [ ] **Remediation status:** Open.
- **Reference:** [`ARCHITECTURE.md:119-132`](ARCHITECTURE.md) and [`ARCHITECTURE.md:1251-1310`](ARCHITECTURE.md), especially [`ARCHITECTURE.md:1272-1297`](ARCHITECTURE.md).
- **Affected component:** Dependency direction and forbidden-dependency enforcement.
- **Technical evidence:** The layer rules state that services depend on core contracts/domain, runtime composes services, and the public facade depends on runtime. The first architecture diagram uses arrows from facade toward runtime/services/contracts/domain. The section titled “Dependency Graph” instead draws `DOM --> CON --> SL`, `AN --> PJ`, `W --> DS`, and `API --> PJ`; under the earlier arrow convention these mean domain depends on contracts, contracts depend on Sleigh, analyzers depend on project, workers depend on dispatcher, and the facade depends directly on project rather than the runtime facade boundary.
- **Expected behavior:** One arrow convention must be declared and every graph must represent the same dependency direction. The graph must satisfy the forbidden dependencies immediately below it.
- **Actual behavior:** An implementer can follow the second graph and create dependencies that the layer table forbids.
- **Impact:** CMake/module imports can be inverted, creating cycles and coupling core to services/runtime; the architecture loses its role as an enforceable boundary specification.
- **Failure scenario:** Implement `core/domain -> core/contracts -> services/sleigh` as shown, then discover that domain imports contract declarations and contracts import concrete service APIs, contrary to sections 3.1 and 15’s forbidden list.
- **Root cause:** The diagrams use inconsistent semantic meanings for arrows (data/control flow versus dependency direction) without labeling them.
- **Recommended fix:** Replace the graph with a dependency-only diagram whose arrows are explicitly “depends on,” or reverse the arrows consistently and add a separate runtime call/data-flow diagram.
- **Regression risks:** Correcting arrows may expose additional missing edges, especially event schema and runtime-to-service composition; validate the graph against a generated module import graph during implementation.
- **Relevant validation:** The contradiction is internal to the cited architecture sections; no implementation graph exists yet to disambiguate it.

## Findings — medium

### MEDIUM-001 — `InstructionReference` is specified but missing from the final folder tree and catalog

- [ ] **Remediation status:** Open.
- **Reference:** [`ARCHITECTURE.md:179-188`](ARCHITECTURE.md), [`ARCHITECTURE.md:1321-1350`](ARCHITECTURE.md), and [`ARCHITECTURE.md:1564-1592`](ARCHITECTURE.md).
- **Affected component:** Instruction/reference domain model.
- **Technical evidence:** Section 4.4 assigns `InstructionReference` to `core/domain/instruction_reference.cppm` and requires operand index, exact reference class, original flow, fall-through, target, and source classification. The proposed tree contains `instruction.cppm` and `reference.cppm` but no `instruction_reference.cppm`; the class/module catalog also has no row for it.
- **Expected behavior:** The final tree and catalog must contain every required domain module or explicitly merge the type into another module with the ownership and API consequences documented.
- **Actual behavior:** The architecture requires a type that cannot be located in the proposed implementation tree.
- **Impact:** A later implementer may reduce rich instruction-level references to the simpler listing `Reference`, losing the operand/flow information required by the original instruction and FID contracts.
- **Failure scenario:** Implement only `core/domain/reference.cppm`; decompiler/Sleigh adapters then have no specified place for instruction-prototype flow metadata before projection into listing references.
- **Root cause:** Section 4.4 was expanded after the folder tree/catalog and was not synchronized.
- **Recommended fix:** Add the module and catalog row, or document a deliberate merge into `instruction.cppm`/`reference.cppm` and define the conversion boundary.
- **Regression risks:** Merging the type can recreate a large instruction aggregate; keep the instruction-level and listing-level reference semantics distinct.
- **Relevant validation:** Direct inspection of the proposed tree shows no `instruction_reference.cppm` entry.

### MEDIUM-002 — The domain design does not satisfy the requested per-type implementation details

- [ ] **Remediation status:** Open.
- **Reference:** [`../TASKS/architecture.md:165-229`](../TASKS/architecture.md) and [`ARCHITECTURE.md:142-221`](ARCHITECTURE.md).
- **Affected component:** Core/domain specification quality.
- **Technical evidence:** The task requires, for each proposed domain type, exact module/name, purpose, fields, ownership, value/reference semantics, mutability, dependencies, consuming services, reason for core placement, original source, serializability, and public-API status. The architecture tables provide useful fields and some immutability statements, but do not provide those decisions consistently for types such as `AddressRange`, `AddressFactory`, `Operand`, `FlowInfo`, `BasicBlock`, `Relocation`, `DataObject`, and the nested signature/fact values. The class catalog covers only a subset of domain types and does not add the missing per-type columns.
- **Expected behavior:** The specification should provide a complete domain catalog or define a common policy with explicit exceptions for every type in the tree.
- **Actual behavior:** Implementers still have to decide ownership, serialization, public exposure, and service usage for multiple foundational values.
- **Impact:** Independent modules can choose incompatible identity, lifetime, and serialization behavior, especially for address ranges, p-code, facts, and nested function data.
- **Failure scenario:** One implementation treats `AddressFactory` as a runtime-owned mutable registry while another treats it as a serialized immutable value; providers and public queries then cannot share a stable contract.
- **Root cause:** The document provides a broad conceptual domain list but not the requested implementation-level metadata for every entry.
- **Recommended fix:** Extend the domain table/catalog with the required fields or add a clearly scoped common policy and an exceptions table.
- **Regression risks:** Over-specifying every nested value can freeze the public ABI prematurely; distinguish source-stable module contracts from future ABI policy.
- **Relevant validation:** Compared the task checklist with all Section 4 rows and the Section 17 catalog; the required metadata is not uniformly present.

### MEDIUM-003 — SQLite, environment, and build integration are not specified against the current build

- [ ] **Remediation status:** Open.
- **Reference:** [`ARCHITECTURE.md:817-838`](ARCHITECTURE.md), [`ARCHITECTURE.md:1473-1524`](ARCHITECTURE.md), current [`vcpkg.json`](vcpkg.json), and current [`CMakeLists.txt`](CMakeLists.txt).
- **Affected component:** Runtime storage and implementation/build plan.
- **Technical evidence:** The architecture selects SQLite and says it is available through the C++ package/build ecosystem. Current `NEW/vcpkg.json` declares only `gtest`, `pugixml`, and `zlib`; current root CMake adds only `features` and links the existing feature targets. No SQLite package, `find_package`/link target, runtime CMake target, CTest registration, or storage build wrapper is mapped. The document also omits the required `.env`/`GHIDRA_INSTALL_DIR` configuration table and the repository wrapper validation path.
- **Expected behavior:** The architecture-only plan should name the intended vcpkg package/SQLite integration, target graph, CTest/build-wrapper changes, and environment/configuration boundary needed by the proposed runtime.
- **Actual behavior:** The first implementation step involving `sqlite_projection_store.cppm` cannot be configured from the current declared dependency/build graph without additional architectural decisions.
- **Impact:** Runtime implementation fails during configuration or diverges in storage/build integration; validation requirements are unclear.
- **Failure scenario:** Add the proposed SQLite module and run the existing `NEW\build.bat`; no SQLite dependency or target is available to satisfy the module’s build.
- **Root cause:** Storage was designed in isolation from the current vcpkg/CMake/wrapper graph.
- **Recommended fix:** Add a build/configuration section mapping SQLite, CMake targets, tests, wrappers, and required environment handling; keep the exact dependency decision separate from service contracts.
- **Regression risks:** Adding a database dependency affects clean configure, triplet availability, license/package policy, and full-project test time.
- **Relevant validation:** Current [`vcpkg.json`](vcpkg.json) and [`CMakeLists.txt`](CMakeLists.txt) were read directly; no build was run in this research review.

### MEDIUM-004 — The proposed tree does not carry the required test and port-evidence boundaries into new services

- [ ] **Remediation status:** Open.
- **Reference:** [`../AGENTS.md:10,23-30`](../AGENTS.md), [`ARCHITECTURE.md:1380-1393`](ARCHITECTURE.md), [`ARCHITECTURE.md:1490-1504`](ARCHITECTURE.md), and [`ARCHITECTURE.md:1552-1558`](ARCHITECTURE.md).
- **Affected component:** Translation engine, runtime persistence, and test/documentation architecture.
- **Technical evidence:** The repository rules require tests for every feature module and README/GHIDRA_PORT synchronization for C++ modules/ports. The target tree gives the shared `translation_engine` a README and GHIDRA_PORT but no tests directory; it gives event store, storage, projections, and analysis directories READMEs but no local tests. The root `tests/integration`, `tests/replay`, and `tests/fixtures` entries have no README/CMake/CTest ownership or test matrix in the architecture.
- **Expected behavior:** Every proposed port/service boundary should identify its test directory, target, CTest registration, original-behavior fixtures, and port-evidence document; shared runtime tests should cover event recovery, projection idempotence, concurrency, and cancellation.
- **Actual behavior:** The document mentions replay/integration tests in the implementation order but does not specify where the tests live or how they are built and synchronized.
- **Impact:** The highest-risk new behavior—shared native consolidation and event-sourced persistence—can be implemented without the required regression coverage or port evidence.
- **Failure scenario:** Implement the native translation merge or event log with only existing feature tests; no target-level test is defined for cross-service address/p-code equivalence or truncated-log replay.
- **Root cause:** Existing feature-local test evidence was mapped, but the proposed new module hierarchy was not given corresponding test/documentation ownership.
- **Recommended fix:** Add test directories/targets to the final tree and implementation order, including translation-engine parity, runtime event-store/projection crash tests, scheduler concurrency tests, and CMake/CTest registration.
- **Regression risks:** Broad integration tests can become slow; retain focused module tests and bounded fixtures as well as one final GTA5-scale check.
- **Relevant validation:** Existing feature tests were inspected; no proposed `translation_engine/tests` or runtime persistence test directories appear in the target tree.

### MEDIUM-005 — The stated one-class/struct-per-module rule conflicts with the proposed files

- [ ] **Remediation status:** Open.
- **Reference:** [`ARCHITECTURE.md:146`](ARCHITECTURE.md), [`ARCHITECTURE.md:150-175`](ARCHITECTURE.md), [`ARCHITECTURE.md:181-219`](ARCHITECTURE.md), and [`../AGENTS.md:68-69`](../AGENTS.md).
- **Affected component:** C++23 module granularity and source layout.
- **Technical evidence:** The document says primary declarations use one class/struct per `.cppm`, while `identifiers.cppm` is assigned eight independent wrappers, `diagnostics.cppm` four types, `address_space.cppm` three types, and `pcode.cppm` both `PcodeOp` and `PcodeSequence`. The target tree preserves those grouped files.
- **Expected behavior:** The architecture should either split independent primary types into dedicated modules or define a precise exception for value-family modules that is compatible with the repository’s one-class-per-module rule.
- **Actual behavior:** The prose, target tree, and module catalog give conflicting implementation guidance.
- **Impact:** Agents can produce different module graphs, CMake file sets, and import boundaries; generated bindings and documentation cannot reliably map one class to one module.
- **Failure scenario:** One agent creates `identifiers.cppm` containing all wrappers; another creates one module per identifier as the repository rule suggests. Consumers and CMake target names diverge before implementation is complete.
- **Root cause:** The target domain grouping was chosen for documentation compactness without reconciling it with the source-layout rule.
- **Recommended fix:** Split independent classes/structs or explicitly classify grouped declarations as a permitted value-family exception and list the resulting module API.
- **Regression risks:** Excessive splitting can increase module rebuild cost; retain cohesive modules only where the types are inseparable and document that rationale.
- **Relevant validation:** The contradiction is directly visible between the statement at line 146, the grouped type tables, and [`../AGENTS.md:69`](../AGENTS.md).

### MEDIUM-006 — `IEventStore::append` accepts committed envelopes while the store assigns commit fields

- [ ] **Remediation status:** Open.
- **Reference:** [`ARCHITECTURE.md:254-268`](ARCHITECTURE.md), [`ARCHITECTURE.md:748-766`](ARCHITECTURE.md), and [`ARCHITECTURE.md:778-787`](ARCHITECTURE.md).
- **Affected component:** Event append API, idempotency, and serialization.
- **Technical evidence:** `IEventStore::append` accepts `span<const EventEnvelope>`, but `global_sequence` is required and assigned by the store; payload length/checksum are required in the binary record; correlation/causation/source metadata are envelope fields. The document never defines a draft/pre-commit envelope, whether caller-provided sequence values are ignored, or how event IDs are reused on retry.
- **Expected behavior:** The API must distinguish an uncommitted event draft from a committed envelope, specify which fields the store fills, and define idempotent retry behavior and sequence assignment for batches.
- **Actual behavior:** Callers appear to supply a complete envelope whose store-owned fields are simultaneously required and overwritten.
- **Impact:** Different implementations can produce incompatible event IDs/sequences, duplicate events on retry, or reject valid mutation proposals because required commit metadata is unavailable before append.
- **Failure scenario:** A command constructs an envelope without `global_sequence` and checksum and calls `append`; one implementation fills them, another rejects the record because the contract says they are required.
- **Root cause:** Physical record schema and pre-commit command/event API were conflated.
- **Recommended fix:** Define `EventDraft`/`EventBatchDraft` and `CommittedEventEnvelope`, list store-assigned fields, and specify retry/idempotency/partial-append behavior.
- **Regression risks:** Changing event identity or sequence semantics affects replay, projection checkpoints, and analyzer causation links; add compatibility/version tests before freezing the format.
- **Relevant validation:** Compared the contract signature with the envelope table and append protocol; no implementation currently resolves the mismatch.

## Findings — low

### No findings

No additional low-severity findings were retained after prioritization. Minor wording and naming issues were omitted in favor of the actionable findings above.

## Verified strengths

- [x] The document correctly identifies the current `AnalysisContext` as a large mutable owner and names its concrete state categories; this matches [`features/analyzers/shared/src/analyzer_context.cppm`](features/analyzers/shared/src/analyzer_context.cppm).
- [x] The duplicate native translation boundary is well supported by the current CMake lists and original files such as [`../Ghidra/Features/Decompiler/src/decompile/cpp/address.hh`](../Ghidra/Features/Decompiler/src/decompile/cpp/address.hh), [`../Ghidra/Features/Decompiler/src/decompile/cpp/translate.hh`](../Ghidra/Features/Decompiler/src/decompile/cpp/translate.hh), and the corresponding current Sleigh/decompiler modules.
- [x] The command/response/event distinction, synchronous one-instruction decode path, asynchronous expensive decompiler path, shared worker-pool intent, conventional projection-first policy, and future knowledge projection are all present and aligned with the task requirements at a conceptual level.
- [x] The original Java/native references for the address, instruction, PE, Function ID, and analysis scheduler areas were verified to exist except for the specifically reported `error.cc` reference.

## Reviewed areas with no retained findings

- [x] Current root/features CMake target inventory and the stated current absence of runtime/services layers.
- [x] Current Sleigh public value boundary and its native callback adapter.
- [x] Current decompiler provider vocabulary and native engine source list.
- [x] Current Function ID parser/database ownership and the existing `std::async` migration point.
- [x] Current PE model coverage and the proposed separation of generic memory values from PE-specific details.
- [x] Original `Instruction.java`, `SleighInstructionPrototype.java`, native `Sleigh`, `LoadImage`, and `Translate` contracts used as architecture anchors.
- [x] End-to-end GTA5 workflow, lifecycle, concurrency, risks, assumptions, and decision-log sections are present at the requested conceptual level; the findings above concern contradictions and implementation gaps within them.

## Validation results

- [x] Read all 1,875 lines of [`ARCHITECTURE.md`](ARCHITECTURE.md).
- [x] Read current root/features CMake files, vcpkg manifest, relevant public/module interfaces, analyzer context/manager, current README files, and representative tests/fixtures metadata.
- [x] Inspected original Java/native source paths and representative method/class contracts.
- [x] Performed path existence checks for the cited current and original source files.
- [ ] Build, CTest, formatting, and tidy validation: not run; this review changed no implementation and the architecture file is a design document.

## Unresolved questions and residual risks

- [ ] Confirm whether the author intentionally uses repository-root path notation in `ARCHITECTURE.md`; if so, it must be labeled consistently because the repository documentation rules require navigable relative references.
- [ ] Confirm whether the missing `NEW/framework` tree reflects an incomplete checkout or an intended replacement; the architecture cannot safely choose between those interpretations.
- [ ] Resolve the event-schema layer before implementation, because it affects every command, projection, replay, and binding module.
- [ ] Validate SQLite package availability for the supported vcpkg baseline before committing the projection build design.
- [ ] Preserve original delay-slot, flow-override, operand-object, FID context, and multi-space semantics when the current feature-local models are converted to the proposed core values.

## Final follow-up decision

- [ ] No implementation fixes were started in this review.
- [ ] Follow-up implementation should begin with `HIGH-001`, `HIGH-002`, `HIGH-003`, and `HIGH-004`, then resolve the remaining high and medium findings before the architecture is used as a coding blueprint.
