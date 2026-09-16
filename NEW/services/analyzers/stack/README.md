# Stack

Ports `StackVariableAnalyzer.added()` and the local-variable path of
`NewFunctionStackAnalysisCmd` from `Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/function/`.

## Contract

- Required state: function bodies and decoded stack-based operands.
- Consumes: `function_added`.
- Produces: stack variables, stack references, and function changes.
- Priority: `903`.
- Consumers: listing/reporting clients.

Stack/frame-pointer displacements are signed, frame allocation and SP delta are
recorded, storage widths follow operand qualifiers, and locals/parameters are
deduplicated by offset/size. Stack references retain a typed signed offset
instead of fabricating a process virtual address. Parameter creation remains
disabled by default, matching the requested initial layer.

Golden evidence: [`tests/data/`](tests/data/).
