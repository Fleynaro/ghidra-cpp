# Subroutine References

Ports `FunctionAnalyzer.added()` and `fallthroughCall()` from
`Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/function/FunctionAnalyzer.java`.

## Contract

- Required state: existing decoded instructions and materialized call references.
- Consumes: `code_added`, `reference_added`, and `flow_changed` events.
- Produces: missing functions, thunk relationships, body changes, and
  `function_added` events.
- Priority: `399`.
- Function construction is synchronous in [`AnalysisContext::create_function`](../shared/src/analyzer_context.cppm),
  which is the native equivalent of `CreateFunctionCmd.applyTo()` and its
  `FollowFlow`/overlap-repair calls.

The analyzer never scans raw PE bytes. It accepts unconditional, conditional,
and resolved computed call references, rejects calls whose destination is the
actual fall-through, preserves real existing functions, repairs only precise
one-address placeholders, and creates targets only after the decoder has
materialized the call flow.

## FunctionBodyAnalyzer investigation

Original Ghidra has no `FunctionBodyAnalyzer` class. Body construction happens
inside `CreateFunctionCmd.getFunctionBody()` and `fixupFunctionBody()`, which
delegate traversal to `FollowFlow`; block views are derived by
`BasicBlockModel` and `SimpleBlockModel`. The native `function_body` directory
now contains focused context tests only; no body analyzer target is registered
by the built-in pipeline. This removes the incorrect dependency where
`Subroutine References` waited for a separate body analyzer.

Porting evidence: [`GHIDRA_PORT.md`](GHIDRA_PORT.md). Golden evidence:
[`tests/data/`](tests/data/).
