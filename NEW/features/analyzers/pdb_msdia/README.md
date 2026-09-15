# PDB MSDIA

This module is a real Windows Microsoft DIA provider boundary. On Windows it
initializes COM, creates `DiaSource`, loads the PDB, opens an `IDiaSession`,
and enumerates function/data symbols. On non-Windows it returns an explicit
platform error and never falls back to a fake reader or a function-name list.

- [`src/pdb_msdia.cppm`](src/pdb_msdia.cppm) contains the declaration and implementation.
- [`tests/pdb_msdia_tests.cppm`](tests/pdb_msdia_tests.cppm) checks provider symbols and platform behavior.
- [`tests/data/`](tests/data/) contains the real fixture and report.
- [`CMakeLists.txt`](CMakeLists.txt) locates `dia2.h`/`diaguids.lib` through the selected Visual Studio generator, `VSINSTALLDIR`, or `vswhere`, links COM, and defines CTest.
- [`build.bat`](build.bat) selects the focused build from `NEW/build.bat`.
- [`../pdb_universal/README.md`](../pdb_universal/README.md) documents the independent raw reader.
- [`../../pe_loader/README.md`](../../pe_loader/README.md) documents the mapped-image dependency.
- [`../README.md`](../README.md) documents analyzer-family integration.
