# Analyzer Services

Analyzer services implement business algorithms behind [`../../core/contracts/analyzer.cppm`](../../core/contracts/analyzer.cppm). Runtime scheduling, trigger coalescing, dependency ordering, and commit serialization live under [`../../runtime/analysis`](../../runtime/analysis/README.md).

- [`entry_materialization.cppm`](entry_materialization.cppm) verifies the real loader/Sleigh materialization used by the end-to-end project path.
- The migrated analyzer directories under this directory retain their original algorithms, fixtures, and tests; runtime scheduling is supplied through canonical snapshots and mutation proposals.
