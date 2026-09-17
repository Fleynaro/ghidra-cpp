# Services Module Review Report

## Review Metadata

- [x] **Scope:** current `HEAD` (`9cd8a17501`), covering `core/contracts` and all matching PE, Sleigh, decompiler, Function ID, analyzer, translation-engine, and service consumers/implementors under `services`.
- [x] **Date:** 2026-09-16.
- [x] **Reviewer:** Kilo, independent read-only pass.
- [x] **Assumption:** native algorithms may remain private compatibility implementations, but runtime-facing calls must honor core contracts and service lifetime rules.

### Scope Extension: domain-model duplication audit

- [x] **Audit scope:** read-only comparison of `services` (analyzers, PE loader, Sleigh, decompiler, Function ID, translation-engine, and hello) against `core/domain` and related `core/contracts` boundaries; `runtime` was also checked for runtime-service model overlap.
- [x] **Audit date:** 2026-09-16.
- [x] **Source changes:** none; only module-local review reports were updated or added by this audit.
- [x] **Comparison rule:** native parser/decompiler engine ownership types were not classified as duplicates when their pointer ownership, graph identity, or algorithm lifecycle differs from core values.

## Review Status

- [x] Scope confirmation.
- [x] Service source, CMake, dependency, fixture, and test inspection.
- [x] Core contract conversion inspection.
- [x] Canonical domain DTO, provider-context, revision, and address-space boundary inspection.
- [x] Validation status recorded.
- [x] Audit baseline recorded; implementation fixes from the authorized follow-up pass are reflected below.

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

- [x] **Remediation status:** Partially fixed. Canonical architecture metadata and function-entry address spaces are now consumed; `sla_path_` resource selection remains a follow-up.
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
- **Technical evidence:** The moved analyzer libraries still link `ReCode::PeLoader`, `ReCode::SleighRuntime`, and `ReCode::DecompilerFrontend` and implement the old mutable `AnalysisContext`/`AutoAnalysisManager` model. The new analyzer service target contains only `EntryMaterializationAnalyzer`; project startup registers only that new `IAnalyzer`.
- **Expected behavior:** Migrated analyzers consume immutable core snapshots/contracts and are registered by runtime analysis.
- **Actual behavior:** The physical move hides the old feature coupling; the default runtime does not execute the moved feature analyzers.
- **Impact:** Alternate service implementations cannot be substituted and feature analysis is absent from the facade path.
- **Failure scenario:** Provide a mock `IPCodeDecoder` or call facade analysis expecting the moved analyzer set; native legacy targets/registrations remain in use or are not invoked.
- **Root cause:** Directory relocation preceded extraction of analyzer algorithms from the legacy aggregate/context.
- **Recommended fix:** Introduce contract-facing analyzer adapters and register them through `ReCode::AnalyzerServices`/runtime registry; keep legacy aggregate only as an explicit diagnostic target.
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

### HIGH-007: Legacy analyzer context owns a parallel domain model

