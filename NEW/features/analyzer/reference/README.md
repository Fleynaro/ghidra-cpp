# Reference

Ports the instruction-side behavior of `OperandReferenceAnalyzer` from
`Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/analysis/OperandReferenceAnalyzer.java`.

## Contract

- Required state: Sleigh p-code and mapped PE regions.
- Consumes: `code_added`.
- Produces: analysis-source DATA references.
- Priority: `600`.
- Consumers: data references and downstream state observers.

LOAD and STORE address operands are resolved only when their p-code value is
concrete and mapped. The implementation does not create functions from data
operands, and duplicate references are retained only once.

Golden evidence: [`../test_data/reference/`](../test_data/reference/) and
[`../test_data/data_reference/`](../test_data/data_reference/).
