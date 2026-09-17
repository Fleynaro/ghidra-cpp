# PDB Universal Analyzer

This module provides the platform-independent raw Microsoft PDB reader and its
event-driven analyzer. It reads the PDB 7.0 MSF container, the PDB identity
stream, TPI type records, DBI module streams, and CodeView procedure, data,
public, and COFF-group records. It does not shell out to `llvm-pdbutil`, DIA,
or a fake function-name list.

## Navigation

- [`src/pdb_universal.cppm`](src/pdb_universal.cppm) contains the declaration and implementation.
- [`tests/pdb_universal_tests.cppm`](tests/pdb_universal_tests.cppm) contains hardcoded parser and applicator tests.
- [`tests/data/`](tests/data/) contains the PE/PDB fixture, source, and checked-in behavior report.
- [`CMakeLists.txt`](CMakeLists.txt) defines the `analyzer_pdb_universal` library and CTest target.
- [`build.bat`](build.bat) selects the focused build from `build.bat`.
- [`../README.md`](../README.md) documents the analyzer feature family.
- [`../../pe_loader/README.md`](../../pe_loader/README.md) documents the PE section/address dependency.
- [`../../decompiler/README.md`](../../decompiler/README.md) documents the frontend consumed by related analyzers.
- [`../../function_id/README.md`](../../function_id/README.md) documents adjacent Function ID infrastructure.

The target is included by the existing analyzer aggregate without changing
that aggregate file. Build it with `build.bat analyzer` or the focused
module wrapper when configured.