- [ ] **Remediation status:** Open.
- **References:** `analyzers/shared/src/analyzer_types.cppm:19-22,85-168,170-216,273-280,437-458`; canonical counterparts are `../core/domain/address.cppm:9-16`, `../core/domain/address_range.cppm:9-20`, `../core/domain/reference.cppm:10-40`, `../core/domain/basic_block.cppm:9-20`, `../core/domain/function.cppm:13-36`, `../core/domain/data_object.cppm:10-21`, `../core/domain/analysis_fact.cppm:19-53`, `../core/domain/symbol.cppm:13-27`, and `../core/domain/function_signature.cppm:9-33`.
- **Affected component:** All legacy analyzer implementations and the mutable `AnalysisContext` state store.
- **Technical evidence:** `analyzer_types` declares a second numeric `Address`, `AddressRange`, `Reference`, `BasicBlock`, `Function`, `DataObject`, `SymbolRecord`, `ConstantFact`, `StackVariable`, and string-based `FunctionParameter`. `AnalysisContext` stores these in separate maps/vectors instead of core `Address`/`FunctionSnapshot`/`Reference`/`DataObject`/analysis-fact values. Body ranges and CFG rows are rebuilt manually in `analyzer_context.cppm:204-345` and `:830-925`.
- **Expected behavior:** A service-facing analyzer should read immutable core snapshots and return core-contract mutation proposals; one canonical domain vocabulary should cross the service/runtime boundary.
- **Actual behavior:** Every legacy analyzer mutates a second in-memory listing vocabulary, and no general projection converts that vocabulary into core domain facts. Numeric offsets also erase the address-space identity required by core values.
- **Impact:** Runtime projections and legacy analyzer results can diverge; each future analyzer integration must add another field-by-field conversion and can silently lose entity IDs, provenance, address spaces, or typed signatures.
- **Failure scenario:** A multi-space project or a runtime analysis run consumes a legacy analyzer result; the analyzer only exposes `uint64_t` offsets and string types, so the core projection cannot reconstruct the original entity identity or address space without policy guesses.
- **Root cause:** The feature relocation retained `AnalysisContext` and its Java-listing-shaped value model instead of introducing a core snapshot/mutation adapter.
- **Recommended fix:** Keep native mutable context only as a compatibility implementation; add a deliberate core projection boundary, migrate stable values first (`Address`, ranges, references, functions, data objects, facts), then move analyzers to `core::contracts::AnalysisSnapshot`/`AnalyzerResult`.
- **Regression risks:** CFG overlap, thunk/body reconciliation, reference override semantics, and analyzer event ordering must be parity-tested before deleting the compatibility model.
- **Relevant validation:** Existing legacy analyzer fixture tests exercise the local context, but no test asserts equivalence between local context rows and core projection rows.

### HIGH-004: The Sleigh adapter materializes persistent instructions before the caller requests materialization

- [x] **Remediation status:** Fixed. `IPCodeDecoder` returns `core::DecodedInstruction`; `materialize_decoded_instruction()` owns space-aware identity and promotion.
- **References:** `../core/contracts/pcode_decoder.cppm:12-40`, `sleigh/sleigh_service.cppm:86-132`, symbols `IPCodeDecoder`, `SleighService::convert`, and `DecodeInstruction.materialize` in `../core/contracts/command.cppm:34-38`.
- **Affected component:** Decoder service result boundary and listing identity.
- **Technical evidence:** The contract returns `core::Instruction` rather than canonical `core::DecodedInstruction`. `SleighService::convert` creates `instruction-<offset>` IDs, converts only scalar operand values, and maps every `DecodedFlowInfo::target` storage location to `FlowInfo.target` regardless of whether the flow is indirect.
- **Expected behavior:** Decode returns a transient, space-aware `DecodedInstruction`; a separate materializer creates a listing entity only when requested.
- **Actual behavior:** `materialize=false` still produces a persistence-shaped entity and equal offsets in different spaces collide. Register/address operands and indirect-flow storage are lost or misclassified.
- **Impact:** Multi-space projects and downstream CFG/reference analysis receive incorrect identity and flow data before any consumer can repair it.
- **Failure scenario:** Decode `0x100` in two spaces or decode an indirect branch; inspect the result ID/flow and observe `instruction-256` plus a concrete target in the wrong field.
- **Root cause:** The service adapter promoted the native transient record directly into the persistent listing model.
- **Recommended fix:** Return `DecodedInstruction` from the decoder contract, add an explicit space-aware materializer, and preserve register/address objects or return a structured unsupported diagnostic when architecture metadata is unavailable.
- **Regression risks:** Existing listing event tests must move to the materializer; preserve current x86 textual output while adding non-`ram` and indirect-flow coverage.
- **Relevant validation:** Current Sleigh tests cover x86/ARM decode but do not assert contract result identity, address-space collision behavior, or canonical operand/flow fields.

### HIGH-005: Function ID service bypasses its database contract

