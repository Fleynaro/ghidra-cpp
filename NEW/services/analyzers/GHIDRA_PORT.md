# Ghidra Port Evidence

[`entry_materialization.cppm`](entry_materialization.cppm) maps the invariant-checking portion of Ghidra `AutoAnalysisManager` and listing analyzers to a read-only core analyzer service. It reports missing materialization as diagnostics and never mutates a project directly.

The existing analyzer algorithms and parity tests remain under [`../../features/analyzers`](../../features/analyzers/README.md). Their migration boundary is the canonical `AnalysisSnapshot`/`MutationCommand` contract, not a second mutable project model.
