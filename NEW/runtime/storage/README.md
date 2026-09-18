# Runtime Storage

Storage is an implementation detail behind [`../../core/contracts/projection.cppm`](../../core/contracts/projection.cppm).

- [`sqlite_connection.cppm`](sqlite_connection.cppm) owns RAII SQLite connections and error conversion.
- [`sqlite_projection_store.cppm`](sqlite_projection_store.cppm) persists applied-event identities, event payloads, checkpoints, and materialized `memory_regions`, `symbols`, `functions`, `instructions`, and `analysis_runs` rows in `projection.sqlite`.

The event log remains authoritative. SQLite is a replaceable materialized view and may be rebuilt by [`../event_store/replay.cppm`](../event_store/replay.cppm).