- [x] **Remediation status:** Partially fixed. Matching now calls `IFunctionIdDatabase::query` and malformed hashes return `core::Error`; relation/options propagation remains open.
- **References:** `function_id/function_id_service.cppm:14-44,56-131`, symbols `FunctionIdDatabaseService::query` and `FunctionIdService::identify_now`; `../core/contracts/function_id_database.cppm:9-20`; legacy direct consumer `analyzers/function_id/src/function_id.cppm:129-193`.
- **Affected component:** Function ID matching, database substitution, relation scoring, and malformed-input handling.
- **Technical evidence:** `identify_now` calls the concrete `database_->database().identify` instead of `IFunctionIdDatabase::query`. The public query parses only `full_hash`, leaves relation hashes empty, and does not honor `force_relations`, language/database filters, or bookmark thresholds. `std::stoull` is not caught in the `Result`-returning query method.
- **Expected behavior:** The matcher uses only the injected contract and forwards all declared hash/options semantics through checked results.
- **Actual behavior:** Contract implementations/mocks are ignored, relation-aware matching is silently reduced, and malformed hashes escape as exceptions.
- **Impact:** Alternate databases cannot be substituted and callers can receive a failed task rather than a structured diagnostic or relation-aware ranking.
- **Failure scenario:** Inject a mock `IFunctionIdDatabase` or call query with `full_hash="not-hex"`; the mock is unused and `std::invalid_argument` escapes.
- **Root cause:** Native Function ID APIs were retained behind a newer service facade without completing the contract adapter.
- **Recommended fix:** Depend on `IFunctionIdDatabase` in the matcher, use bounded `from_chars`, carry parent/child families and filters, and explicitly reject unsupported relation modes.
- **Regression risks:** Native score ordering and database-language filtering need parity tests before replacing direct calls.
- **Relevant validation:** Valid packed database tests do not cover contract injection, relation families, option filters, or malformed hashes.

### HIGH-006: The canonical project-query instruction contract is consumed as complete, but the projection supplies a partial listing

- [x] **Remediation status:** Fixed for hash-critical fields. Listing events now persist/replay bytes, instruction masks, and operand object facts; p-code/flow replay remains a separate artifact concern.
- **References:** `function_id/function_id_service.cppm:84-112`, symbol `FunctionIdService::identify_now`; `../runtime/projections/software_model_projection.cppm:203-221`, symbol `SoftwareModelProjection::apply_locked`; canonical fields `../core/domain/instruction.cppm:18-37`.
- **Affected component:** Function ID hashing and any service consuming `IProjectQuery::instructions()`.
- **Technical evidence:** Function ID hashes `instruction.bytes`, `instruction.instruction_mask`, `instruction.operands`, and `instruction.flow`. The current `ListingStateChanged` projection reconstructs only key, length, mnemonic, assembly, p-code instruction address, and provenance; it does not persist bytes, masks, operands, or flow.
- **Expected behavior:** `IProjectQuery` returns a revision-pinned, complete `Instruction` or the service returns an explicit incomplete-input diagnostic.
- **Actual behavior:** A normal projected listing can be hashed as empty/zero-evidence input, producing misleading Function ID results.
- **Impact:** Function ID matches and downstream analysis silently degrade while all types remain valid.
- **Failure scenario:** Decode and project one instruction, then call `FunctionIdService::identify`; the query contains no bytes/operands and the hash input differs from the decoded instruction.
- **Root cause:** Compact listing events and complete domain DTOs were treated as interchangeable.
- **Recommended fix:** Extend the event/projection codec to preserve hash-critical fields, or add a query capability that explicitly returns decoded/hash records and make Function ID reject incomplete snapshots.
- **Regression risks:** Event schema/version and replay storage increase; preserve compact rendering fields while adding a versioned hash payload.
- **Relevant validation:** No integration test compares Function ID hashing before and after projection/replay.

## Findings: Medium

### MEDIUM-001: Service aggregate and no-test build mode do not cover the migrated service targets

- [ ] **Remediation status:** Open.
- **References:** `CMakeLists.txt:13-24`, `../build.bat:257-263`.
- **Affected component:** Service CMake target graph.
- **Technical evidence:** `recode_services` depends on legacy engine targets but omits the public adapter libraries and translation engine. The `services --no-test` wrapper assignment is overwritten with `recode_service_tests`, which is not defined when `BUILD_TESTING=0`.
- **Expected behavior:** The aggregate builds every public service and the no-test mode builds that aggregate.
- **Actual behavior:** Compile-only service validation can omit adapters or request a disabled test target.
- **Impact:** Migration target health is not reliably validated by the prescribed workflow.
- **Failure scenario:** Invoke `build.bat services --no-test`.
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

