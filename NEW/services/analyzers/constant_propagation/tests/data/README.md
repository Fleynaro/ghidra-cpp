# Constant Propagation Fixture

This fixture contains loop/switch arithmetic, conditional joins, global memory
stores, and two interacting exported functions. [`test_constant_propagation.cpp`](test_constant_propagation.cpp)
is built by [`build.bat`](build.bat), and [`run_ghidra.py`](run_ghidra.py)
creates the human-readable [`test_constant_propagation.md`](test_constant_propagation.md)
report.

Run `build.bat` from this directory, then run
`TEST\\run_ghidra_python.bat NEW\\features\\analyzers\\constant_propagation\\tests\\data\\run_ghidra.py`
from the repository root.
