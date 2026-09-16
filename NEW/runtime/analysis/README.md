# Runtime Analysis

Runtime analysis owns registration, dependency ordering, trigger coalescing, cancellation context, and analyzer execution. Feature algorithms remain service implementations.

- [`analyzer_registry.cppm`](analyzer_registry.cppm) rejects duplicate IDs, validates prerequisites, and topologically orders analyzers by priority.
- [`trigger_coalescer.cppm`](trigger_coalescer.cppm) removes duplicate event triggers while retaining ordered envelopes.
- [`analysis_scheduler.cppm`](analysis_scheduler.cppm) runs only triggered analyzers against revision-stamped immutable snapshots and returns mutation proposals for the project commit lane.

The scheduler does not write projections or event history directly.