- [x] **Remediation status:** Partially fixed. Structured PE symbols/relocations and Sleigh address/indirect-flow promotion are now populated; architecture-specific register resolution remains open.
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

- [x] **Remediation status:** Fixed. Hash parsing is guarded and mapped to `core::Error`.
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

### MEDIUM-012: Analyzer event and descriptor values duplicate core contracts

- [ ] **Remediation status:** Open.
- **References:** `analyzers/shared/src/analyzer_types.cppm:35-62,403-417`; canonical contracts are `../core/events/event.cppm:23-63` and `../core/contracts/analyzer.cppm:26-68`.
- **Affected component:** Legacy `AutoAnalysisManager`, analyzer registration, and runtime analysis scheduling.
- **Technical evidence:** Local `EventKind`/`AnalysisEvent`/`AnalyzerDescriptor`/`AnalysisResult` model an event queue, registration metadata, and analyzer output that are separate from core `EventDraft`/`EventEnvelope`/`EventBatch` and `AnalyzerDescriptor`/`AnalyzerResult`. The local descriptor uses display name plus `EventKind`, while the core descriptor uses stable ID, scope, run policy, execution class, and removal semantics.
- **Expected behavior:** A single contract vocabulary should describe analyzer triggers, ordering, lifecycle, and mutation output.
- **Actual behavior:** The legacy manager schedules local mutable events and returns local strings/errors; the runtime scheduler consumes immutable core event batches and `MutationCommand` values. There is no general, field-preserving bridge.
- **Impact:** Analyzer registration and results cannot be substituted into the runtime scheduler without bespoke adapters, and lifecycle fields (scope, policy, execution class, idempotency) have no representation in the local model.
- **Failure scenario:** Register a legacy analyzer through the runtime contract and attempt queued/project-scoped execution; local descriptor data cannot express the runtime scope/policy, so the adapter must invent defaults.
- **Root cause:** Legacy scheduling vocabulary was preserved beside the new core contract instead of being isolated behind an explicit compatibility facade.
- **Recommended fix:** Introduce a compatibility adapter that maps local descriptors/events to core contracts with explicit default diagnostics; migrate registrations and mutation outputs to core contracts before removing local event types.
- **Regression risks:** Priority tie-breaking, prerequisite validation, cancellation, and removal-event behavior.
- **Relevant validation:** `analyzers/shared` tests cover local manager ordering; no cross-scheduler contract-equivalence test exists.

### MEDIUM-013: Decompiler metadata DTOs repeat core type, variable, flow, and architecture concepts

- [ ] **Remediation status:** Open.
- **References:** `decompiler/src/decompiler.cppm:121-226,238-269,291-357,447-477,488-514`; canonical counterparts are `../core/domain/symbol.cppm:13-27`, `../core/domain/data_object.cppm:10-21`, `../core/domain/data_type.cppm:8-47`, `../core/domain/function_signature.cppm:9-33`, `../core/domain/variable.cppm:16-23`, `../core/domain/analysis_fact.cppm:38-53`, `../core/domain/architecture.cppm:10-24`, and `../core/domain/decompilation.cppm`.
- **Affected component:** Public native decompiler provider metadata boundary and its clients.
- **Technical evidence:** `SymbolDescription`, `TypeDescription`, `PrototypeDescription`, `VariableDescription`, `FlowDescription`, `ArchitectureDescription`, `FunctionDescription`, and `DecompilationResult` carry the same semantic rows as core symbols/data/types/signatures/variables/switch facts/architecture/function snapshots/decompilation, but use numeric addresses and provider-name strings. `decompiler_service.cppm:157-181` and `:133-147` then copy selected fields into native/provider values.
- **Expected behavior:** Shared domain facts should be canonical; provider-only extensions should be named as contract DTOs and converted once at the edge.
- **Actual behavior:** Provider DTOs remain a second public vocabulary. Some are intentionally richer (bitfields, relative pointers, native fixups, raw/high p-code artifacts), but no documented conversion boundary distinguishes canonical fields from provider-only extensions.
- **Impact:** Identical symbols/types/signatures can acquire different identity, address-space, storage, or status semantics depending on whether they came through the project domain or the decompiler provider.
- **Failure scenario:** Supply a core `FunctionSnapshot` signature/variables to a provider and compare it with a provider-returned `PrototypeDescription`; string type names and numeric storage require ad-hoc resolution and may not round-trip to `DataTypeId`/`VariableStorage`.
- **Root cause:** The decompiler frontend predates the richer core domain and retained a source-compatible provider API.
- **Recommended fix:** Keep native-only fixup/injection/raw-artifact DTOs local; move only shared provider contract values to `core/contracts/decompiler` with explicit `Provider*` names and add one tested conversion layer to/from core domain snapshots. Do not alias richer provider DTOs blindly.
- **Regression risks:** Native type declaration identity, bitfield packing, relative-pointer handling, ABI split storage, and generated C output.
- **Relevant validation:** Decompiler provider contract and metadata tests cover provider semantics; no core↔provider round-trip tests cover identities and multi-space addresses.

