# Ghidra Port Evidence

The service maps the native `Sleigh::oneInstruction`, `instructionLength`, `printAssembly`, `PcodeEmit`, and `AssemblyEmit` behavior from `Ghidra/Features/Decompiler/src/decompile/cpp/sleigh.hh`, `translate.hh`, and `loadimage.hh`. Java operand/mask compatibility follows `SleighInstructionPrototype.java`.

The implementation source remains [`../../features/sleigh_runtime`](../../features/sleigh_runtime/README.md), which is retained with its focused tests. This layer only owns the core contract conversion and runtime scheduling boundary; `.slaspec` compilation remains outside the current compiled-SLA scope.
