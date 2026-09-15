# Aggregate Ghidra Port Evidence

This directory is port-level integration evidence for the native analyzer
pipeline. The individual ports remain documented beside their implementations
under [`../`](../).

## Original Behavior Studied

The fixture and oracle combine behavior from the original Java analyzers listed
in [`data/test_analyzers_integration.cpp`](data/test_analyzers_integration.cpp),
including `AutoAnalysisManager`, entry/function discovery, operand and data
references, stack analysis, no-return repair, string/media/data scanners,
decompiler-dependent parameter/switch analysis, demangling, resource
references, Function ID, PDB providers, and address/filler analyzers.

## Transferred Coverage

The native test registers all instances from
[`../analyzer_builtin.cpp`](../analyzer_builtin.cpp) through the public manager
adapter. It validates artifacts in the shared model rather than comparing
addresses from a compiler-specific layout. The repeat pass verifies event
re-analysis is idempotent and that duplicate references, overlapping bodies,
or conflicting state are not introduced.

## Intentional Limitations

- The executable is x64, so x86-only analyzers execute their architecture guard
  and are validated as safe no-ops; the focused x86 fixtures retain positive
  coverage for those implementations.
- Data archives are enabled but no external `.gdt` is supplied because the
  native archive service is intentionally not a runtime dependency.
- PDB Universal and PDB MSDIA remain registered but are disabled for this raw
  image. The matching PDB emitted by `build.bat` is available to the focused
  provider tests; omitting it here prevents PDB-defined objects from masking
  the same initialized storage that ASCII, data-reference, and address-table
  analyzers must consume together. The focused MSDIA test retains its
  platform-boundary coverage.
- Function ID uses the production database discovery path and may find no match;
  the integration contract checks that it cannot corrupt existing state.
- The Markdown oracle is bounded and for development comparison only. The C++
  test contains all runtime expectations directly and does not parse the report.

## Related Validation

- [`../shared/src/analyzer_manager.cppm`](../shared/src/analyzer_manager.cppm)
  supplies scheduling and repeat-analysis behavior.
- [`../shared/src/analyzer_context.cppm`](../shared/src/analyzer_context.cppm)
  supplies the artifact model and duplicate/overlap boundaries.
- [`analyzer_global_integration_tests.cppm`](analyzer_global_integration_tests.cppm)
  is the regression test.
- [`run_ghidra.py`](run_ghidra.py) is the bounded external oracle generator.
