# Runtime Event Bus

[`event_bus.cppm`](event_bus.cppm) publishes only committed envelopes after projection application. It is not a command transport, persistence layer, or projection writer. Subscribers can observe analyzer/runtime hints and public notifications while durable resumption uses the event store.
