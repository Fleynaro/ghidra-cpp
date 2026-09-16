# Ghidra Port Evidence

The service maps the native `Sleigh::oneInstruction`, `instructionLength`, `printAssembly`, `PcodeEmit`, and `AssemblyEmit` behavior from `Ghidra/Features/Decompiler/src/decompile/cpp/sleigh.hh`, `translate.hh`, and `loadimage.hh`. Java operand/mask compatibility follows `SleighInstructionPrototype.java`.

`NEW/core/domain/decoded_instruction.cppm` is the single transient value boundary for these facts. `sleigh_runtime.cppm` and `sleigh_runtime_adapter.cppm` use aliases and populate that domain value; `sleigh_service.cppm` alone promotes it to the space-aware `core::Instruction`. This keeps native runtime ownership private while preserving selector, source-operand, mask, flow, and p-code ordering metadata.

The implementation source remains in [`src`](src) with focused tests under [`tests`](tests). This layer owns the core contract conversion and runtime scheduling boundary; `.slaspec` compilation remains outside the current compiled-SLA scope.
