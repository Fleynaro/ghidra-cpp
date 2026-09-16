# Decompiler Service

The decompiler service uses its native frontend in [`src/decompiler.cppm`](src/decompiler.cppm) behind the core `IDecompiler` contract. Decoder providers consume the shared [`../../core/domain/decoded_instruction.cppm`](../../core/domain/decoded_instruction.cppm) values rather than frontend-local storage, p-code, or instruction DTOs.

- [`decompiler_service.cppm`](decompiler_service.cppm) adapts canonical memory/project snapshots to provider-local native objects and returns `core::Decompilation`.
- [`src/decompiler.cppm`](src/decompiler.cppm) aliases `core::StorageLocation`, `core::PcodeOp`, `core::PcodeOpcode`, `core::DecodedInstruction`, and `core::DecodeError`; only mutable native engine descriptions remain frontend-owned.
- [`src/decompiler_impl.cppm`](src/decompiler_impl.cppm) performs the explicit domain-to-native materialization and normalizes only the native LOAD/STORE selector representation.
- Native state is created per task, so concurrent requests do not share mutable `Architecture`/`Funcdata` state.
- The service is scheduled by [`../../runtime/workers`](../../runtime/workers/README.md); no direct `std::async` is introduced.
