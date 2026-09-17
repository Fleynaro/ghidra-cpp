# Subroutine References Fixture

This is an adversarial MSVC x64 PE fixture for the C++23 implementation of Ghidra's
`Subroutine References` analyzer. The checked-in [`test_subroutine_references.exe`](test_subroutine_references.exe)
and [`test_subroutine_references.md`](test_subroutine_references.md) are generated artifacts from the
source and scripts in this directory.

## Navigation

- [`test_subroutine_references.cpp`](test_subroutine_references.cpp) contains direct chains, fan-in, recursion, conditional flow, thunk-like code, alignment, and unresolved indirect flow.
- [`build.bat`](build.bat) locates an MSVC installation, compiles x64 code, and links a CRT-free executable.
- [`run_ghidra.py`](run_ghidra.py) creates a temporary Ghidra project, prepares disassembly without analysis, enables only `Subroutine References`, and writes the deterministic report.
- [`test_subroutine_references.exe`](test_subroutine_references.exe) is the generated PE input.
- [`test_subroutine_references.md`](test_subroutine_references.md) is the generated behavioral reference report.
- [`../../README.md`](../../README.md) describes the fixture collection boundary.
- [`../../../README.md`](../../../README.md) describes the C++23 feature collection.

## Reproduction

Run `build.bat` from this directory, then run
`TEST\\run_ghidra_python.bat \features\\analyzers\\subroutine_references\\tests\\data\\run_ghidra.py`
from the repository root.

The harness explicitly disassembles the executable section with code analysis disabled. It then
turns every analysis option off except `Subroutine References`, calls `project.analyze(program)`,
and records call references and function state relevant to this analyzer. The
`.md` report is for human inspection only; GTests hardcode the oracle values and
never load the report.
