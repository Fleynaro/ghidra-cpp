# PE Loader Service

The PE service adapts its checked value parser in [`src/pe_loader.cppm`](src/pe_loader.cppm) to core contracts.

- [`pe_loader_service.cppm`](pe_loader_service.cppm) exposes `PeLoaderService`, `PeImage`, generic memory regions, architecture facts, and serializable PE details.
- `PeLoadResult` now retains canonical imported/exported [`Symbol`](../../core/domain/symbol.cppm) values and [`Relocation`](../../core/domain/relocation.cppm) values in addition to parser-specific detail strings.
- The service does not create functions, mutate symbols, write SQLite, or write `events.log`; project ingestion emits events through the runtime coordinator.
- Parser coverage remains in [`tests`](tests), while service integration tests live under the service target.
