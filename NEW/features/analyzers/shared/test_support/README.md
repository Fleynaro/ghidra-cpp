# Analyzer Test Support

This directory contains the shared support used by analyzer tests. The C++23
module [`analyzer_test_support.cppm`](analyzer_test_support.cppm) provides
fixture loading and typed function-body assertions for tests in sibling
analyzer directories. [`CMakeLists.txt`](CMakeLists.txt) builds it as
`NewGhidra::AnalyzerTestSupport` when `BUILD_TESTING` is enabled.

The Python helpers [`evidence.py`](evidence.py) and
[`pdb_validation.py`](pdb_validation.py) are used by fixture scripts under
[`../../*/tests/data/`](../../). They must be imported from this directory and
must not be copied into individual fixture directories. The parent shared
runtime is documented in [`../README.md`](../README.md), while the analyzer
build and registration are documented in [`../../README.md`](../../README.md).
