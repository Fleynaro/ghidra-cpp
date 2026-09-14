# Ghidra Port Evidence

This document records the port of **Create Address Tables** and links its
source, build target, tests, fixture, and shared model boundary.

The native source is [`src/create_address_tables.cppm`](src/create_address_tables.cppm),
the target is [`CMakeLists.txt`](CMakeLists.txt), the wrapper is [`build.bat`](build.bat),
and the focused tests are [`tests/create_address_tables_tests.cppm`](tests/create_address_tables_tests.cppm).
Shared dependencies are documented in [`../shared/README.md`](../shared/README.md).

## Original Sources

- [`AddressTableAnalyzer.java`](../../../../Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/disassembler/AddressTableAnalyzer.java)
  supplies `canAnalyze()`, searchable-memory filtering, defined-code filtering,
  option defaults, table iteration, table processing, bookmarks, and valid-code
  disassembly.
- [`AddressTable.java`](../../../../Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/disassembler/AddressTable.java)
  supplies pointer decoding, alignment, minimum-address, memory, relocation,
  collision, distance, table truncation, and function-entry rules.
- [`AddressTableAnalyzerTest.java`](../../../../Ghidra/Features/Base/src/test/java/ghidra/app/plugin/core/disassembler/AddressTableAnalyzerTest.java)
  supplies the undefined pointer-run and bookmark fixture contract.

## Transferred Behavior

- PE32 and PE32+ pointer widths are selected from the loader's optional header.
- Search is restricted to initialized regions having at least one read/write/
  execute permission, with table-start and pointer-target alignment enforced.
- Zero, unmapped, too-small, too-distant, relocation-inconsistent, and
  disallowed offcut targets terminate a candidate table.
- A table is recorded only when its configured minimum entry count is reached;
  a sentinel terminates the run. The native model records one table data object
  spanning all entries and preserves every resolved target in the record.
- Bookmarks use category `Address Table` and the original `Address table[N]
  created` comment. Optional deterministic analysis symbols represent
  `autoLabelTable`.
- When all targets are executable, `AnalysisContext::disassemble_flow()` is
  used instead of duplicating Sleigh decoding.

## Differences and Risks

- The Java `AddressTable` can create one PointerDataType code unit per entry,
  secondary index arrays, switch-table labels, and function-body repairs. The
  current shared model exposes one `AddressTableRecord` and one table data
  object; index-table and switch-specific behavior remains pending because no
  corresponding native model fields exist.
- Java's relocation table abstraction is mapped to parsed PE relocation entries.
  Relocation guidance is enforced only when the image contains relocation
  entries, matching the Java relocatable/non-empty guard.
- Full Unicode-string and low-bit processor-mode heuristics are not reimplemented
  in this x64-oriented module; instruction offcuts are conservatively rejected
  unless the shared option allows them.

## Verification

[`tests/create_address_tables_tests.cppm`](tests/create_address_tables_tests.cppm)
hardcodes the four pointer rows at `0x140002038` through `0x140002050` from
[`tests/data/test_create_address_tables.md`](tests/data/test_create_address_tables.md)
and never reads Markdown at runtime. PyGhidra scripts use the required
[`TEST/run_ghidra_python.bat`](../../../../TEST/run_ghidra_python.bat) wrapper.
