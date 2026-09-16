# Analyzer Services

Analyzer services implement business algorithms behind [`../../core/contracts/analyzer.cppm`](../../core/contracts/analyzer.cppm). Runtime scheduling, trigger coalescing, dependency ordering, and commit serialization live under [`../../runtime/analysis`](../../runtime/analysis/README.md).

- [`entry_materialization.cppm`](entry_materialization.cppm) verifies the real loader/Sleigh materialization used by the end-to-end project path.
- Existing feature analyzers under [`../../features/analyzers`](../../features/analyzers/README.md) remain regression sources while their mutable `AnalysisContext` state is extracted into canonical snapshots and mutation proposals.
