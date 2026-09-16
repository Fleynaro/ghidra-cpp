# Core Contracts Boundary Audit Report

## Review Metadata

- [x] **Scope:** current `HEAD` (`9cd8a17501`) in `NEW/core/contracts` and every matching implementation/consumer found under `NEW/services`.
- [x] **Date:** 2026-09-16.
- [x] **Reviewer:** Kilo, independent read-only audit.
- [x] **Assumption:** native decompiler and Sleigh engine records may remain private implementation details, but every service-facing conversion must be explicit, loss-aware, and revision-safe.

## Review Status

- [x] Scope confirmation.
- [x] Contract declarations and aggregate exports inspected.
- [x] Service implementors, adapters, consumers, CMake exposure, and focused tests inspected.
- [x] Findings sorted by severity and remediation status recorded.
- [x] No source, build, or configuration implementation was changed.

## Findings: Critical

### No findings

No critical defect was confirmed in a contract declaration itself. The queued-task lifetime defect is in the service/runtime ownership boundary and remains tracked in [`../../services/REVIEW_REPORT.md`](../../services/REVIEW_REPORT.md#critical-001).

## Findings: High

### CONTRACT-HIGH-001: The decoder contract returns a persistent listing entity at a transient decode boundary

- [x] **Remediation status:** Fixed. `IPCodeDecoder` and `DecodeBatchResult` now use `core::DecodedInstruction`; promotion is centralized in `core::materialize_decoded_instruction()`.
- **Reference:** [`pcode_decoder.cppm:12-40`](pcode_decoder.cppm#L12-L40), symbols `DecodeRequest`, `DecodeBatchResult`, and `IPCodeDecoder`; canonical transient value [`../domain/decoded_instruction.cppm:49-75`](../domain/decoded_instruction.cppm#L49-L75), symbol `DecodedInstruction`; consumer/promotion [`../../services/sleigh/sleigh_service.cppm:86-132`](../../services/sleigh/sleigh_service.cppm#L86-L132), symbol `SleighService::convert`.
- **Affected component:** Decoder command/service boundary and every decoder consumer.
- **Technical evidence:** `IPCodeDecoder::decode` returns `Result<core::Instruction>`, which requires an `InstructionKey` and entity identity. `DecodedInstruction` already exists for numeric, low-level decoder output. `SleighService::convert` fabricates an entity ID from only `address.offset`, and `DecodeInstruction::materialize` in [`../contracts/command.cppm:34-38`](../contracts/command.cppm#L34-L38) cannot prevent that fabrication.
- **Expected behavior:** A bounded decode is a side-effect-free transient result. Promotion to a project listing entity must be a separate operation with project identity, address-space identity, provenance, and collision policy.
- **Actual behavior:** A decoder silently performs domain materialization and can produce the same entity ID for equal offsets in different spaces.
- **Impact:** Transient requests bypass canonical entity ownership, multi-space projects can collide, and callers cannot distinguish decode from persistence materialization.
- **Reproduction or failure scenario:** Decode offset `0x100` in two address spaces through `SleighService::convert`; both IDs are `instruction-256`. Submit `DecodeInstruction{materialize=false}`; the returned type is still a persistent `Instruction`.
- **Root cause:** The contract selected the richer listing snapshot instead of the existing decoder snapshot.
- **Recommended fix:** Change decoder results to `DecodedInstruction` (and batch results to `vector<DecodedInstruction>`), then add one explicit, space-aware materializer at the runtime/project boundary. Preserve a compatibility facade only while consumers migrate.
- **Regression risks:** Existing listing event helpers and tests currently expect `Instruction`; add conversion tests for entity identity, operands, flow, and p-code before changing the public result type.
- **Relevant tests or validation:** Existing Sleigh aliases assert `sleigh_runtime::Instruction == core::DecodedInstruction`, but no contract test asserts that `IPCodeDecoder` returns the same transient type or that IDs include address-space identity.

### CONTRACT-HIGH-002: `ProviderContext` cannot carry the architecture or snapshot identity required by decompilation

- [x] **Remediation status:** Partially fixed. `ProviderContext` now carries canonical `ArchitectureDescription`; revision/resource identity and SLA selection remain open.
- **Reference:** [`decompiler.cppm:14-28`](decompiler.cppm#L14-L28), symbols `ProviderContext` and `DecompileRequest`; unused architecture contract [`architecture_provider.cppm:11-24`](architecture_provider.cppm#L11-L24), symbol `IArchitectureProvider`; service consumer [`../../services/decompiler/decompiler_service.cppm:105-154`](../../services/decompiler/decompiler_service.cppm#L105-L154), symbol `DecompilerService::decompile_now`.
- **Affected component:** Decompiler architecture, memory, p-code, and revision-consistency boundary.
- **Technical evidence:** `ProviderContext` contains only p-code, memory, and project query providers. It has no `IArchitectureProvider`, `Revision`, or resource-set identity. `IArchitectureProvider` has no implementor under `NEW/services`; `DecompilerService` instead constructs x86-64 metadata locally. The request's `read_revision` is not validated against the query/provider snapshots.
- **Expected behavior:** One decompilation request must bind an immutable architecture, code/data spaces, registers, processor context, memory, decoder, query, and revision/resource identity.
- **Actual behavior:** Architecture is absent from the request, the architecture contract is orphaned, and independent providers can be mixed without a compatibility check.
- **Impact:** A request for ARM, 32-bit x86, a non-`ram` code space, or a changed projection can return a plausible but semantically wrong result.
- **Reproduction or failure scenario:** Supply a non-x86 query and decoder in a `DecompileRequest`; the service still selects x86-64 registers and `__cdecl`. Supply a query at revision N and a memory provider from revision M; the type system accepts both.
- **Root cause:** Provider grouping was designed before architecture/resource identity became canonical domain state.
- **Recommended fix:** Add a required `shared_ptr<const IArchitectureProvider>` and immutable snapshot/resource identity to the core context, validate all provider revisions before execution, and map core architecture metadata to the native frontend only at its adapter boundary.
- **Regression risks:** Existing aggregate initializers and CLI fixtures need a compatibility builder; do not alias the native architecture DTO because it lacks core language/compiler identity and several address-space invariants.
- **Relevant tests or validation:** No contract test rejects a missing architecture provider or mismatched query revision; current architecture tests exercise native frontend DTOs independently.

### CONTRACT-HIGH-003: Core analyzer contracts are not the contracts implemented by the production analyzer set

- [ ] **Remediation status:** Open.
- **Reference:** [`analyzer.cppm:26-82`](analyzer.cppm#L26-L82), symbols `AnalyzerDescriptor`, `AnalysisSnapshot`, `MutationCommand`, and `IAnalyzer`; actual service base [`../../services/analyzers/shared/src/analyzer_base.cppm:18-37`](../../services/analyzers/shared/src/analyzer_base.cppm#L18-L37), symbol `ghidra::analyzer::Analyzer`; duplicate value model [`../../services/analyzers/shared/src/analyzer_types.cppm:35-168,403-408`](../../services/analyzers/shared/src/analyzer_types.cppm#L35-L168), symbols `AnalysisEvent`, `Function`, and `AnalyzerDescriptor`.
- **Affected component:** Analyzer registration, scheduling, canonical entity mutation, and all analyzers linked by [`../../services/analyzers/CMakeLists.txt:25-66`](../../services/analyzers/CMakeLists.txt#L25-L66).
- **Technical evidence:** Only `EntryMaterializationAnalyzer` under `NEW/services` derives `core::contracts::IAnalyzer`. The feature analyzers derive the legacy mutable base, receive `AnalysisContext&`, return `void`, and mutate local `Function`, `Reference`, and `DataObject` records directly. Their descriptor uses `set<EventKind>` and only name/priority/prerequisites, unlike the core stable ID, scope, run policy, execution class, and mutation metadata.
- **Expected behavior:** Production analyzers consume a revision-stamped immutable snapshot and return typed mutation proposals that the project commit lane can validate and append.
- **Actual behavior:** The core contract registry can schedule only the one new analyzer; the migrated feature set bypasses it and can never satisfy its canonical entity/revision boundary.
- **Impact:** Service substitution, deterministic replay, optimistic concurrency, removals, and source-priority event ordering are not guaranteed for the actual analyzer set.
- **Reproduction or failure scenario:** Register a legacy `FunctionIdAnalyzer` with the core `runtime::analysis::AnalyzerRegistry`; it is not an `IAnalyzer`. Run the legacy manager; it mutates `AnalysisContext` without producing `MutationCommand` values.
- **Root cause:** Directory migration and native algorithm reuse preceded extraction of a contract-facing adapter.
- **Recommended fix:** Introduce a `LegacyAnalyzerAdapter` that snapshots canonical entities, runs the legacy algorithm in an explicitly isolated working model, translates every mutation to typed/event drafts, and registers only adapter instances in the core registry. Migrate feature by feature; do not alias mutable working DTOs to persistent domain entities.
- **Regression risks:** Analyzer priority/prerequisite behavior and Ghidra mutation semantics must be covered before removing the legacy path.
- **Relevant tests or validation:** Existing analyzer tests exercise `AutoAnalysisManager` and local fixtures; they do not prove core registry registration, snapshot isolation, or event-proposal replay.

### CONTRACT-HIGH-004: Function ID database and matcher contracts are bypassed and semantically narrower than their documentation

- [x] **Remediation status:** Partially fixed. Matching uses `IFunctionIdDatabase::query` and malformed hashes return `Result` diagnostics; relation/options propagation remains open.
- **Reference:** [`function_id.cppm:9-20`](function_id.cppm#L9-L20), symbols `IFunctionIdMatcher`; [`function_id_database.cppm:9-20`](function_id_database.cppm#L9-L20), symbol `IFunctionIdDatabase`; service [`../../services/function_id/function_id_service.cppm:14-44,56-131`](../../services/function_id/function_id_service.cppm#L14-L44), symbols `FunctionIdDatabaseService` and `FunctionIdService`.
- **Affected component:** Function ID matching, relation scoring, database substitution, and error handling.
- **Technical evidence:** `FunctionIdService::identify_now` calls the concrete `database_->database().identify` at line 116 instead of `IFunctionIdDatabase::query`. The contract describes a relation-aware `FunctionHashFamily`, but `FunctionIdDatabaseService::query` uses only `full_hash`, returns zero-score candidates, and `identify_now` leaves parent/child hashes empty. `FunctionIdOptions` fields `force_relations`, language/database filters, and bookmark threshold are not applied.
- **Expected behavior:** The matcher depends only on the database contract, passes all declared hash relations/options, and returns checked canonical score/evidence values.
- **Actual behavior:** Replacing the contract database has no effect on matcher execution, and relation-aware/options-aware callers receive a reduced full-hash path.
- **Impact:** Alternate databases cannot be used, relation ranking is silently disabled, and malformed hash text can escape the declared `Result` boundary through `std::stoull`.
- **Reproduction or failure scenario:** Inject a mock `IFunctionIdDatabase` that returns a candidate; `FunctionIdService` never calls it. Pass `full_hash = "not-hex"` to `FunctionIdDatabaseService::query`; `std::invalid_argument` escapes.
- **Root cause:** The service adapter retained direct access to the native database while exposing a newer canonical contract.
- **Recommended fix:** Make the matcher depend on `shared_ptr<const IFunctionIdDatabase>`, implement checked `from_chars` parsing, carry child/parent families and filters through the adapter, and add explicit unsupported diagnostics for relation modes the packed format cannot represent.
- **Regression risks:** Native score ordering and database-language filtering must remain byte-for-byte compatible; add mock-contract and malformed-input tests first.
- **Relevant tests or validation:** Valid packed database tests do not verify contract substitution, relation hashes, option filters, or malformed canonical input.

## Findings: Medium

### CONTRACT-MEDIUM-001: Core memory has no representation for native volatile access or the full named-space adapter contract

- [x] **Remediation status:** Partially fixed. `IMemoryProvider::volatile_ranges()` is now canonical and decompiler adapters forward it; native named-space reads remain offset-oriented.
- **Reference:** [`memory_provider.cppm:11-25`](memory_provider.cppm#L11-L25), symbol `IMemoryProvider`; native duplicate [`../../services/decompiler/src/decompiler.cppm:86-117`](../../services/decompiler/src/decompiler.cppm#L86-L117), symbols `MemoryRangeDescription` and `MemoryProvider`; adapter [`../../services/decompiler/decompiler_service.cppm:13-31`](../../services/decompiler/decompiler_service.cppm#L13-L31), symbol `LegacyMemoryProvider`.
- **Affected component:** Multi-space memory reads and side-effect-sensitive decompiler analysis.
- **Technical evidence:** Core reads are `Address`/`Bytes` and expose only generic `MemoryRegion` metadata. The native frontend contract additionally exposes `read(space, address, size)` and `volatile_ranges()`. `LegacyMemoryProvider` implements only offset-only `read`, hardcodes `ram`, and never forwards volatile ranges.
- **Expected behavior:** The adapter preserves address-space selection and volatile semantics, or rejects unsupported native requests explicitly.
- **Actual behavior:** Native LOAD/STORE requests can silently read the wrong space, and volatile ranges are treated as ordinary memory.
- **Impact:** IO/overlay architectures and dead-volatile/read-side-effect decisions can be wrong without an error.
- **Reproduction or failure scenario:** Use a native frontend operation targeting `io` or a volatile range through `DecompilerService`; the adapter calls `IMemoryProvider::read(ram, ...)` and returns no volatile metadata.
- **Root cause:** Two similarly named memory interfaces evolved independently.
- **Recommended fix:** Extend the core memory capability with explicit space/volatile metadata or keep the native interface adapter-private and require a capability-aware adapter object; add rejection tests for unsupported spaces.
- **Regression risks:** Existing PE images intentionally expose only `ram`; preserve that behavior while making non-`ram` rejection explicit.
- **Relevant tests or validation:** Native provider tests cover the local interface directly, not `LegacyMemoryProvider` against `IMemoryProvider`.

### CONTRACT-MEDIUM-002: `MutationCommand` duplicates `EventDraft` without event identity/concurrency fields

- [ ] **Remediation status:** Open.
- **Reference:** [`analyzer.cppm:53-68`](analyzer.cppm#L53-L68), symbols `MutationCommand` and `AnalyzerResult`; canonical event value [`../events/event.cppm:22-34`](../events/event.cppm#L22-L34), symbol `EventDraft`.
- **Affected component:** Analyzer-to-commit-lane mutation transport.
- **Technical evidence:** Both structures carry event/aggregate/source/payload strings, but `MutationCommand` omits schema version, project, correlation, causation, and idempotency key. The runtime scheduler stores commands as strings in a separate vector.
- **Expected behavior:** A mutation proposal should either be a typed domain command or a complete event draft with explicit concurrency/provenance semantics.
- **Actual behavior:** A commit adapter must reconstruct omitted event metadata and can accidentally produce non-idempotent or uncorrelated events.
- **Impact:** Replay, deduplication, audit provenance, and optimistic commit behavior can diverge between analyzer and command paths.
- **Reproduction or failure scenario:** Return two identical `MutationCommand` values from an analyzer; no contract field supplies a stable idempotency key or causal identity.
- **Root cause:** The analyzer proposal DTO duplicated the event envelope shape instead of reusing the canonical event draft boundary.
- **Recommended fix:** Replace it with a typed mutation variant or an `events::EventDraft`-based proposal builder that requires project/correlation/idempotency metadata at construction.
- **Regression risks:** Existing scheduler report consumers must migrate from string fields to typed drafts.
- **Relevant tests or validation:** No proposal replay test verifies idempotency or event metadata preservation.

### CONTRACT-MEDIUM-003: Analyzer scope metadata has no target value

- [ ] **Remediation status:** Open.
- **Reference:** [`analyzer.cppm:19-50`](analyzer.cppm#L19-L50), symbols `AnalysisScope` and `AnalysisSnapshot`.
- **Affected component:** Entity- and range-scoped analyzer scheduling.
- **Technical evidence:** `AnalysisScope` advertises `entity` and `range`, but `AnalysisSnapshot` contains only project, revision, scope, resources, and providers. There is no `EntityId`, `AddressRangeSet`, or address-space target.
- **Expected behavior:** The snapshot carries the immutable target selected by the scheduler, with invariants matching the scope enum.
- **Actual behavior:** An entity/range analyzer must infer its target from the entire event batch or query, defeating the declared scope boundary.
- **Impact:** Unbounded work and accidental mutations outside the requested analysis target.
- **Reproduction or failure scenario:** Construct `AnalysisSnapshot{scope=AnalysisScope::entity}` with no target; the contract accepts an impossible-to-enforce scope.
- **Root cause:** Scope classification was added without a target model.
- **Recommended fix:** Add optional canonical entity/range target fields and validate them at registration/request construction.
- **Regression risks:** Project-scoped callers need unchanged defaults; range targets must preserve address-space identity.
- **Relevant tests or validation:** No scope-specific snapshot validation test exists.

### CONTRACT-MEDIUM-004: Async contract context and service task ownership are not connected

- [ ] **Remediation status:** Open.
- **Reference:** [`operation.cppm:108-114`](operation.cppm#L108-L114), symbol `OperationContext`; service methods [`../../services/sleigh/sleigh_service.cppm:49-72`](../../services/sleigh/sleigh_service.cppm#L49-L72), [`../../services/decompiler/decompiler_service.cppm:89-101`](../../services/decompiler/decompiler_service.cppm#L89-L101), and [`../../services/function_id/function_id_service.cppm:65-80`](../../services/function_id/function_id_service.cppm#L65-L80).
- **Affected component:** Cancellation, queue-full diagnostics, and operation status propagation.
- **Technical evidence:** Each service submits a new worker operation and checks only that new token. The caller's `OperationContext::cancellation` and `operation` are not linked. Queue submission errors are converted to `std::runtime_error` even though the contract otherwise returns checked `Result` values.
- **Expected behavior:** Caller cancellation/status controls the submitted operation, and queue rejection is represented by the declared error channel.
- **Actual behavior:** Cancelling the caller operation may not stop the service task; queue-full/project-closed conditions escape as exceptions.
- **Impact:** Scheduler cancellation and user-visible diagnostics are unreliable.
- **Reproduction or failure scenario:** Cancel the `OperationControl` passed to `decode_batch` after submission; the worker token remains independent. Fill the worker queue; `decode_batch` throws instead of returning a structured rejection.
- **Root cause:** The `Task` return shape and worker submission API do not model adoption of an existing operation control.
- **Recommended fix:** Add a worker submission overload that adopts/links `OperationControl`, and use a checked `Result<Task<...>>` (or a documented non-throwing rejected task) consistently across async contracts.
- **Regression risks:** Existing `Task` callers and runtime shutdown ordering must be migrated together.
- **Relevant tests or validation:** No service test covers caller-token cancellation, queue-full rejection, or status identity.

## Findings: Low

### No findings

## Verified Strengths

- [x] Contracts use value types and avoid native Ghidra pointers.
- [x] Core domain already contains canonical `Address`, `StorageLocation`, `PcodeOp`, `DecodedInstruction`, `Instruction`, `FunctionSnapshot`, `ArchitectureDescription`, and `Decompilation` values that can support explicit adapters.
- [x] The public aggregate [`contracts.cppm`](contracts.cppm) exports the focused contract modules without importing service implementations.
- [x] Existing Sleigh/decompiler aliases reuse canonical storage, opcode, p-code, and transient instruction values instead of defining a second copy of those particular records.

## Reviewed Areas With No Findings

- [x] Event-store, projection-store, and event-bus declarations were checked for service-facing type leakage; their concrete implementors are under `NEW/runtime`, outside the requested service implementation scope.
- [x] PE loader and resource-manager contract declarations do not expose native parser ownership; their adapter field-loss findings are recorded in [`../../services/REVIEW_REPORT.md`](../../services/REVIEW_REPORT.md#medium-005).
- [x] `IProjectQuery` uses canonical entity return types; the remaining risk is snapshot completeness/revision pinning in its implementations and consumers, not a duplicate declaration.

## Validation Results

- [x] Current contract sources, service sources, CMake module exposure, and matching tests were inspected read-only.
- [x] Symbol/reference searches covered all contract class names and DTO names under `NEW/services`.
- [x] Existing module-local review reports were read for prior findings and non-regression context.
- [ ] No build, test, sanitizer, ABI, or runtime race execution was performed for this audit.
- [x] No implementation or configuration files were changed.

## Unresolved Questions And Residual Risks

- [ ] Decide whether native decompiler metadata providers remain a private engine boundary or become canonical domain contracts.
- [ ] Define the address-space and volatile-memory capability model before widening `IMemoryProvider`.
- [ ] Define a migration order for the legacy analyzer working model so event/replay parity is not lost.

## Follow-Up Decision

- [ ] Highest-priority remediation candidates are `CONTRACT-HIGH-001`, `CONTRACT-HIGH-002`, `CONTRACT-HIGH-003`, and `CONTRACT-HIGH-004`.
- [x] Fixes were authorized by the current user request; full build/test validation completed.
