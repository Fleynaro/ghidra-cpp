# Function Start Search

Ports the `FunctionStartPreFuncAnalyzer`, `FunctionStartAnalyzer`,
`FunctionStartFuncAnalyzer`, `FunctionStartPostAnalyzer`, and
`FunctionStartDataPostAnalyzer` chain from
`Ghidra/Features/BytePatterns/src/main/java/ghidra/app/analyzers/`.

## Contract

- Required state: executable bytes, decoded validation instructions, and existing functions when a delayed rule requires them.
- Consumes: memory, code, function, and data events depending on phase.
- Produces: delayed candidates, decoded code, functions, and pattern bookmarks.
- Priorities: `199`, `402`, `498`, and `898`.
- Consumers: function body, stack, and later analyses.

Candidates are retained in `AnalysisContext::potential_function_starts()` like
Ghidra's property map. When `AnalysisOptions::pattern_root` is configured, the
pass loads concrete post-wildcard suffixes from the original `<data>` XML
patterns and validates each match through Sleigh. Without a pattern directory,
the provider-backed fallback examines filler boundaries and PE exports rather
than treating every byte as code. Candidates are restricted to executable
memory, rejected when already functions, and materialized through the same
flow-based function creation path. Pattern indexes are retained in bookmark
comments. The compiled provider profiles currently limit execution to
architectures for which an SLA is available.

Golden evidence: [`../test_data/function_start_search/`](../test_data/function_start_search/).
