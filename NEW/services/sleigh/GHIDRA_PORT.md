# Ghidra Port Evidence

The service maps the native `Sleigh::oneInstruction`, `instructionLength`, `printAssembly`, `PcodeEmit`, and `AssemblyEmit` behavior from `Ghidra/Features/Decompiler/src/decompile/cpp/sleigh.hh`, `translate.hh`, and `loadimage.hh`. Java operand/mask compatibility follows `SleighInstructionPrototype.java`.

The implementation source remains in [`src`](src) with focused tests under [`tests`](tests). This layer owns the core contract conversion and runtime scheduling boundary; `.slaspec` compilation remains outside the current compiled-SLA scope.
