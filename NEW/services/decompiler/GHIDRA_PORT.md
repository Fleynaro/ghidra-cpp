# Ghidra Port Evidence

The adapter preserves the native `Architecture`, raw flow, SSA, action, and C-printing sequence from `Ghidra/Features/Decompiler/src/decompile/cpp/architecture.cc`, `funcdata.cc`, `flow.cc`, `action.cc`, and `printc.cc`. Provider-local behavior is supplied through the service-owned [`src/decompiler.cppm`](src/decompiler.cppm) interfaces.

The provider boundary uses the same `core::DecodedInstruction`, `core::StorageLocation`, and `core::PcodeOp` values as the Sleigh runtime. The frontend aliases those types, validates them, and materializes native `VarnodeData`/`PcodeOp` records only inside [`src/decompiler_impl.cppm`](src/decompiler_impl.cppm). The shared contract is verified by [`tests/decompiler_tests.cppm`](tests/decompiler_tests.cppm) and [`tests/provider_contract_tests.cppm`](tests/provider_contract_tests.cppm).

The service contract now carries the canonical `ArchitectureDescription` through [`../../core/contracts/decompiler.cppm`](../../core/contracts/decompiler.cppm). [`decompiler_service.cppm`](decompiler_service.cppm) maps supplied address spaces/registers and uses the function entry space for memory and decoder requests; the legacy x86 description is only a compatibility fallback when callers provide no architecture snapshot.

The runtime-facing result is a canonical value and is revision-stamped. It does not expose `Funcdata`, `Architecture`, native varnodes, or C++ pointers to the project facade.
