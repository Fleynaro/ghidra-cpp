# Ghidra Port Evidence

## Scope

The implementation follows `Ghidra/Features/PDB/src/main/java/ghidra/app/plugin/core/analysis/PdbUniversalAnalyzer.java`,
`PdbAnalyzer.java`, `PdbAnalyzerCommon.java`, and the raw reader classes under
`Ghidra/Features/PDB/src/main/java/ghidra/app/util/bin/format/pdb2/pdbreader/`.
The checked-in behavior is exercised by
[`tests/pdb_universal_tests.cppm`](tests/pdb_universal_tests.cppm) against
[`tests/data/test_pdb_universal.pdb`](tests/data/test_pdb_universal.pdb).

## Status

- **Complete:** MSF 7.0 superblock and stream-directory validation.
- **Complete:** PDB identity (version, signature, age, GUID) parsing.
- **Complete:** TPI structures, unions, enums, field lists, pointers, arrays, procedures, and argument lists used by the fixture.
- **Complete:** DBI module symbol streams and the DBI global/public symbol stream for procedure, data, public, and COFF-group records.
- **Complete:** Native application of names, function bodies, signatures, PDB symbols, PDB type declarations, and an identity bookmark.
- **Partial:** Type records not listed above are rejected or omitted rather than guessed. Line tables, FPO data, locals, source files, and edit-and-continue records are not yet represented by the current native model.

## Design Decisions

The parser preserves section/offset/type-index provenance so applications can
match records to the loaded PE image. Bounds checks reject corrupt stream
lengths and page references. Procedure parameter storage is intentionally left
empty because CodeView TPI signatures describe types, not the decompiler's
recovered storage locations; storage must come from the decompiler frontend.

The current `AnalysisContext` has no CodeView debug-directory identity accessor,
so exact PE/PDB GUID and age matching is not silently simulated. The analyzer
parses the configured PDB and records the identity bookmark; callers that need
strict matching must perform that check at the loader boundary.

The current model also has no PDB-loaded property or source/line table API.
Those records are parsed only where they can be represented faithfully and are
documented as pending rather than discarded as if parity existed.

## Original References

- [`PdbUniversalAnalyzer.java`](../../../../Ghidra/Features/PDB/src/main/java/ghidra/app/plugin/core/analysis/PdbUniversalAnalyzer.java)
- [`PdbParser.java`](../../../../Ghidra/Features/PDB/src/main/java/ghidra/app/util/bin/format/pdb2/pdbreader/PdbParser.java)
- [`PdbNewDebugInfo.java`](../../../../Ghidra/Features/PDB/src/main/java/ghidra/app/util/bin/format/pdb2/pdbreader/PdbNewDebugInfo.java)
- [`SymbolRecords.java`](../../../../Ghidra/Features/PDB/src/main/java/ghidra/app/util/bin/format/pdb2/pdbreader/SymbolRecords.java)
