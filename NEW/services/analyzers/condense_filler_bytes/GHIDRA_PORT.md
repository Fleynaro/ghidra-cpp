# Ghidra Port Evidence

This document records the native port of **Condense Filler Bytes** and links
the implementation, tests, configuration, and fixture evidence.

The native source is [`src/condense_filler_bytes.cppm`](src/condense_filler_bytes.cppm),
the target is [`CMakeLists.txt`](CMakeLists.txt), the wrapper is [`build.bat`](build.bat),
and the focused tests are [`tests/condense_filler_bytes_tests.cppm`](tests/condense_filler_bytes_tests.cppm).
Shared dependencies are documented in [`../shared/README.md`](../shared/README.md).

## Original Source

- [`CondenseFillerBytesAnalyzer.java`](../../../../Ghidra/Features/Base/src/main/java/ghidra/app/analyzers/CondenseFillerBytesAnalyzer.java)
  supplies `determineFillerValue()`, `added()`, `countUndefineds()`, option
  defaults, and `replaceFillerBytes()`.
- [`AlignmentDataType.java`](../../../../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/model/data/AlignmentDataType.java)
  supplies the alignment datatype represented by the native `alignment` data
  object.
- [`AnalysisContext`](../shared/src/analyzer_context.cppm) supplies function,
  instruction, data, and mapped-memory state without introducing a second PE or
  Sleigh implementation.

## Transferred Behavior

- Every function contributes only its immediately following undefined byte to
  Auto-mode frequency detection.
- The winning byte is used to count a contiguous undefined run, and the run is
  ignored when it is shorter than the configured minimum.
- Explicit filler selection bypasses frequency detection.
- Alignment data is add-only, overlap-safe, and idempotent under repeated event
  scheduling.
- The native default minimum remains the shared model's configured value; the
  fixture explicitly sets the original report's value of one.

## Differences and Risks

- The Java listing can expose a variable-length undefined `Data` code unit and
  its default representation. The native model has no undefined code-unit
  object, so the equivalent is an unowned mapped byte sequence.
- Java `Options` registration is represented by existing `AnalysisOptions`
  fields; this module does not add a separate option registry.
- The native result records alignment semantics but does not provide GUI
  datatype rendering such as `align(1)`.

## Verification

[`tests/condense_filler_bytes_tests.cppm`](tests/condense_filler_bytes_tests.cppm)
uses the real fixture and hardcodes the `0x1400002B9`/one-byte result from
[`tests/data/test_condense_filler_bytes.md`](tests/data/test_condense_filler_bytes.md).
It never reads the Markdown file at runtime. PyGhidra fixture scripts must be
launched via [`TEST/run_ghidra_python.bat`](../../../../TEST/run_ghidra_python.bat).
