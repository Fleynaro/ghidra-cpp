# Ghidra Port Evidence

[`entry_materialization.cppm`](entry_materialization.cppm) maps the invariant-checking portion of Ghidra `AutoAnalysisManager` and listing analyzers to a read-only core analyzer service. It reports missing materialization as diagnostics and never mutates a project directly.

All analyzer algorithms and parity tests now live in the sibling directories under this service. Their runtime migration boundary is the canonical `AnalysisSnapshot`/`MutationCommand` contract, not a second mutable project model.
