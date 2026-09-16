# Services

Services perform feature operations against core contracts and return immutable values or mutation proposals.

- [`translation_engine`](translation_engine/README.md) is the shared native translation boundary.
- [`pe_loader`](pe_loader/README.md) adapts checked PE parsing and immutable image memory.
- [`sleigh`](sleigh/README.md) adapts compiled-SLA decoding and shared-pool batch work.
- [`function_id`](function_id/README.md) and [`decompiler`](decompiler/README.md) are the next service adapters; existing feature implementations remain regression sources until their contract adapters are wired.
- [`analyzers`](analyzers/README.md) contains analyzer service implementations and never owns runtime scheduling.

No service writes `events.log` or SQLite. Runtime composition owns persistence and commit ordering.
