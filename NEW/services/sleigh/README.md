# Sleigh Service

The Sleigh service owns the compiled-SLA runtime under [`src`](src) and adapts it into the canonical decoder contract. Decoder-facing values are defined in [`../../core/domain/decoded_instruction.cppm`](../../core/domain/decoded_instruction.cppm); this service does not own parallel `Instruction`, `Varnode`, or `PcodeOp` DTOs.

- [`sleigh_service.cppm`](sleigh_service.cppm) preserves bytes, masks, operands, flow, context, and p-code while converting storage to `core::Address`/`core::StorageLocation`.
- [`sleigh_runtime.cppm`](sleigh_runtime.cppm) re-exports aliases for `core::DecodedInstruction`, `core::StorageLocation`, `core::PcodeOp`, `core::PcodeOpcode`, and `core::ProcessorContext`; native `ghidra::*` parser state remains private to [`sleigh_runtime_adapter.cppm`](sleigh_runtime_adapter.cppm).
- The promotion boundary is `core::DecodedInstruction` to `core::Instruction`; tests assert the aliases and preserve p-code metadata in [`tests/sleigh_runtime_tests.cppm`](tests/sleigh_runtime_tests.cppm).
- One project-scoped native decoder is protected by a mutex in the MVP, matching the concurrency decision in [`../../ARCHITECTURE.md`](../../ARCHITECTURE.md).
- Single decode is synchronous; batch decode is queued on [`../../runtime/workers`](../../runtime/workers/README.md).
