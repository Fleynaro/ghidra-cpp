# Runtime Projections

Projection code materializes current query state from committed event envelopes.

- [`software_model_projection.cppm`](software_model_projection.cppm) provides immutable-copy queries for memory, instructions, functions, symbols, and references.
- [`analysis_projection.cppm`](analysis_projection.cppm) tracks analysis-run status.
- [`diagnostics_projection.cppm`](diagnostics_projection.cppm) keeps bounded diagnostics.
- [`projection_coordinator.cppm`](projection_coordinator.cppm) is the sole append/apply/checkpoint/publication commit path.

No projection invokes an analyzer or regenerates missing events. SQLite persistence is supplied by [`../storage`](../storage/README.md), while history is owned by [`../event_store`](../event_store/README.md).
