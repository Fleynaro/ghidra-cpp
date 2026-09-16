# Debugger Contract Review

## Review Metadata

- **Date:** 2026-09-16
- **Reviewer:** Kilo, independent review pass
- **Scope:** [`debugger.cppm`](debugger.cppm), its imported debugger domain values, and the contract-level requirements for local, remote, and cross-platform debugger implementations.
- **Reviewed state:** Current working-tree implementation; no source or test implementation changes were made during this review.
- **Assumptions:** The contract is intended to be reusable by DbgEng, x64dbg, GDB, LLDB, and remote targets, and its generic behavior must not depend on one active local Windows process.

## Review Status

### Post-review implementation update

Process selection/current-process identity, structured memory-read completeness, explicit timeout diagnostics, and target byte-order metadata were added after the review. The original multi-process, architecture/register-write, heap-model, breakpoint-capability, and event-delivery limitations remain open unless explicitly marked partial below.

- [x] Scope confirmed against the requested generic debugger contract.
- [x] Source inspected, including all public `IDebugSession` and `IDebugger` operations.
- [x] Related domain values inspected in [`../domain/debugger.cppm`](../domain/debugger.cppm).
- [x] Backend usage compared with [`../../services/debugger/win_dbg_eng/win_dbg_eng.cppm`](../../services/debugger/win_dbg_eng/win_dbg_eng.cppm).
- [x] Contract integration tests inspected in [`../../services/debugger/win_dbg_eng/tests/debugger_contract_tests.cppm`](../../services/debugger/win_dbg_eng/tests/debugger_contract_tests.cppm).
- [x] No implementation changes made as part of the review.

## critical

No findings.

## high

### HIGH-CONTRACT-001: Session operations cannot address more than the implicit current process