### MEDIUM-014: Three analyzer adapters copy the same x86-64 architecture model

- [x] **Remediation status:** Fixed for the shared x86-64 provider fixture. All three adapters call `make_x86_64_architecture()` from the decompiler frontend.
- **References:** `analyzers/call_convention_id/src/call_convention_id.cppm:143-173`, `analyzers/decompiler_switch_analysis/src/decompiler_switch_analysis.cppm:69-97`, `analyzers/decompiler_parameter_id/src/decompiler_parameter_id.cppm:66-94`; canonical architecture vocabulary is `../core/domain/architecture.cppm:10-24` and provider contract metadata is `decompiler/src/decompiler.cppm:447-477`.
- **Affected component:** Decompiler-backed analyzers.
- **Technical evidence:** Each `architecture()` function repeats the same four spaces, x86-64 register offsets, stack register, and pointer size, with only the diagnostic name differing. These values are manually pushed into provider-local `ArchitectureDescription`/`SpaceDescription`/`RegisterDescription` rows.
- **Expected behavior:** Architecture facts come from one canonical core snapshot or one shared contract adapter.
- **Actual behavior:** Three implementations can drift independently and are not tied to the loaded PE/SLA architecture.
- **Impact:** Different analyzers may assign different register/storage semantics for the same function; non-x86 or 32-bit analysis silently receives x86-64 facts.
- **Failure scenario:** Change one register offset or invoke only switch analysis for a 32-bit image; the analyzer-specific literal remains x86-64 and produces incompatible native decompiler results.
- **Root cause:** Compatibility adapters were copied into each analyzer instead of shared architecture-provider construction.
- **Recommended fix:** Add a single core-contract/translation-engine factory that converts `core::ArchitectureDescription` to provider metadata; have all three analyzers consume it and reject unsupported architecture identities explicitly.
- **Regression risks:** Existing x86-64 fixtures and provider register aliases must remain byte-for-byte equivalent.
- **Relevant validation:** Each analyzer has focused tests, but no test compares architecture metadata emitted by the three adapters.

### MEDIUM-015: Function ID uses a second operand/instruction/relocation projection

