# Decompiler Service

The decompiler service uses its native frontend in [`src/decompiler.cppm`](src/decompiler.cppm) behind the core `IDecompiler` contract. Decoder providers consume the shared [`../../core/domain/decoded_instruction.cppm`](../../core/domain/decoded_instruction.cppm) values rather than frontend-local storage, p-code, or instruction DTOs.

- [`decompiler_service.cppm`](decompiler_service.cppm) adapts canonical memory/project snapshots to provider-local native objects and returns `core::Decompilation`.
- `core::contracts::ProviderContext` carries the revision's canonical `ArchitectureDescription`; the service maps it into the native frontend and uses the function entry address space for memory/decode requests. The x86 fallback is retained only for legacy callers that omit architecture metadata.
- [`src/decompiler.cppm`](src/decompiler.cppm) aliases `core::StorageLocation`, `core::PcodeOp`, `core::PcodeOpcode`, `core::DecodedInstruction`, and `core::DecodeError`; only mutable native engine descriptions remain frontend-owned.
- [`src/decompiler_impl.cppm`](src/decompiler_impl.cppm) performs the explicit domain-to-native materialization and normalizes only the native LOAD/STORE selector representation.
- `IMemoryProvider::volatile_ranges()` is forwarded by [`decompiler_service.cppm`](decompiler_service.cppm), so native LoadImage does not silently lose side-effect-sensitive memory metadata.
- Native state is created per task, so concurrent requests do not share mutable `Architecture`/`Funcdata` state. The native call is guarded by a process-wide mutex because the migrated engine still initializes process-global XML/attribute tables; tasks remain asynchronous and are serialized only at that unsafe native boundary.
- The service is scheduled by [`../../runtime/workers`](../../runtime/workers/README.md); no direct `std::async` is introduced.
