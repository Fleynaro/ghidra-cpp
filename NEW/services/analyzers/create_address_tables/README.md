# Create Address Tables

Ports `AddressTableAnalyzer` and the pointer-run portion of `AddressTable` from
[`Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/disassembler/AddressTableAnalyzer.java`](../../../../Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/disassembler/AddressTableAnalyzer.java)
and [`AddressTable.java`](../../../../Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/disassembler/AddressTable.java).

The analyzer scans initialized, searchable PE regions for aligned contiguous
32-bit or 64-bit pointers. It applies minimum-address, target-memory,
offcut, relocation, and maximum-distance checks before recording an
`AddressTableRecord`, table data, optional labels, bookmarks, and all-code
disassembly. `AnalysisContext` remains the sole owner of PE and Sleigh state.

Implementation: [`src/create_address_tables.cppm`](src/create_address_tables.cppm)
Build/tests: [`CMakeLists.txt`](CMakeLists.txt)
Build wrapper: [`build.bat`](build.bat)
Focused tests: [`tests/create_address_tables_tests.cppm`](tests/create_address_tables_tests.cppm)
Fixture evidence: [`tests/data/`](tests/data/)
Shared parent APIs: [`../shared/`](../shared/)
Port evidence: [`GHIDRA_PORT.md`](GHIDRA_PORT.md)
