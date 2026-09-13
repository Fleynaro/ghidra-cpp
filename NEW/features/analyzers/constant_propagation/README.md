# Constant Propagation

Ports the p-code data-flow contract of `ConstantPropagationAnalyzer` and the
misspelled original `SymbolicPropogator` from
`Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/analysis/` and
`Ghidra/Features/Base/src/main/java/ghidra/program/util/SymbolicPropogator.java`.

## Contract

- Required state: function bodies and materialized Sleigh p-code.
- Consumes: `code_added` and `function_added`.
- Produces: register/memory `ConstantFact` values and constant events.
- Priority: `596`.
- Consumers: reference and later analysis layers.

The evaluator handles COPY/CAST/extension, integer arithmetic and comparisons,
shifts, signed operations, LOAD/STORE memory facts, call clobbering, and a
bounded CFG worklist with conservative state joins. Values are width-truncated
at p-code destinations and division by zero remains unknown. Unresolved
indirect control flow is not guessed.

Golden constant evidence: [`../x86_constant_reference/tests/data/`](../x86_constant_reference/tests/data/).
