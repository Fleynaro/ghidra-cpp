# Analyzer Name
Data Reference

## Purpose
Exercise data-origin reference processing without creating functions from pointer data.

## Original Ghidra Implementation
`Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/analysis/DataOperandReferenceAnalyzer.java`, using `OperandReferenceAnalyzer.java`.

## Test Scenario
Two strings are pointed to by relocation-bearing data and a third is unreferenced. The entry function reads a pointer slot itself so the `Reference` prerequisite materializes a real pointer-data source; the script reports only data-origin references newly observable after analysis.

## Why This C++ Code Was Chosen
Const pointers in `.rdata` provide relocations, and the explicit pointer-slot load gives the `Reference` prerequisite a genuine code-to-data edge without Python-created references or source-level database injection.

## Required Compiler Options
MSVC x64 and the linker as invoked by `build.bat`.

## Required Auxiliary Files
The PE loader, `Reference` prerequisite, and `TEST/run_ghidra_python.bat` are required; no CRT or manual symbol database is used.

## Ghidra Analysis Options
Retain `Reference`, enable `Data Reference`, and disable unrelated boolean analyzers.

## Expected Results
The generated report must contain at least one data-origin reference with a defined pointer target. The unreferenced string remains a negative control, and the script fails rather than accepting a zero-row result.

## Generated Markdown
`test_data_reference.md` is generated from before/after snapshots of the reference manager and data listing, with an explicit target delta.

## Notes / Limitations
Relocation markup and string recognition are loader/version dependent. The existing sections below retain reproduction details.

## Navigation

- [`test_data_reference.cpp`](test_data_reference.cpp) defines relocatable data pointers to referenced and unreferenced strings.
- [`build.bat`](build.bat) builds the deterministic CRT-free MSVC x64 PE.
- [`run_ghidra.py`](run_ghidra.py) preserves `Reference`, enables `Data Reference`, snapshots source data before/after analysis, and extracts only newly observed data-origin references.
- `test_data_reference.exe` and `test_data_reference.md` are generated artifacts when the required tools exist.
- [`../README.md`](../README.md) is the analyzer fixture collection guide.

## Fixture Design

The `.rdata` pointer array points to two real pointer-data objects, while a third string is deliberately unreferenced. The entry load forces the normal Reference prerequisite to retain the pointer slots; standard Ghidra pointer definitions make the C++ pointer objects explicit, and Data Reference consumes their real data-origin references. The negative string prevents the report from confusing generic string discovery with this analyzer's behavior.

## Ghidra Java Contract

The target is [`Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/analysis/DataOperandReferenceAnalyzer.java`](../../../../../Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/analysis/DataOperandReferenceAnalyzer.java), which is the `Data Reference` specialization of [`OperandReferenceAnalyzer.java`](../../../../../Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/analysis/OperandReferenceAnalyzer.java). Its priority follows reference analysis and its `createFunctions(...)` override intentionally does nothing. The script therefore retains the `Reference` prerequisite and reports source data, target data type, and length only.

## Dependencies

The C++ fixture requires MSVC x64. The PyGhidra run requires [`TEST/run_ghidra_python.bat`](../../../../../TEST/run_ghidra_python.bat), the PE loader, and the `Reference` analysis prerequisite. No CRT or manual symbol database is used.

## Limitations

The PE loader and Reference prerequisite provide the input reference; the fixture does not manufacture references in Python. The harness uses the standard Ghidra pointer-data command to expose the C++ pointer objects that a format/data loader would define. Results can vary if a future loader changes relocation markup or string recognition, but the report fails if no data-origin result is produced. It does not assert unrelated code functions or generic strings.

## Reproduction

Run `build.bat`, then from the repository root run `TEST\run_ghidra_python.bat \features\\analyzers\data_reference\\tests\\data\\\\run_ghidra.py`. Optional arguments select the executable and report path.

## Generated Artifacts

The expected outputs are `test_data_reference.exe` from `build.bat` and `test_data_reference.md` from `run_ghidra.py`. They are not fabricated when tool execution is unavailable.

## Validation

Validation completed with MSVC 2022 x64 compilation/linking, relocation-enabled PE generation, the repository PyGhidra wrapper, project save/reopen through `project.openProgram(...)`, target-only analysis, data-origin extraction, and generated-report inspection. The report contains real data-origin results rather than a fabricated zero-row claim.
