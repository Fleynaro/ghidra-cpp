# Function ID

This analyzer uses the existing [`../../function_id/`](../../function_id/) parser, hasher, and
scoring implementation. [`src/function_id.cppm`](src/function_id.cppm) only adapts native function
bodies and PE relocations to that public API; it does not duplicate FID storage or hashing.

[`tests/function_id_analyzer_tests.cppm`](tests/function_id_analyzer_tests.cppm) queries the genuine
packed fixture in [`tests/data/`](tests/data/). [`CMakeLists.txt`](CMakeLists.txt) defines the local
library and test target, and [`GHIDRA_PORT.md`](GHIDRA_PORT.md) records the option/state mapping.