- [x] **Remediation status:** Partially fixed. `fid::OperandObject` is now an alias to `core::OperandObject`; FID-specific instruction/mask records remain intentionally algorithm-local.
- **References:** `function_id/src/types.cppm:35-67`; canonical counterparts are `../core/domain/operand.cppm:8-37`, `../core/domain/decoded_instruction.cppm:9-70`, and `../core/domain/relocation.cppm:8-15`; conversion loops are `function_id/function_id_service.cppm:87-105` and `analyzers/function_id/src/function_id.cppm:73-93,153-166`.
- **Affected component:** Function ID service and analyzer hashing inputs.
- **Technical evidence:** `fid::OperandObject` repeats core operand object kind/value/relocation flags; `fid::Instruction` is a second byte/mask/operand projection; `fid::Relocation` repeats loader-neutral relocation ranges. Both service and analyzer manually copy and static-cast fields before hashing.
- **Expected behavior:** FID-specific hash policy should consume a canonical decoded snapshot and a loader-neutral relocation view, while retaining only algorithm-specific masks/skip flags locally.
- **Actual behavior:** Every FID entry point maintains its own projection and enum casts. The service conversion can silently diverge from the analyzer conversion when core operand kinds or flow values change.
- **Impact:** Hash reproducibility and relocation masking can change across service paths without a compiler error; address-space identity is discarded by `uint64_t` relocation addresses.
- **Failure scenario:** Add a core operand kind or run a multi-space query; one adapter's `static_cast`/numeric assumptions no longer match the FID projection, producing different hashes or masked bytes.
- **Root cause:** FID's algorithm-specific record shape was exposed as if it were the input domain model.
- **Recommended fix:** Keep `fid::Instruction` as a private hash projection, but centralize `core::DecodedInstruction`→FID conversion in `core/contracts/function_id` or one service adapter; pass `core::Relocation`/`Address` explicitly and test enum/value mapping.
- **Regression risks:** Exact Java FunctionID hash compatibility, signed scalar interpretation, and relocation width rules.
- **Relevant validation:** FID hash tests cover native records; no test asserts equal hashes for service and analyzer conversion paths.

### MEDIUM-016: PE loader has overlapping region/relocation models and an incomplete structured-result bridge

- [x] **Remediation status:** Partially fixed. Structured symbols/relocations and pointer-width address metadata are emitted; parser-private section/block records remain by design.
- **References:** `pe_loader/src/pe_loader.cppm:272-305,373-389`; canonical targets are `../core/domain/memory_region.cppm:9-27`, `../core/domain/relocation.cppm:8-15`, and `../core/contracts/pe_loader.cppm:34-43`; conversion is `pe_loader/pe_loader_service.cppm:104-147`.
- **Affected component:** PE parser-to-core loader service boundary.
- **Technical evidence:** The parser's `MemoryRegion` carries the same loaded-range/permissions facts as core `MemoryRegion`, while `RelocationEntry`/`RelocationBlock` carry target/width evidence represented by core `Relocation`. `make_regions` manually reconstructs address-space-aware rows at `pe_loader/pe_loader_service.cppm:104-118`; `make_result` now copies structured symbols/relocations at `:131-173`, but still hard-codes `address_bits` to 64 at `:179-181` and reduces machine identities to x86-64 versus x86-32 at `:174-178`.
- **Expected behavior:** PE-specific section/block records remain parser-private, but the loader emits complete core regions, symbols, and relocations with explicit PE32/PE32+ address-space policy.
- **Actual behavior:** Region facts are copied into a second shape and structured symbols/relocations are emitted, but the architecture conversion remains only partially width-aware and parser-specific section metadata is not represented in the generic result.
- **Impact:** Consumers of `PeLoadResult` cannot perform structured import/export/relocation analysis and may infer a 64-bit address model for PE32.
- **Failure scenario:** Load a PE32 image and inspect `PeLoadResult.architecture.spaces.front().address_bits`; it remains 64 despite the pointer size being 4. Load a non-x86 machine and the adapter still selects an x86 language identity.
- **Root cause:** The adapter was implemented for region/architecture happy paths while parser DTOs remained the authoritative metadata source.
- **Recommended fix:** Keep one tested parser→core mapping (including symbol source/identity and relocation width/type), use `pointer_size * 8` for address bits, map PE machine identities exhaustively, and retain `Section`/`RelocationBlock` only inside the parser.
- **Regression risks:** Forwarded exports, ordinal imports, relocation type width mapping, and partial-parse status.
- **Relevant validation:** PE service tests cover region and architecture basics but do not assert structured symbols/relocations or PE32 address bits.

### MEDIUM-008: Native memory adapters collapse named spaces and volatile metadata

