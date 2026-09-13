# Subroutine References Fixture

This is a small MSVC x64 PE fixture for the future C++23 implementation of Ghidra's
`Subroutine References` analyzer. The checked-in [`test_subroutine_references.exe`](test_subroutine_references.exe)
and [`test_subroutine_references.md`](test_subroutine_references.md) are generated artifacts from the
source and scripts in this directory.

## Navigation

- [`test_subroutine_references.cpp`](test_subroutine_references.cpp) contains the callers, targets, and the already-known-function negative case.
- [`build.bat`](build.bat) locates an MSVC installation, compiles x64 code, and links a CRT-free executable.
- [`run_ghidra.py`](run_ghidra.py) creates a temporary Ghidra project, prepares disassembly without analysis, enables only `Subroutine References`, and writes the deterministic report.
- [`test_subroutine_references.exe`](test_subroutine_references.exe) is the generated PE input.
- [`test_subroutine_references.md`](test_subroutine_references.md) is the generated behavioral reference report.
- [`../../README.md`](../../README.md) describes the fixture collection boundary.
- [`../../../README.md`](../../../README.md) describes the C++23 feature collection.

## Reproduction

Run `build.bat` from this directory, then run
`TEST\\run_ghidra_python.bat NEW\\features\\function_analyzer\\test_data\\subroutine_references\\run_ghidra.py` from the repository root.
The script defaults to the executable and report beside itself, but accepts an input executable and
optional output report path.

The harness explicitly disassembles the executable section with code analysis disabled. It then
turns every analysis option off except `Subroutine References`, calls `project.analyze(program)`,
and records only direct call references and function state relevant to this analyzer.
