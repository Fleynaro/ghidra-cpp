# Runtime Workers

The worker modules implement the single runtime-owned bounded pool required by [`../../ARCHITECTURE.md`](../../ARCHITECTURE.md).

- [`worker_pool.cppm`](worker_pool.cppm) owns worker threads, deterministic priority/sequence scheduling, queue backpressure, and shutdown.
- [`task.cppm`](task.cppm) and [`cancellation.cppm`](cancellation.cppm) expose the core-owned task/cancellation contracts.
- [`progress.cppm`](progress.cppm) provides a bounded operation-owned progress channel.

Workers execute callables and do not know PE, Sleigh, FID, decompiler, analyzer, event, or projection business semantics. Project mutation remains in the runtime commit lane.
