# Debugger Domain Review

## Review Metadata

- **Date:** 2026-09-16
- **Reviewer:** Kilo, independent review pass
- **Scope:** [`debugger.cppm`](debugger.cppm) and its fit with the generic debugger contract.
- **Reviewed state:** Current working-tree implementation; no source or test implementation changes were made.
- **Assumptions:** Domain values must remain backend-neutral and support non-Windows, remote, multi-architecture targets.

## Review Status

### Post-review implementation update

Register values now carry explicit byte order and refuse numeric interpretation when it is unknown; memory reads carry requested/transferred sizes and completeness. Process/thread ownership and richer region/exit metadata remain open.

- [x] Scope confirmed for debugger value/domain types.
- [x] All identity, state, register, memory, stack, module, breakpoint, watchpoint, exception, event, and request values inspected.
- [x] Generic-contract consumers and the DbgEng mapping were compared.
- [x] No implementation changes made during review.

## critical

No findings.

## high

### HIGH-DOMAIN-001: Register value conversion hard-codes little-endian interpretation

- **Status:** [x] Remediated for numeric interpretation; non-little-endian values now return no numeric helper result instead of being reversed
- **Source:** [`debugger.cppm:129-145`](debugger.cppm#L129-L145), especially `RegisterValue::unsigned_value()`.
- **Component:** Generic register value domain.
- **Technical evidence:** `unsigned_value()` shifts byte `index` by `index * 8`, explicitly treating every register as little-endian. No endianness field exists in the domain.
- **Expected behavior:** A cross-platform debugger value must either carry target endianness/architecture or avoid interpreting opaque bytes as an integer in the generic domain.
- **Actual behavior:** A big-endian backend can return correct raw bytes but the provided integer helper returns a reversed value.
- **Impact:** Register comparisons and consumers can make incorrect decisions on big-endian targets; this violates the stated cross-platform contract.
- **Reproduction/failure scenario:** Return bytes `{0x01, 0x02}` for a big-endian 16-bit register. The helper returns `0x0201` instead of `0x0102`.
- **Root cause:** The helper encoded the current x64/Windows assumption directly into a reusable domain type.
- **Recommended fix:** Add target byte order to architecture/session metadata and require it for numeric conversion, or remove the numeric helper from the backend-neutral value type.
- **Regression risks:** Existing x64 callers should preserve little-endian behavior through explicit target metadata.
- **Relevant tests/validation:** No test covers endianness or non-x64 register values.

## medium

### MEDIUM-DOMAIN-001: Thread and module values cannot express process ownership

- **Status:** [ ] Remediation required
- **Source:** [`debugger.cppm:104-114`](debugger.cppm#L104-L114) and [`debugger.cppm:148-158`](debugger.cppm#L148-L158).
- **Component:** `Thread` and `Module` snapshots.
- **Technical evidence:** `Thread` has only `ThreadId`, name, state, current flag, and optional IP. `Module` has name/path/base/size/symbol flag. Neither carries `ProcessId`.
- **Expected behavior:** In a session capable of multiple processes, every thread and module snapshot should identify its owning process or be returned from a request whose process scope is immutable and explicit.
- **Actual behavior:** The values cannot be safely correlated with the process list if a backend exposes more than one process.
- **Impact:** Consumers can associate a thread/module with the wrong process and remote/multi-process backends cannot provide complete snapshots.
- **Reproduction/failure scenario:** Enumerate two inferiors with identical module names and thread IDs scoped per inferior; the returned values have no process association.
- **Root cause:** Domain values assume the session's implicit current process.
- **Recommended fix:** Add process ownership to `Thread`/`Module` or enforce a typed process-scoped snapshot wrapper in the contract.
- **Regression risks:** Existing single-process initialization can default the process field to the current process.
- **Relevant tests/validation:** Current tests only use one local process and do not compare ownership metadata.

### MEDIUM-DOMAIN-002: Memory-region model loses important target mapping state

- **Status:** [ ] Remediation recommended
- **Source:** [`debugger.cppm:160-173`](debugger.cppm#L160-L173).
- **Component:** `MemoryRegion`.
- **Technical evidence:** The value contains booleans for readable/writable/executable and a free-form name, but no committed/free/reserved state, guard/copy-on-write flags, allocation base, source/module association, or partial-query status.
- **Expected behavior:** A generic memory map should preserve enough semantic state for a debugger UI or remote consumer to distinguish mapped, reserved, inaccessible, guarded, and file-backed regions.
- **Actual behavior:** Backends must collapse those states into booleans or strings.
- **Impact:** Memory inspection and breakpoint placement decisions lose information, especially on Windows virtual memory and remote sparse maps.
- **Reproduction/failure scenario:** Query a guarded or reserved region and observe that the model cannot represent the distinction from an ordinary inaccessible region.
- **Root cause:** The model was reduced to permissions and a label.
- **Recommended fix:** Add a generic region state/protection enum and optional allocation/module metadata without exposing OS flags.
- **Regression risks:** Existing boolean consumers can derive their values from the richer state.
- **Relevant tests/validation:** No test calls `memory_regions()`.

### MEDIUM-DOMAIN-003: Snapshot exit and symbol metadata are too weak for diagnosis

- **Status:** [ ] Remediation recommended
- **Source:** [`debugger.cppm:91-101`](debugger.cppm#L91-L101), [`debugger.cppm:148-158`](debugger.cppm#L148-L158), and [`debugger.cppm:216-222`](debugger.cppm#L216-L222).
- **Component:** `Process`, `Module`, and `DebugException` values.
- **Technical evidence:** `Process.exit_code` is only an optional signed integer; there is no termination/signal description. `Module.symbols_loaded` is one boolean with no symbol state/error. `DebugException` has one numeric code and description but no exception parameters or access type.
- **Expected behavior:** A backend-neutral snapshot should preserve common diagnostic distinctions while allowing backend-specific detail to remain opaque.
- **Actual behavior:** Signals, termination reasons, unresolved-symbol states, exception parameters, and memory access faults cannot be represented.
- **Impact:** Exit/exception reporting is lossy for POSIX and remote backends and weak for Windows first/second chance analysis.
- **Reproduction/failure scenario:** A SIGSEGV with fault address and access type or a process terminated by signal 9 must be flattened into a signed integer/description.
- **Root cause:** The domain model only captured the fields used by the initial DbgEng fixture.
- **Recommended fix:** Add generic optional exit/exception detail structures with capability-safe fields and preserve unknown values explicitly.
- **Regression risks:** Consumers should handle absent optional detail.
- **Relevant tests/validation:** The exception test checks only numeric code and first-chance; no exit detail test exists.

## low

### LOW-DOMAIN-001: `EventPayload` permits kind/payload mismatches

- **Status:** [ ] Remediation recommended
- **Source:** [`debugger.cppm:248-333`](debugger.cppm#L248-L333).
- **Component:** `DebugEvent` representation.
- **Technical evidence:** `DebugEvent` stores an independent `EventKind` and `std::variant` payload, including `std::monostate`, with no constructor invariant or validator ensuring the kind matches the variant alternative.
- **Expected behavior:** A translated event should be impossible or difficult to construct with contradictory kind and payload.
- **Actual behavior:** Any code can create `EventKind::exception` with a `ModuleLoadedEvent`, and consumers must defensively inspect both fields.
- **Impact:** Low-level event consumers can misinterpret notifications.
- **Reproduction/failure scenario:** Construct the mismatched aggregate directly; it compiles and is accepted by the type system.
- **Root cause:** The event kind was duplicated beside a flexible variant for convenience.
- **Recommended fix:** Derive kind from the variant or provide validated event constructors.
- **Regression risks:** Existing aggregate initialization may need migration.
- **Relevant tests/validation:** No event payload invariant test exists.

## Verified Strengths

- [x] Native handles, HRESULTs, Windows structs, and DbgEng constants are absent from the domain module.
- [x] Process and thread identities are opaque strings rather than direct OS/DbgEng index types.
- [x] Address values retain named address spaces through the existing core `Address` type.
- [x] Watchpoint access includes execute/read/write/read-write semantics, with backend limitations documented separately.

## Validation Results

- [x] Domain source and all contract consumers were inspected.
- [x] Existing focused debugger tests/build evidence was reviewed.
- [ ] No non-Windows, big-endian, or multi-process backend was available for runtime validation.

## Unresolved Questions and Residual Risks

- [ ] Decide whether register architecture metadata belongs in the debugger domain or an existing architecture domain.
- [ ] Decide whether heap/allocation values should be a first-class domain concept or an optional memory-provider capability.

## Final Follow-up Decision

- [ ] Domain changes should be authorized before claiming full cross-platform register/memory compatibility.
