# PE Loader Service

The PE service adapts the checked value parser in [`../../features/pe_loader/src/pe_loader.cppm`](../../features/pe_loader/src/pe_loader.cppm) to core contracts.

- [`pe_loader_service.cppm`](pe_loader_service.cppm) exposes `PeLoaderService`, `PeImage`, generic memory regions, architecture facts, and serializable PE details.
- The service does not create functions, mutate symbols, write SQLite, or write `events.log`; project ingestion emits events through the runtime coordinator.
- Existing parser coverage remains in [`../../features/pe_loader/tests`](../../features/pe_loader/tests), while service integration tests live under the service target.
