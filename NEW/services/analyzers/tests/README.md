# Analyzer Global Integration Tests

This directory contains the additional end-to-end test for the complete native
analyzer registration. It does not replace any focused analyzer test.

## Navigation

- [`analyzer_global_integration_tests.cppm`](analyzer_global_integration_tests.cppm) loads the single PE, calls `register_builtin_analyzers_impl` through [`AutoAnalysisManager`](../shared/src/analyzer_manager.cppm), checks cross-analyzer artifacts, and repeats analysis for stability.
- [`CMakeLists.txt`](CMakeLists.txt) builds and registers `analyzer_global_integration_tests` with CTest.
- [`run_ghidra.py`](run_ghidra.py) runs one bounded all-analyzer Ghidra workflow and writes the development oracle.
- [`data/test_analyzers_integration.cpp`](data/test_analyzers_integration.cpp) is the realistic CRT-free GTA-like engine fixture.
- [`data/build.bat`](data/build.bat) compiles the fixture with MSVC, links real Windows resources, and emits a matching PDB.
- [`data/test_analyzers_integration.rc`](data/test_analyzers_integration.rc) and [`data/resource.h`](data/resource.h) provide string-table, dialog, and menu resources.
- [`data/test_analyzers_integration.md`](data/test_analyzers_integration.md) is generated evidence only; the C++ test never reads it.
- [`../README.md`](../README.md) documents the aggregate analyzer target and registration list.

## Test Contract

The fixture exercises hierarchy dispatch, constructors/destructors, global and
static state, pointers/references, arrays, strings, data pointers, callbacks,
switches, arithmetic constants, stack locals, Windows imports/resources,
no-return paths, embedded media, linker filler, address tables, exports, and
unused code. The native test enables all supported option flags and registers
all 34 aggregate analyzer instances without manually selecting a subset.

The test asserts scheduler coverage, function and body validity, signatures and
stack state, code/data/reference interactions, string/media/resource artifacts,
external and no-return behavior, uniqueness, and exact final-state equality
after `re_analyze_all()`.
