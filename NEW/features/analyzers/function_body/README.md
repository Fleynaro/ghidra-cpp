# Function Body Tests

This directory contains focused tests for the flow and body behavior used by
`CreateFunctionCmd`, `FollowFlow`, `SimpleBlockModel`, and `BasicBlockModel`.
The implementation lives in [`../shared/src/analyzer_context.cppm`](../shared/src/analyzer_context.cppm),
not in a separate analyzer class.

## Contract

- Required state: decoded instructions, flow references, and function entries.
- `AnalysisContext::create_function()` constructs bodies synchronously.
- `AnalysisContext::rebuild_function_body()` performs explicit repair when a
  flow override or existing-function change requires it.
- The focused target is [`tests/function_body_tests.cppm`](tests/function_body_tests.cppm).

Calls are excluded from body traversal but retain call and fall-through
semantics. `Function::instruction_starts` preserves traversal units while
`Function::body` and `body_ranges` expose the complete byte AddressSet. Direct
and conditional jumps become CFG edges, returns terminate blocks, and block
starts include entries, flow destinations, and post-terminator fall-through.
Overlap carving, placeholder repair, terminal-flow handling, and unresolved
indirect flow are tested against the corresponding Ghidra command behavior.
There is intentionally no `FunctionBodyAnalyzer`: original Ghidra has no such
analyzer and no priority-400 dependency.

Golden body evidence: [`../subroutine_references/tests/data/`](../subroutine_references/tests/data/).
