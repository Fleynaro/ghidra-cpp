# Subroutine References

Ports `FunctionAnalyzer.added()` and `fallthroughCall()` from
`Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/function/FunctionAnalyzer.java`.

## Contract

- Required state: existing decoded instructions and direct CALL references.
- Consumes: `code_added` and `reference_added`.
- Produces: missing functions and `function_added` events.
- Priority: `399`.
- Consumer: Function Body.

The analyzer never scans raw PE bytes. It rejects computed calls and calls whose
target is the recorded fall-through, preserves existing functions, and creates
targets only after the decoder has materialized the call flow. The resulting
body is built by the separate Function Body analyzer.

Golden evidence: [`tests/data/`](tests/data/).
