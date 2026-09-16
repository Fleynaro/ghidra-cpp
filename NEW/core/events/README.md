# Core Events

The event modules define persistent state-change payload construction and the append-only envelope schema.

- [`event.cppm`](event.cppm) owns drafts, committed envelopes, batches, deterministic fields, and checksums.
- [`project_events.cppm`](project_events.cppm), [`memory_events.cppm`](memory_events.cppm), [`code_events.cppm`](code_events.cppm), [`function_events.cppm`](function_events.cppm), [`symbol_events.cppm`](symbol_events.cppm), [`type_events.cppm`](type_events.cppm), and [`analysis_events.cppm`](analysis_events.cppm) create typed state-event drafts.
- [`events.cppm`](events.cppm) is the aggregate import used by runtime persistence and projection code.

Events contain enough value data to rebuild projections without invoking services. The binary physical framing is implemented in [`../../runtime/event_store`](../../runtime/event_store).
