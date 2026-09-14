# Ghidra Port Evidence

- Original: [`Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/function/ExternalEntryFunctionAnalyzer.java`](../../../../Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/function/ExternalEntryFunctionAnalyzer.java).
- Native source: [`src/external_entry_references.cppm`](src/external_entry_references.cppm).
- Native tests: [`tests/external_entry_references_tests.cppm`](tests/external_entry_references_tests.cppm).
- Fixture evidence: [`tests/data/test_external_entry_references.md`](tests/data/test_external_entry_references.md).

The port preserves the analyzer name, priority `398`, enablement option, executable-entry filtering,
instruction-start requirement, fall-through rejection, existing-function exclusion, external-entry
marking for named exports and the PE image entry point, and named `CreateFunctionCmd`-equivalent body
creation.

The Java symbol table can contain external-entry symbols originating from formats other than PE.
The native public loader currently exposes PE named exports and an entry-point value, but no general
symbol-table external-entry provider; the entry point may duplicate a named export. The native test
therefore expects the three exported entry records rather than inventing the Java fixture's fourth
symbol-table-only record. This is a documented provider boundary, not a synthetic scan of arbitrary
bytes.
