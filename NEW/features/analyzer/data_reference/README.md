# Data Reference

Ports `DataOperandReferenceAnalyzer` from
`Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/analysis/DataOperandReferenceAnalyzer.java`.

## Contract

- Required state: defined pointer-sized data objects and PE mapped memory.
- Consumes: `data_added`.
- Produces: data-origin DATA references.
- Priority: `602`.
- Consumer: state/reporting clients; it never creates functions.

Pointer width follows PE32 versus PE32+. Relocation-bearing non-executable
cells are used as loader evidence for pointer data; arbitrary section bytes are
not automatically promoted to data. Values are read from the loader's mapped
image and accepted only when the complete target is mapped.

Golden evidence: [`../test_data/data_reference/`](../test_data/data_reference/).
