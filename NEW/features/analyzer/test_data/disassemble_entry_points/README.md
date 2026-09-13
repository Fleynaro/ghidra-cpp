# Analyzer Name
Disassemble Entry Points

## Purpose
Exercise symbol-driven disassembly at newly added executable entry points while rejecting a data symbol.

## Original Ghidra Implementation
`Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/disassembler/EntryPointAnalyzer.java`, using `CreateFunctionCmd.java`.

## Test Scenario
Two exported code symbols and one C++-linkage exported data marker are imported without pre-disassembly; the script resolves both undecorated and decorated names and compares instruction and function presence before and after analysis.

## Why This C++ Code Was Chosen
PE export symbols are the analyzer's genuine input and no-inline functions retain deterministic code without script-created symbols or instructions.

## Required Compiler Options
MSVC x64 and the linker as invoked by `build.bat`; `/OPT:NOREF` retains the exported entry points.

## Required Auxiliary Files
The PE loader, base disassembler, and `TEST/run_ghidra_python.bat` are required.

## Ghidra Analysis Options
Enable only `Disassemble Entry Points` with its execute-set behavior configured by `run_ghidra.py`.

## Expected Results
Code export locations may gain instructions/functions, while the C++-linkage data marker must remain non-instructional and functionless in both snapshots.

## Generated Markdown
`test_disassemble_entry_points.md` is generated from before/after selected-symbol snapshots and an explicit target delta by `run_ghidra.py`.

## Notes / Limitations
PE import behavior can seed an initial entry function and affect before/after state. The existing sections below retain reproduction details.

## Navigation

- [`test_disassemble_entry_points.cpp`](test_disassemble_entry_points.cpp) defines two exported code entry points and one exported data marker.
- [`build.bat`](build.bat) builds the deterministic CRT-free MSVC x64 PE.
- [`run_ghidra.py`](run_ghidra.py) deliberately avoids pre-disassembly, resolves the decorated C++ data symbol, enables Disassemble Entry Points, and compares selected symbol state.
- `test_disassemble_entry_points.exe` and `test_disassemble_entry_points.md` are generated artifacts when the required tools exist.
- [`../README.md`](../README.md) is the analyzer fixture collection guide.

## Fixture Design

PE exports provide genuine code and data symbols. The two noinline functions are retained and called by the entry function; the C++-linkage data marker is a negative case. Unlike the other fixtures, the script does not disassemble the executable section before analysis because instruction presence is the behavior under test.

## Ghidra Java Contract

The exact implementation is [`Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/disassembler/EntryPointAnalyzer.java`](../../../../../Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/disassembler/EntryPointAnalyzer.java). It intersects newly added memory with the execute set when configured to respect it, consumes code/external-entry symbols, and performs immediate or delayed disassembly. The script reports only instruction/function presence at the selected symbols, with before/after evidence for the data negative.

## Dependencies

The build requires MSVC x64. The analysis requires the repository PyGhidra wrapper, PE loader, and base disassembler. No other analysis boolean is enabled and no script-created symbols or instructions are used.

## Limitations

PE import behavior may create an initial entry function, and future loader changes may alter the before/after state. The report records the observed state rather than claiming a discovery when no code symbol is exposed. Decorated C++ data symbols are matched by their demangled name or MSVC prefix, and data symbols are not treated as code merely because their bytes are executable-looking.

## Reproduction

Run `build.bat`, then from the repository root run `TEST\run_ghidra_python.bat NEW\features\analyzer\test_data\disassemble_entry_points\run_ghidra.py`. Optional executable/report arguments are accepted.

## Generated Artifacts

The expected generated files are `test_disassemble_entry_points.exe` and `test_disassemble_entry_points.md`. They are created only by MSVC and PyGhidra, respectively.

## Validation

Validation completed with MSVC 2022 x64 compilation/linking, project save/reopen through `project.openProgram(...)`, before/after target-only analysis, decorated-symbol lookup, and report inspection. The generated report contains the two exported code symbols and the C++-linkage data marker; the data marker is not treated as an entry point.
