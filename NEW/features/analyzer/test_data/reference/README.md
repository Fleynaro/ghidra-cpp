# Analyzer Name
Reference

## Purpose
Exercise instruction and data operand reference creation from loaded bytes and existing code units.

## Original Ghidra Implementation
`Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/analysis/OperandReferenceAnalyzer.java` and `DataOperandReferenceAnalyzer.java`.

## Test Scenario
The script disassembles first, snapshots the resulting references, enables only `Reference`, and compares the final reference-manager rows with that baseline. Every row is labeled as either pre-existing disassembler output or genuinely added by the analyzer.

## Why This C++ Code Was Chosen
Real strings, values, pointer-bearing data, and direct calls provide multiple reference classes without hand-written expected addresses.

## Required Compiler Options
MSVC x64 and the deterministic linker configuration in `build.bat`.

## Required Auxiliary Files
The PE loader, disassembler, and `TEST/run_ghidra_python.bat` are required; no manual symbol database is used.

## Ghidra Analysis Options
Enable only `Reference` after the script's explicit disassembly prerequisite.

## Expected Results
References that satisfy memory, symbol, relocation, offcut, and existing-reference checks should appear in the report. A reference present in the disassembly snapshot is not misreported as analyzer-created merely because it is listed after analysis.

## Generated Markdown
`test_reference.md` is generated from Ghidra's reference manager by `run_ghidra.py`.

## Notes / Limitations
Instruction encodings, relocations, and addresses vary by compiler/linker version. The existing sections below retain reproduction details.

This MSVC x64 PE fixture exercises the ordinary `Reference` analyzer. It contains
real pointer-bearing data, a string, and calls whose operand and data references are
discovered from the loaded bytes. The report is generated from Ghidra's reference
manager and does not contain hand-written expected addresses.

With the current MSVC/Ghidra combination, all four eligible rows are already present as
`DEFAULT` disassembler references, so the target analyzer adds zero new rows. The report
preserves that result and the before/after evidence; it must not be read as proof of a
positive `ANALYSIS` reference until a compiler variant produces an unresolved operand case.

## Navigation

- [`test_reference.cpp`](test_reference.cpp) defines the referenced data and code.
- [`build.bat`](build.bat) builds a deterministic CRT-free MSVC x64 executable.
- [`run_ghidra.py`](run_ghidra.py) reopens the executable with `project.openProgram(...)`, snapshots the deliberate disassembly prerequisite, configures `Reference`, and extracts before/after reference rows with provenance labels.
- [`test_reference.exe`](test_reference.exe) and [`test_reference.md`](test_reference.md) are generated artifacts when the local toolchain and Ghidra environment are available.
- [`../../README.md`](../../README.md) describes the fixture collection boundary.

## Java contract

The implementation is
`Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/analysis/OperandReferenceAnalyzer.java`.
It examines existing instructions, data, relocations, and operand objects, rejecting
references outside memory or inside offcut functions and preserving stronger existing
references. This fixture therefore explicitly disassembles executable bytes first,
then enables only `Reference`; the script reports the actual memory/operand references
before and after analysis, preserving the distinction between `DEFAULT` disassembler
references and `ANALYSIS` references created by the analyzer while leaving
compiler-dependent instruction addresses unasserted.

## Reproduction

Run `build.bat`, then
`TEST\\run_ghidra_python.bat NEW\\features\\analyzer\\test_data\\reference\\run_ghidra.py`
from the repository root.
