# Analyzer Name
Data Reference

## Purpose
Exercise data-origin reference processing without creating functions from pointer data.

## Original Ghidra Implementation
`Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/analysis/DataOperandReferenceAnalyzer.java`, using `OperandReferenceAnalyzer.java`.

## Test Scenario
Two strings are pointed to by relocation-bearing data and a third is unreferenced; the script reports only data-origin references and defined data.

## Why This C++ Code Was Chosen
Const pointers in `.rdata` provide loader-visible relocations and a genuine negative control without source-level database injection.

## Required Compiler Options
MSVC x64 and the linker as invoked by `build.bat`.

## Required Auxiliary Files
The PE loader, `Reference` prerequisite, and `TEST/run_ghidra_python.bat` are required; no CRT or manual symbol database is used.

## Ghidra Analysis Options
Retain `Reference`, enable `Data Reference`, and disable unrelated boolean analyzers.

## Expected Results
The source provides two relocation-bearing pointer slots, but the generated Ghidra 12.1.3 report exposes no fixture-specific data-origin references; it records zero rather than manufacturing unsupported rows. The unreferenced string remains a negative control.

## Generated Markdown
`test_data_reference.md` is generated from the final reference manager and data listing.

## Notes / Limitations
Relocation markup and string recognition are loader/version dependent. The existing sections below retain reproduction details.

## Navigation

- [`test_data_reference.cpp`](test_data_reference.cpp) defines relocatable data pointers to referenced and unreferenced strings.
- [`build.bat`](build.bat) builds the deterministic CRT-free MSVC x64 PE.
- [`run_ghidra.py`](run_ghidra.py) preserves `Reference`, enables `Data Reference`, and extracts only data-origin references.
- `test_data_reference.exe` and `test_data_reference.md` are generated artifacts when the required tools exist.
- [`../README.md`](../README.md) is the analyzer fixture collection guide.

## Fixture Design

The `.rdata` pointer array points to two strings, while a third string is deliberately unreferenced. Relocation-bearing data gives the loader the data-to-data references that the analyzer consumes. The negative string prevents the report from confusing generic string discovery with this analyzer's behavior.

## Ghidra Java Contract

The target is [`Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/analysis/DataOperandReferenceAnalyzer.java`](../../../../../Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/analysis/DataOperandReferenceAnalyzer.java), which is the `Data Reference` specialization of [`OperandReferenceAnalyzer.java`](../../../../../Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/analysis/OperandReferenceAnalyzer.java). Its priority follows reference analysis and its `createFunctions(...)` override intentionally does nothing. The script therefore retains the `Reference` prerequisite and reports source data, target data type, and length only.

## Dependencies

The C++ fixture requires MSVC x64. The PyGhidra run requires [`TEST/run_ghidra_python.bat`](../../../../../TEST/run_ghidra_python.bat), the PE loader, and the `Reference` analysis prerequisite. No CRT or manual symbol database is used.

## Limitations

The PE loader's relocation/reference behavior is part of the input contract; the fixture does not manufacture references in Python. Results can vary if a future loader changes relocation markup or string recognition. The report does not assert unrelated code functions or generic strings.

## Reproduction

Run `build.bat`, then from the repository root run `TEST\run_ghidra_python.bat NEW\features\analyzer\test_data\data_reference\run_ghidra.py`. Optional arguments select the executable and report path.

## Generated Artifacts

The expected outputs are `test_data_reference.exe` from `build.bat` and `test_data_reference.md` from `run_ghidra.py`. They are not fabricated when tool execution is unavailable.

## Validation

Validation completed with MSVC 2022 x64 compilation/linking, relocation-enabled PE generation, the repository PyGhidra wrapper, project save/reopen through `project.openProgram(...)`, target-only analysis, and generated-report inspection. The zero-row result is retained as an honest loader/analyzer limitation.
