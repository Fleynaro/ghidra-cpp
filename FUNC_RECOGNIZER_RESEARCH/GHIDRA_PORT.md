# Ghidra Semantic Port Notes

## Scope

This Python module is a semantic port of the portions needed for concrete integer x86-64 research traces. It is not a wrapper around Ghidra's `PcodeExecutor` and does not invoke the Java emulator at runtime.

## Studied Sources

- [`../Ghidra/Framework/Emulation/src/main/java/ghidra/pcode/exec/PcodeExecutor.java`](../Ghidra/Framework/Emulation/src/main/java/ghidra/pcode/exec/PcodeExecutor.java) dispatches ordinary operations and defines `LOAD`/`STORE` address-space operands.
- [`../Ghidra/Framework/Emulation/src/main/java/ghidra/pcode/exec/PcodeExecutorState.java`](../Ghidra/Framework/Emulation/src/main/java/ghidra/pcode/exec/PcodeExecutorState.java), [`BytesPcodeExecutorState.java`](../Ghidra/Framework/Emulation/src/main/java/ghidra/pcode/exec/BytesPcodeExecutorState.java), and [`BytesPcodeExecutorStatePiece.java`](../Ghidra/Framework/Emulation/src/main/java/ghidra/pcode/exec/BytesPcodeExecutorStatePiece.java) establish byte-oriented spaces, width quantization, sparse memory, and separate unique/register state concerns.
- [`../Ghidra/Framework/Emulation/src/main/java/ghidra/pcode/emu/PcodeEmulator.java`](../Ghidra/Framework/Emulation/src/main/java/ghidra/pcode/emu/PcodeEmulator.java), [`PcodeThread.java`](../Ghidra/Framework/Emulation/src/main/java/ghidra/pcode/emu/PcodeThread.java), and [`DefaultPcodeThread.java`](../Ghidra/Framework/Emulation/src/main/java/ghidra/pcode/emu/DefaultPcodeThread.java) establish instruction sequencing and control-flow boundaries.
- Integer behavior classes such as [`OpBehaviorIntAdd.java`](../Ghidra/Framework/Emulation/src/main/java/ghidra/pcode/opbehavior/OpBehaviorIntAdd.java), [`OpBehaviorIntSright.java`](../Ghidra/Framework/Emulation/src/main/java/ghidra/pcode/opbehavior/OpBehaviorIntSright.java), [`OpBehaviorIntScarry.java`](../Ghidra/Framework/Emulation/src/main/java/ghidra/pcode/opbehavior/OpBehaviorIntScarry.java), [`OpBehaviorIntSborrow.java`](../Ghidra/Framework/Emulation/src/main/java/ghidra/pcode/opbehavior/OpBehaviorIntSborrow.java), [`OpBehaviorPiece.java`](../Ghidra/Framework/Emulation/src/main/java/ghidra/pcode/opbehavior/OpBehaviorPiece.java), and [`OpBehaviorSubpiece.java`](../Ghidra/Framework/Emulation/src/main/java/ghidra/pcode/opbehavior/OpBehaviorSubpiece.java) were used to verify fixed-width semantics.

## Mappings

- Java `Varnode` becomes the immutable Python `Varnode` descriptor in [`src/pcode_emulator.py`](src/pcode_emulator.py), preserving space, offset, byte size, and optional register/userop names.
- Java byte state becomes sparse byte-addressed `MachineState`; reads fill unmapped bytes with zero for deterministic fixture execution, while memory remains segmented by address-space name.
- Java arithmetic dispatch becomes explicit `_execute_op()` dispatch. Every result is masked by the output varnode size, signed comparisons/extensions use the input width, and signed division truncates toward zero.
- Ghidra constant-space branch offsets are treated as relative to the current flattened operation index. Nonconstant branch targets are mapped by extracted instruction address. This preserves the distinction between internal P-Code-frame branches and external machine-address control flow without copying `PcodeFrame`.
- `RETURN` halts the selected concrete function trace. `CALL` supports mapped local targets and records a return index, but external calls are not silently emulated.
- Ghidra Java observer callbacks do not exist in this exact form; Python `PcodeObserver` is an intentional extension boundary used by [`src/trace.py`](src/trace.py).

## Status

Implemented for the listed integer, Boolean, memory, piece, `POPCOUNT`/`LZCOUNT`, branch, call, return, and registered userop operations. Floating point, indirect control flow, external calls, and architecture-specific userops remain pending. The ten-function executable corpus and unit tests are in [`src/corpus.cpp`](src/corpus.cpp) and [`tests/`](tests/).
