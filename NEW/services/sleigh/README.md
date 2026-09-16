# Sleigh Service

The Sleigh service adapts [`../../features/sleigh_runtime`](../../features/sleigh_runtime/README.md) into the canonical decoder contract.

- [`sleigh_service.cppm`](sleigh_service.cppm) preserves bytes, masks, operands, flow, context, and p-code while converting storage to `core::Address`/`core::StorageLocation`.
- One project-scoped native decoder is protected by a mutex in the MVP, matching the concurrency decision in [`../../ARCHITECTURE.md`](../../ARCHITECTURE.md).
- Single decode is synchronous; batch decode is queued on [`../../runtime/workers`](../../runtime/workers/README.md).