- [x] **Remediation status:** Partially fixed. Core volatile ranges and function-entry spaces are forwarded; native provider remains offset-oriented for non-code spaces.
- **References:** `decompiler/src/decompiler.cppm:86-117`, symbols `MemoryProvider` and `MemoryRangeDescription`; `decompiler/decompiler_service.cppm:13-31`, symbol `LegacyMemoryProvider`; `decompiler/decompiler_service.cppm:44-50`, symbol `ContractPcodeProvider`.
- **Affected component:** Decompiler memory reads and provider conversion.
- **Technical evidence:** Core `IMemoryProvider` reads an `Address`, while native decompiler memory also accepts a named space and exposes `volatile_ranges()`. `LegacyMemoryProvider` reads only `ram`, does not override named-space reads, and supplies no volatile ranges; `ContractPcodeProvider` likewise constructs every decode address in `ram`.
- **Expected behavior:** Code/data/IO/overlay spaces and volatile attributes are preserved or rejected explicitly.
- **Actual behavior:** Native LOAD/STORE requests can read the wrong space and volatile access is treated as ordinary memory.
- **Impact:** Non-`ram` architectures and side-effect-sensitive decompilation can produce incorrect semantics without diagnostics.
- **Failure scenario:** Request a native read from `io` or a volatile region through the core service path; the adapter routes it to `ram` and returns ordinary bytes.
- **Root cause:** The native and core memory contracts were made similarly named but not semantically equivalent.
- **Recommended fix:** Add a capability-aware core memory adapter for named spaces/volatile regions, or keep the native interface private and reject unsupported capabilities at service construction.
- **Regression risks:** PE remains `ram`-only; preserve that behavior while testing explicit rejection for other spaces.
- **Relevant validation:** Native frontend provider tests do not exercise `LegacyMemoryProvider` against core `IMemoryProvider`.

### MEDIUM-009: Caller operation cancellation and queue failures are not preserved by async service contracts

- [ ] **Remediation status:** Open.
- **References:** `sleigh/sleigh_service.cppm:49-72`, `decompiler/decompiler_service.cppm:89-101`, `function_id/function_id_service.cppm:65-80`, symbols `decode_batch`, `decompile`, and `identify`; `../core/contracts/operation.cppm:108-114`, symbol `OperationContext`.
- **Affected component:** Worker-pool scheduling, cancellation, and user-visible diagnostics.
- **Technical evidence:** Each method creates a new worker operation and checks that new token, not `OperationContext::operation` or `OperationContext::cancellation`. A failed `WorkerPool::submit` is thrown as `std::runtime_error` despite service methods returning `Task` without a documented throw boundary.
- **Expected behavior:** The caller's cancellation/status controls the task and queue-full/project-closed failures use one documented checked channel.
- **Actual behavior:** Cancelling the caller operation can leave work running, and queue failure escapes as an exception.
- **Impact:** Analysis shutdown and cancellation are nondeterministic; API callers must catch undocumented exceptions.
- **Failure scenario:** Cancel the context after submission or fill the bounded queue; the task uses an unrelated token or throws.
- **Root cause:** Worker submission has no operation-adoption/linking overload and service methods have no common rejection result type.
- **Recommended fix:** Link/adopt the caller `OperationControl` in worker submission and standardize on `Result<Task<...>>` or a documented rejected task.
- **Regression risks:** Runtime shutdown and task ownership must be migrated together.
- **Relevant validation:** No service test covers caller-token cancellation or queue-full rejection.

### MEDIUM-010: Decompiler request options and canonical function ranges are discarded

