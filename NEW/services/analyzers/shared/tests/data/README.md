# Shared Function Body Fixture

This fixture belongs to the shared `AnalysisContext` infrastructure rather than
to any analyzer. [`test_function_body.cpp`](test_function_body.cpp) contains
straight-line, loop/switch, call, and data cases. [`build.bat`](build.bat)
builds the deterministic PE, and [`run_ghidra.py`](run_ghidra.py) creates the
human-readable [`test_function_body.md`](test_function_body.md) oracle.

Run `build.bat` here, then invoke
`TEST\\run_ghidra_python.bat \features\\analyzers\\shared\\tests\\data\\run_ghidra.py`
from the repository root.