- **Status:** [ ] Partially remediated; explicit process selection/current identity now exists, but most query methods still operate on the selected implicit context rather than carrying process scope in request values
- **Source:** [`debugger.cppm:63-111`](debugger.cppm#L63-L111), especially `process()`, `processes()`, `threads()`, `read_memory()`, `stack_trace()`, and `modules()`.
- **Component:** `IDebugSession` process/thread/memory/stack/module contract.
- **Technical evidence:** The contract can enumerate processes, but every other query operates on an implicit current process. There is no `select_process(ProcessId)`, no process argument on thread/module/memory/stack/register operations, and no process association in the `Thread` or `Module` contract methods.
- **Expected behavior:** A backend-independent session should represent a target connection that may expose multiple processes, remote process servers, or a debugger-specific process context. Operations must identify the process or provide an explicit, documented process-selection operation.
- **Actual behavior:** The only way to choose a process is backend-specific implicit state. A GDB multi-process target, a remote session with multiple inferiors, or a DbgEng client with multiple processes cannot be used through this interface without silently changing hidden backend state.
- **Impact:** Important state can be read from the wrong process; remote and multi-process implementations cannot satisfy the contract faithfully; process identity values exist but cannot be used to scope most operations.
- **Reproduction/failure scenario:** Attach to two processes in one target connection, call `threads()` or `read_memory()` for the non-current process, and observe that the contract provides no operation that can request the non-current process.
- **Root cause:** The interface was shaped around one active local process and added `processes()` as an enumeration rather than modeling a process context as a first-class scope.
- **Recommended fix:** Add explicit process scope to all target queries/operations or add a generic `select_process(ProcessId)` with a documented current-context invariant. Prefer request types carrying `ProcessId` for remote-safe semantics and preserve the selected context in `SessionInfo`.
- **Regression risks:** Existing single-process callers should retain default-current behavior, while multi-process implementations must not be forced to emulate a local process.
- **Relevant tests/validation:** The current tests only call `threads()` and `modules()` for the fixture's implicit process; there is no multi-process or process-selection test.

### HIGH-CONTRACT-002: Register/context contract is incomplete for architecture-independent debugging

- **Status:** [ ] Partially remediated; byte order is now explicit and x64 flags aliasing is covered, but register roles, known masks, architecture metadata, and writes remain
- **Source:** [`debugger.cppm:78-92`](debugger.cppm#L78-L92); related values [`../domain/debugger.cppm:116-145`](../domain/debugger.cppm#L116-L145).
- **Component:** Register enumeration, register values, and thread context API.
- **Technical evidence:** `Register` has only name, bit width, and five boolean roles. `RegisterValue` has only raw bytes and a helper that interprets them as little-endian. There is no target architecture, endianness, register identifier, parent/subregister relation, known-bit mask, frame/context selection, or register-write operation.
- **Expected behavior:** A generic debugger contract should describe general-purpose, flags, vector/SIMD/FPU, system, and subregister values without assuming x64 or little-endian layout. It should also make context selection and register writes explicit where supported.
- **Actual behavior:** A backend can return names, but consumers cannot reliably distinguish `RFLAGS`, vector registers, aliases, unavailable bits, or a non-little-endian target. The contract supports reads only, despite debugger context commonly requiring writes.
- **Impact:** x64 flags/SIMD validation is weak, ARM/MIPS/remote targets cannot expose correct value interpretation, and callers cannot implement context editing or compare register states reliably.
- **Reproduction/failure scenario:** Implement a big-endian remote backend or return an `RFLAGS`/`ZMM0` value; `RegisterValue::unsigned_value()` either applies the wrong byte order or cannot communicate register role/known bits.
- **Root cause:** Register values were reduced to opaque byte vectors without a separate architecture/register-description model.
- **Recommended fix:** Add architecture/endianness metadata, stable register descriptors with semantic roles and aliases, known masks or availability, optional frame/context scope, and a generic register-write operation with explicit backend capability errors.
- **Regression risks:** Existing name-based x64 callers need compatibility aliases while new architecture metadata is introduced.
- **Relevant tests/validation:** The current test checks only that `read_registers()` is non-empty and reads `rip`/`rsp`; it does not verify `RAX-R15`, `RBP`, `RFLAGS`, SIMD, endianness, or writes.

### HIGH-CONTRACT-003: Memory results cannot represent partial reads, failures, or heap semantics

- **Status:** [ ] Partially remediated; reads now preserve requested/transferred sizes and complete status, but no generic heap/allocation model exists
- **Source:** [`debugger.cppm:94-101`](debugger.cppm#L94-L101); related [`../domain/debugger.cppm:160-173`](../domain/debugger.cppm#L160-L173).
- **Component:** Memory access and memory-region contract.
- **Technical evidence:** `read_memory()` returns only `Result<Bytes>`, and `MemoryRegion` contains only start, size, three booleans, and a name. There is no transferred-byte count, known-byte mask, partial-read status, protection/state detail, allocation identity, or heap/allocation abstraction.
- **Expected behavior:** Local and remote debuggers must be able to report partial/unmapped reads without presenting truncated bytes as a complete success. Heap-backed memory should be inspectable through a documented pointer/allocation model or at least through a result that preserves the target address and transfer status.
- **Actual behavior:** The contract has no way to distinguish a complete read from a backend that returned fewer bytes, and it has no heap concept at all. A caller can read an address if it already knows it, but cannot ask for heap allocations, allocation ownership, or a diagnostic for partial data.
- **Impact:** Memory inspection can silently produce incomplete data; heap debugging and remote sparse-memory targets cannot be represented faithfully.
- **Reproduction/failure scenario:** Read across a valid page into an unmapped page or inspect a remote target with a short packet response. The returned `Bytes` has no contract-level indication of which bytes are valid.
- **Root cause:** The contract models memory as a byte vector rather than a transfer result with completeness and target mapping metadata.
- **Recommended fix:** Introduce a memory-read result containing requested address, returned bytes, known mask/transfer count, and diagnostic status. Add a generic memory-map/allocation query only if the backend supports it, with capability/error semantics for unavailable heap metadata.
- **Regression risks:** Existing callers should continue to consume complete reads while being forced to handle partial results explicitly.
- **Relevant tests/validation:** The current test reads and writes one exported global of exactly eight bytes; it does not test an unmapped boundary, partial transfer, memory regions, or heap-backed storage.

## medium

### MEDIUM-CONTRACT-001: Breakpoint/watchpoint semantics are too narrow and partly misleading

- **Status:** [ ] Remediation required
- **Source:** [`debugger.cppm:113-138`](debugger.cppm#L113-L138); related [`../domain/debugger.cppm:189-214`](../domain/debugger.cppm#L189-L214).
- **Component:** Breakpoint and watchpoint API.
- **Technical evidence:** Breakpoints accept only one address, kind, and one-shot flag. There is no scope/process, condition, symbolic request, resolved-location list, list/query operation, hit address, or hit access. `BreakpointKind::hardware` is a generic semantic value even though the current backend maps it to a code breakpoint mechanism.
- **Expected behavior:** The generic contract should express semantic capabilities without promising backend equivalence. Hardware/data breakpoint support, conditions, unresolved symbols, and multiple resolved locations should be represented explicitly.
- **Actual behavior:** A backend must either discard useful breakpoint semantics or hide them in implementation state. Consumers cannot inspect installed breakpoints or determine exactly what address/access caused a stop.
- **Impact:** Conditional/symbolic breakpoints and remote multi-location breakpoints cannot be modeled; callers may believe a hardware request was honored when it was not.
- **Reproduction/failure scenario:** Request a hardware breakpoint on a backend that only supports software breakpoints, then inspect the returned `Breakpoint.kind`; the contract reports the requested hardware kind without a resolved-capability result.
- **Root cause:** The model was optimized for one DbgEng breakpoint object rather than a resolved semantic breakpoint set.
- **Recommended fix:** Add request/result types with scope, condition, requested/resolved locations, capability or unsupported status, and explicit hit metadata. Avoid reporting unsupported hardware semantics as installed hardware behavior.
- **Regression risks:** Existing simple address breakpoints should remain a convenience overload over richer request types.
- **Relevant tests/validation:** Tests install one address breakpoint but do not inspect current address, resolved location, enable/disable state, removal, conditions, or hardware semantics.

### MEDIUM-CONTRACT-002: Event delivery and event coverage are underspecified

- **Status:** [ ] Remediation required
- **Source:** [`debugger.cppm:134-149`](debugger.cppm#L134-L149); related [`../domain/debugger.cppm:248-333`](../domain/debugger.cppm#L248-L333).
- **Component:** `DebugEvent` queue and `DebugEventSink` callback.
- **Technical evidence:** The contract exposes a polling queue and a `std::function` sink but does not specify callback thread, ordering relative to polling, backpressure, reentrancy, or whether the sink may block. Event kinds omit session state changes, execution-started, detach/failure, diagnostics, and thread/process state changes.
- **Expected behavior:** Event delivery must have a precise threading/lifetime contract, and lifecycle/state transitions should be observable without inferring them from a stop event.
- **Actual behavior:** A backend can invoke the sink on any thread consistent with its implementation, while the interface only says “lightweight.” Important state transitions are not first-class events.
- **Impact:** Consumers can deadlock or race by calling session APIs from the sink; failures and detach events can be lost; remote backends cannot report useful lifecycle changes uniformly.
- **Reproduction/failure scenario:** Install a sink that calls `poll_events()` or waits on another session operation. The contract gives no rule that makes this safe.
- **Root cause:** Polling and callback delivery were added as convenience mechanisms without a formal event-dispatch policy.
- **Recommended fix:** Specify delivery thread/reentrancy and bounded queue behavior, or expose only polling/subscription tokens. Add state-change/failure/diagnostic events and include session/process/thread scope and sequence invariants in the event domain.
- **Regression risks:** Existing event consumers need a clear migration from implicit callback-thread assumptions.
- **Relevant tests/validation:** No test installs an event sink, checks ordering, checks thread identity, or verifies process/thread/module/exception events individually.

### MEDIUM-CONTRACT-003: Launch parameters are biased toward local filesystem process creation

- **Status:** [ ] Remediation required
- **Source:** [`../domain/debugger.cppm:335-347`](../domain/debugger.cppm#L335-L347), consumed by [`debugger.cppm:34-40`](debugger.cppm#L34-L40).
- **Component:** Launch/attach request model.
- **Technical evidence:** `LaunchRequest` requires `std::filesystem::path program` and a map of local environment variables. There is no target locator/URI, transport/server endpoint, stop-on-entry policy, or explicit remote launch mode.
- **Expected behavior:** A generic debugger should support a local executable, a remote target locator, a remote process-server, or an already configured target without changing the contract.
- **Actual behavior:** Remote implementations must invent a path encoding or reject otherwise valid target descriptions; the contract implies local process creation even though the session is intended to be cross-platform and remote-capable.
- **Impact:** The contract is not truly implementation-independent for launch workflows and cannot express remote connection parameters.
- **Reproduction/failure scenario:** Implement an LLDB remote backend that launches through a target URI rather than a local path. There is no typed request field for the locator or transport options.
- **Root cause:** Launch modeling started from the DbgEng `CreateProcess` path and retained filesystem-specific fields in the domain layer.
- **Recommended fix:** Use a backend-neutral target locator/request with optional local-process fields and capability-specific options kept outside the core contract or represented as opaque backend-neutral transport data.
- **Regression risks:** Preserve a local-path convenience constructor while making the underlying request remote-capable.
- **Relevant tests/validation:** The integration fixture supplies only a local `.exe` path and never exercises attach or remote launch.

## low

### LOW-CONTRACT-001: `SessionId` is numeric while other target identities are opaque

- **Status:** [ ] Remediation recommended
- **Source:** [`../domain/debugger.cppm:17-36`](../domain/debugger.cppm#L17-L36).
- **Component:** Session identity value.
- **Technical evidence:** `ProcessId` and `ThreadId` are opaque strings, but `SessionId` is a `uint64_t` with an implied numeric identity.
- **Expected behavior:** All externally observable identities should be opaque unless numeric semantics are part of the contract.
- **Actual behavior:** Remote/session implementations may need to manufacture numeric IDs or maintain a second identity mapping.
- **Impact:** Minor interoperability friction and inconsistent identity semantics.
- **Reproduction/failure scenario:** A remote debugger whose session locator is a URI must allocate an unrelated numeric ID for the public session identity.
- **Root cause:** Session IDs were generated by a local atomic counter and promoted directly into the domain type.
- **Recommended fix:** Represent session identity as an opaque strong value while retaining a backend-local numeric counter internally.
- **Regression risks:** Existing equality/hash users need a migration from numeric access.
- **Relevant tests/validation:** No test exercises session identity beyond object creation.

## Verified Strengths

- [x] No native DbgEng/Windows types appear in [`debugger.cppm`](debugger.cppm).
- [x] Execution operations that wait for a stop use the established `Task<Result<T>>` abstraction.
- [x] Process and thread IDs do not expose OS handles or DbgEng indexes.
- [x] The contract separates immediate inspection from asynchronous execution at the interface level.

## Validation Results

- [x] Source inspection completed for the contract, domain values, backend implementation, binding, and integration tests.
- [x] Existing focused debugger build/test evidence was reviewed; the suite reported passing tests on the available Windows environment.
- [ ] No remote backend or multi-process implementation was available for interoperability validation.
- [ ] No static contract test currently proves that a non-Windows/non-DgbEng implementation can satisfy all operations.

## Unresolved Questions and Residual Risks

- [ ] Decide whether heap inspection belongs in the generic contract or whether generic memory plus backend capability descriptors is sufficient.
- [ ] Decide whether execution requests should be scoped by process/thread/frame and whether register writes are required for the first stable contract.
- [ ] Define the event sink threading/reentrancy contract before external consumers depend on it.

## Final Follow-up Decision

- [ ] Contract changes should be authorized before treating the abstraction as complete for remote, multi-process, or architecture-independent debugger backends.
