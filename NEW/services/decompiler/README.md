# Decompiler Service

The decompiler service uses the native frontend in [`../../features/decompiler/src/decompiler.cppm`](../../features/decompiler/src/decompiler.cppm) behind the core `IDecompiler` contract.

- [`decompiler_service.cppm`](decompiler_service.cppm) adapts canonical memory/project snapshots to provider-local native objects and returns `core::Decompilation`.
- Native state is created per task, so concurrent requests do not share mutable `Architecture`/`Funcdata` state.
- The service is scheduled by [`../../runtime/workers`](../../runtime/workers/README.md); no direct `std::async` is introduced.
