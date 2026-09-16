# Ghidra Port Evidence

- Original analyzer: [`Ghidra/Features/FunctionID/src/main/java/ghidra/feature/fid/analyzer/FidAnalyzer.java`](../../../../Ghidra/Features/FunctionID/src/main/java/ghidra/feature/fid/analyzer/FidAnalyzer.java).
- Original application command: [`Ghidra/Features/FunctionID/src/main/java/ghidra/feature/fid/cmd/ApplyFidEntriesCommand.java`](../../../../Ghidra/Features/FunctionID/src/main/java/ghidra/feature/fid/cmd/ApplyFidEntriesCommand.java).
- Existing native infrastructure: [`../../function_id/src/function_id.cppm`](../../function_id/src/function_id.cppm), [`../../function_id/src/hasher.cppm`](../../function_id/src/hasher.cppm), and [`../../function_id/src/database.cppm`](../../function_id/src/database.cppm).
- Native analyzer: [`src/function_id.cppm`](src/function_id.cppm).
- Native test: [`tests/function_id_analyzer_tests.cppm`](tests/function_id_analyzer_tests.cppm).

## Preserved Behavior

The native module retains priority `799`, packed `.fidb` opening, x86 language filtering,
Sleigh-derived instruction hashing, PE relocation masking for HIGH/LOW/HIGHLOW/HIGHADJ/DIR64 fields, the four-unit hasher minimum, the 14.6
default score threshold, 30-point multiple-name threshold, name deduplication, and Function ID
bookmark category/comment shape. Database paths may be files or directories and are processed in
deterministic order.

## Public API Boundaries

`AnalysisOptions` does not currently expose the Java analyzer's `Always Apply FID Labels`,
`Ignore Database Filters`, score-threshold, or multiple-name-threshold option fields. The port uses
the existing database defaults and only changes a non-user-looking native `FUN_...` name through the
public `set_function_name` method. `AnalysisContext` has no plate-comment or source-type mutation,
so the Java comment/source bookkeeping is represented only by `SymbolRecord` and the existing
bookmark state. No alternate parser, hasher, or FID database is introduced.
