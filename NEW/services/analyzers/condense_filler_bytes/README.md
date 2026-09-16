# Condense Filler Bytes

Ports [`CondenseFillerBytesAnalyzer.java`](../../../../Ghidra/Features/Base/src/main/java/ghidra/app/analyzers/CondenseFillerBytesAnalyzer.java).
The analyzer samples the first undefined byte after each known function,
selects the most frequent value in Auto mode, counts contiguous undefined bytes,
and records matching runs as `alignment` data objects.

The native declaration and implementation are in
[`src/condense_filler_bytes.cppm`](src/condense_filler_bytes.cppm). Build and
CTest registration are in [`CMakeLists.txt`](CMakeLists.txt); the incremental
wrapper is [`build.bat`](build.bat); focused tests are in
[`tests/condense_filler_bytes_tests.cppm`](tests/condense_filler_bytes_tests.cppm).
The executable fixture and PyGhidra evidence are under [`tests/data/`](tests/data/),
and shared model dependencies are under [`../shared/`](../shared/).
Porting details are in [`GHIDRA_PORT.md`](GHIDRA_PORT.md).
