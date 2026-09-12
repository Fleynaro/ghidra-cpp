# Function Analyzer Test Data

This directory contains integration-test data for the future C++23 port of Ghidra's
`Subroutine References` analyzer. It deliberately contains no analyzer implementation or
build target in the C++ project.

## Navigation

- [`test_data/subroutine_references/README.md`](test_data/subroutine_references/README.md) describes the MSVC x64 fixture.
- [`test_data/subroutine_references/test_subroutine_references.cpp`](test_data/subroutine_references/test_subroutine_references.cpp) defines the direct-call cases.
- [`test_data/subroutine_references/build.bat`](test_data/subroutine_references/build.bat) builds the executable with MSVC x64.
- [`test_data/subroutine_references/run_ghidra.py`](test_data/subroutine_references/run_ghidra.py) runs the isolated Ghidra analysis and writes the report.
- [`../README.md`](../README.md) describes the surrounding C++23 feature collection.
