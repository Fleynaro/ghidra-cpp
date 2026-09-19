# Custom P-Code Emulator Research

This project extracts real x86-64 P-Code from `src/corpus.exe` through PyGhidra, executes each `PcodeOp` in an independent Python engine, and records a state transition after every operation. It does not call Ghidra's executor.

## Quick Start

From the repository root:

```text
FUNC_RECOGNIZER_RESEARCH\build.bat
TEST\run_ghidra_python.bat FUNC_RECOGNIZER_RESEARCH\scripts\run_pipeline.py
```

The build uses MSVC `/Od`, `/Ob0`, `/Zi`, `/GS-`, and exported `noinline` functions. The Python wrapper must be used for every PyGhidra script, as required by the repository's `AGENTS.md`. The ten reports are written to [`output/`](output/).

## Architecture

- [`src/pcode_emulator.py`](src/pcode_emulator.py) owns `Varnode`, `PcodeOp`, sparse byte-addressed `MachineState`, fixed-width integer semantics, LOAD/STORE, and concrete control flow. `execute()` is exactly one P-Code operation; `execute_program()` follows branches until `RETURN`.
- [`src/trace.py`](src/trace.py) is an observer-only Markdown recorder. It receives before/after snapshots, memory events, and branch decisions without being imported by the opcode dispatch layer.
- [`scripts/run_pipeline.py`](scripts/run_pipeline.py) is the PyGhidra adapter. It discovers exported functions, translates Java `PcodeOp`/`Varnode` objects, seeds Windows x64 argument registers (`RCX`, `RDX`, `R8`, `R9`) and sparse memory, then invokes the custom core.
- [`src/corpus.cpp`](src/corpus.cpp) contains exactly ten deterministic exported research functions plus a normal C++ `main` used for validation.
- [`tests/test_emulator.py`](tests/test_emulator.py) contains hand-written P-Code unit tests. [`tests/test_integration.py`](tests/test_integration.py) checks corpus and pipeline artifacts without requiring PyGhidra for every unit run.

## Implemented Operations

The dispatch covers `COPY`, `LOAD`, `STORE`, `INT_EQUAL`, `INT_NOTEQUAL`, `INT_SLESS`, `INT_SLESSEQUAL`, `INT_LESS`, `INT_LESSEQUAL`, `INT_ZEXT`, `INT_SEXT`, `INT_ADD`, `INT_SUB`, `INT_CARRY`, `INT_SCARRY`, `INT_SBORROW`, `INT_2COMP`, `INT_NEGATE`, `INT_XOR`, `INT_AND`, `INT_OR`, `INT_LEFT`, `INT_RIGHT`, `INT_SRIGHT`, `INT_MULT`, `INT_DIV`, `INT_SDIV`, `INT_REM`, `INT_SREM`, `BOOL_NEGATE`, `BOOL_XOR`, `BOOL_AND`, `BOOL_OR`, `PIECE`, `SUBPIECE`, `POPCOUNT`, `LZCOUNT`, `BRANCH`, `CBRANCH`, `CALL`, `RETURN`, and registered `CALLOTHER` handlers. Unsupported operations raise `PcodeExecutionError` with function, step, PC, mnemonic, operands, and control context.

Integer results are masked at the destination width. Memory is sparse and byte-addressed, with configurable endian order. Registers and unique temporaries retain overlapping varnode widths. Constant-space branches use Ghidra's relative P-Code-frame convention; concrete nonconstant targets use extracted address mappings.

## Validation

The normal C++ executable prints the same deterministic cases used by the Python harness. The pipeline compares that output and the custom emulator's final `RAX` or mutated array memory. Each report contains expected, normal C++, emulated, and PASS/FAIL values. Reports are generated only after real P-Code extraction from the executable.

## Original Ghidra References

The semantic mapping was studied in [`Ghidra/Framework/Emulation/src/main/java/ghidra/pcode/exec/PcodeExecutor.java`](../Ghidra/Framework/Emulation/src/main/java/ghidra/pcode/exec/PcodeExecutor.java), [`PcodeExecutorState.java`](../Ghidra/Framework/Emulation/src/main/java/ghidra/pcode/exec/PcodeExecutorState.java), [`BytesPcodeExecutorState.java`](../Ghidra/Framework/Emulation/src/main/java/ghidra/pcode/exec/BytesPcodeExecutorState.java), [`PcodeEmulator.java`](../Ghidra/Framework/Emulation/src/main/java/ghidra/pcode/emu/PcodeEmulator.java), and the integer behavior classes under [`opbehavior/`](../Ghidra/Framework/Emulation/src/main/java/ghidra/pcode/opbehavior/). [`GHIDRA_PORT.md`](GHIDRA_PORT.md) records the important compatibility decisions and intentional differences.

## Limitations

This first corpus is concrete execution, not path exploration. External calls, indirect branches, floating-point operations, and unregistered userops are intentionally unsupported or require explicit extension handlers. The initial-state harness models the Windows x64 integer calling convention and sparse memory; it does not recover types from debug metadata. A report's array label is an input hint, not claimed Ghidra type recovery.
