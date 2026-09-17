# BSim Fixture Data

This directory contains the existing small native PE used as deterministic BSim
input. The source is intentionally ASCII-only and uses externally visible,
no-inline functions so decompilation and feature extraction can inspect stable
standalone boundaries.

## Contents

- [`bsim_fixture.cpp`](bsim_fixture.cpp) defines exactly 30 utility functions
  plus a driver. The semantic twin groups are insertion/selection sorting,
  forward/reverse first-value search, scan/pairwise maximum, branch/mask
  absolute value, forward/reverse additive checksum, and indexed/pointer FNV-1a
  hashing. The remaining 18 functions cover unrelated arithmetic, bit,
  numeric, and byte utilities.
- [`build.bat`](build.bat) discovers an active MSVC developer environment or
  uses `vswhere`, then builds `bsim_fixture.exe` with a matching PDB.
- [`run_ghidra.py`](run_ghidra.py) imports the existing executable through
  PyGhidra, enumerates functions, calls `DecompInterface.generateSignatures`,
  builds weighted vectors with Ghidra's `WeightedLSHCosineVectorFactory`, and
  writes the compact scalar oracle [`bsim_fixture.md`](bsim_fixture.md).
- [`bsim_fixture.md`](bsim_fixture.md) records selected function metadata and
  pairwise Ghidra cosine values only; raw feature hashes and complete vectors
  are deliberately excluded.

Run `build.bat` from this directory, then from the repository root run
`TEST\\run_ghidra_python.bat NEW\\services\\bsim\\tests\\data\\run_ghidra.py`.
The generated executable and PDB remain beside the source for consumers that
need a reproducible PE fixture; they are the existing fixture inputs, not an
additional test binary.