- [x] **Remediation status:** Fixed. Output flags are honored, all body ranges determine the checked read-ahead bound, and overflow is rejected.
- **References:** `decompiler/decompiler_service.cppm:105-154`, symbols `decompile_now` and `DecompilerService::decompile`; `../core/contracts/decompiler.cppm:21-28`, symbol `DecompileRequest`; `../core/domain/address_range.cppm:19-76`, symbol `AddressRangeSet`.
- **Affected component:** Decompiler request semantics and function-boundary preservation.
- **Technical evidence:** `decompile_now` always fills C source and control-flow output regardless of `include_text`/`include_control_flow`, uses only `request.function.body.ranges().front()`, and extends the inclusive end by an unchecked `+16`. The constructor's `query_` and `sla_path_` are not used as fallback/configuration inputs.
- **Expected behavior:** Request flags control output, all canonical body ranges are preserved or unsupported multi-range input is rejected, and end arithmetic is checked.
- **Actual behavior:** Callers cannot request a minimal artifact, multi-range functions are truncated to one range, and end overflow or read-ahead can cross the canonical function boundary.
- **Impact:** Extra work, incorrect decompilation scope, and misleading output for split/near-maximum-address functions.
- **Failure scenario:** Submit a request with `include_text=false` or a two-range body; the result still contains text and only the first range is passed to the native engine.
- **Root cause:** The adapter was written around a contiguous x86 fixture rather than the full domain request.
- **Recommended fix:** Implement option gating, a checked contiguous-range adapter (or explicit multi-range diagnostic), and a bounded read-ahead policy tied to the memory provider.
- **Regression risks:** Existing CLI output defaults must stay unchanged; add option and split-range tests.
- **Relevant validation:** Current decompiler integration tests cover non-empty default output, not option suppression or range preservation.

### MEDIUM-011: PE adapter silently labels unsupported architectures as x86 and drops structured loader facts

- [x] **Remediation status:** Partially fixed. PE32 width and unknown-machine identities are preserved and structured facts are emitted; exhaustive machine mapping remains open.
- **References:** `pe_loader/pe_loader_service.cppm:121-147`, symbols `PeLoaderService::make_result` and `make_regions`; `../core/contracts/pe_loader.cppm:35-43`, symbol `PeLoadResult`; parser records `pe_loader/src/pe_loader.cppm:307-388`.
- **Affected component:** Loader architecture, symbol, and relocation contract conversion.
- **Technical evidence:** `make_result` chooses x86-64 only for the AMD64 machine and maps every other machine to x86-32 while setting `address_bits=64`. It leaves `imported_symbols`, `exported_symbols`, and `relocations` empty even though the parser has structured records and the core result exposes those vectors.
- **Expected behavior:** Supported PE machine values map to their canonical architecture or return an explicit unsupported diagnostic; all promised structured facts are populated.
- **Actual behavior:** ARM/ARM64/other inputs can be accepted with x86 metadata and imports/exports/relocations are available only in textual details.
- **Impact:** Decoder/decompiler selection and reference/relocation analysis can be wrong while reporting successful load.
- **Failure scenario:** Load an ARM64 PE or an import-bearing PE and inspect `PeLoadResult.architecture`/structured vectors.
- **Root cause:** The adapter was limited to an x86 happy path despite a generic core loader result.
- **Recommended fix:** Map the PE machine enum exhaustively, set 32/64-bit address metadata consistently, translate parser records into canonical `Symbol`/`Relocation`, and reject unknown machines.
- **Regression risks:** Existing x86 fixture IDs and serialized details must remain stable; add one fixture per supported machine.
- **Relevant validation:** Current PE tests assert basic regions/architecture only and do not inspect structured symbol or relocation vectors.

## Findings: Low

### No findings

The translation-engine metadata facade is now a direct alias to `core::ArchitectureDescription` at `translation_engine/native/native_translation.cppm:8-9`; its storage, context, p-code, opcode, and memory provider modules likewise use canonical core values. Native algorithm ownership types remain intentionally private.

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
- [x] `ctest --test-dir build --output-on-failure`: 49/49 passed.
- [x] `git diff HEAD~2..HEAD --check`: passed.
- [ ] No sanitizer, alternate-provider, non-x86, malformed-input, or destruction-race validation was run.

## Unresolved Questions And Residual Risks

- [ ] Decide whether moved legacy analyzer libraries are compatibility diagnostics or production services.
- [ ] Complete translation-engine extraction before claiming one native substrate.
- [ ] Define resource ownership and service leases for all queued operations.

## Follow-Up Decision

- [ ] Remaining open work: `CRITICAL-001`, `HIGH-002`, `HIGH-003`, `HIGH-007`, `MEDIUM-001`, `MEDIUM-003`, `MEDIUM-004`, `MEDIUM-009`, `MEDIUM-012`, and `MEDIUM-013`, plus the explicitly partial items above.
- [x] Fixes were authorized by the current user request and validated with the full build/test workflow.
