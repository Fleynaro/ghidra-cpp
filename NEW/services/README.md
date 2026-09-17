# Services

Services perform feature operations against core contracts and return immutable values or mutation proposals.

- [`translation_engine`](translation_engine/README.md) is the shared native translation boundary.
- [`pe_loader`](pe_loader/README.md) adapts checked PE parsing and immutable image memory.
- [`sleigh`](sleigh/README.md) adapts compiled-SLA decoding and shared-pool batch work.
- [`function_id`](function_id/README.md) and [`decompiler`](decompiler/README.md) contain their native engines, contract adapters, CLIs, fixtures, and tests.
- [`bsim`](bsim/README.md) ports normalized graph signatures and weighted sparse vector similarity without database/server infrastructure.
- [`analyzers`](analyzers/README.md) contains all migrated analyzer implementations/tests and never owns runtime scheduling.
- [`hello`](hello/README.md) preserves the smoke-test service.

No service writes `events.log` or SQLite. Runtime composition owns persistence and commit ordering.
