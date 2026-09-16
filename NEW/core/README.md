# Core

`core` is the stable vocabulary and contract layer for the autonomous C++23 implementation.

- [`core.cppm`](core.cppm) exports [`domain`](domain/README.md), [`contracts`](contracts/README.md), and [`events`](events/README.md).
- [`domain`](domain/README.md) contains immutable address-aware values, decoded instructions, p-code, functions, symbols, references, types, artifacts, and structured results.
- [`contracts`](contracts/README.md) contains passive providers, active services, operation/task values, commands, persistence, projections, and bus interfaces.
- [`events`](events/README.md) contains persistent event envelopes and typed draft helpers.

Core has no SQLite, project lifecycle, worker pool, native Ghidra engine, analyzer implementation, GUI, or language binding dependency. Original behavior references remain next to the values that replace the corresponding Java/native contracts.
