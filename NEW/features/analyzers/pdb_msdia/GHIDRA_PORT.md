# Ghidra Port Evidence

The source traces `Ghidra/Features/PDB/src/main/java/ghidra/app/plugin/core/analysis/PdbMsdiaAnalyzer.java`,
`PdbAnalyzer.java`, and `PdbAnalyzerCommon.java`. The native provider is
[`src/pdb_msdia.cppm`](src/pdb_msdia.cppm), and its checked-in fixture contract
is tested by [`tests/pdb_msdia_tests.cppm`](tests/pdb_msdia_tests.cppm).

## Status

- **Complete:** Windows COM initialization, `DiaSource` creation, `loadDataFromPdb`, `openSession`, global-scope function/data enumeration, BSTR conversion, and deterministic COM teardown.
- **Complete:** Function and data symbols are applied to the native model with real DIA virtual addresses and lengths.
- **Complete:** Non-Windows builds return a deterministic error explaining the missing Windows DIA provider.
- **Partial:** The current native model has no DIA type graph, line-table, source-file, or PDB identity property APIs. Those records are not fabricated or silently represented as function names.
- **Pending:** Type and source application require new model boundaries before exact Ghidra DIA parity is possible.
