# Runtime

Runtime owns lifecycle and infrastructure while core owns values/contracts/events.

- [`workers`](workers/README.md) provides the shared bounded worker pool and operation state.
- [`event_store`](event_store/README.md) owns append-only framed history and replay.
- [`storage`](storage/README.md) owns the replaceable SQLite projection store.
- [`projections`](projections/README.md) owns current software/analysis/diagnostic views and the commit coordinator.
- [`event_bus`](event_bus/README.md) publishes committed envelopes after projection application.

Services never write `events.log` or SQLite directly. The future [`project`](project/README.md) and [`dispatcher`](dispatcher/README.md) layers will compose these components into one project commit lane.
