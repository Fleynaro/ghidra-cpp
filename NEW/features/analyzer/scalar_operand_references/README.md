# Scalar Operand References

Ports `ScalarOperandAnalyzer.checkOperands()`, `addReference()`,
`checkOffcutFuncRef()`, and jump-table filtering from
`Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/analysis/ScalarOperandAnalyzer.java`.

## Contract

- Required state: decoded operands and mapped PE regions.
- Consumes: `code_added`.
- Produces: operand-indexed scalar DATA references and address data objects.
- Priority: `598`.
- Consumer: reference/reporting clients.

Small constants, unmapped values, and duplicate operand references are rejected.
Mapped large values are retained with their operand index and never replace an
existing relation. This preserves the positive/negative-control structure of the
fixture rather than classifying every immediate as an address.

Golden evidence: [`../test_data/scalar_operand_references/`](../test_data/scalar_operand_references/).
