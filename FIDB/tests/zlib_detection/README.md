# zlib Detection Test

[`test.cpp`](test.cpp) performs real `compress`, `uncompress`, `crc32_z`, and `adler32_z` operations. The generated executable also contains `non_zlib_control`, which is used as a negative control. The compiler/linker invocation in [`build_consumer.py`](build_consumer.py) does not request a PDB, so positive recognition must come from Ghidra's Function ID hashes and not from consumer debug symbols.

[`run_test.py`](run_test.py) runs all build, analysis, generation, and verification stages through one command. The actual Ghidra verification is implemented in [`../../scripts/verify_fidb.py`](../../scripts/verify_fidb.py), which opens the packed database, invokes original `FidService` and `FidAnalyzer` APIs, and writes [`../../reports/fidb_verification.json`](../../reports/fidb_verification.json).
