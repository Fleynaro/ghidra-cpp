# BSim Fixture Data

This directory contains a small native PE used as deterministic BSim input.
The source is intentionally ASCII-only and uses externally visible, no-inline
functions so decompilation and feature extraction can inspect stable standalone
boundaries.

## Contents

- [`bsim_fixture.cpp`](bsim_fixture.cpp) defines exactly 30 utility functions
  plus a driver. The semantic twin groups are insertion/selection sorting,
  forward/reverse first-value search, scan/pairwise maximum, branch/mask
  absolute value, forward/reverse additive checksum, and indexed/pointer FNV-1a
  hashing. The remaining 18 functions cover unrelated arithmetic, bit,
  numeric, and byte utilities.
- [`build.bat`](build.bat) discovers an active MSVC developer environment or
  uses `vswhere`, then builds `bsim_fixture.exe` with a matching PDB.

Run `build.bat` from any working directory. The generated executable and PDB
remain beside the source for consumers that need a reproducible PE fixture;
they are build outputs rather than additional source inputs.
