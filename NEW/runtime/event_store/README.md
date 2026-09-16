# Event Store

The event-store modules implement the authoritative project history described by [`../../ARCHITECTURE.md`](../../ARCHITECTURE.md).

- [`event_codec.cppm`](event_codec.cppm) encodes explicit metadata and payload fields rather than object layouts.
- [`append_only_log.cppm`](append_only_log.cppm) writes framed records to `events.log`, assigns global sequence/event IDs, detects checksum failures, and truncates only an incomplete final frame.
- [`replay.cppm`](replay.cppm) rebuilds a projection from committed events without invoking services or analyzers.

The store is one logical writer per project. Projection application and event-bus publication are coordinated by the project session, not hidden inside this persistence layer.
