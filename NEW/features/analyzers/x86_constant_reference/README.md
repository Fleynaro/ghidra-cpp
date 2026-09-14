# x86 Constant Reference

[`src/x86_constant_reference.cppm`](src/x86_constant_reference.cppm) ports the x86 LEA/reference
portion of `ConstantPropagationAnalyzer` and `PropagateX86ConstantReferences.java`. It consumes
existing `AnalysisContext` constant facts and Sleigh operands, and emits normal DATA references.

Focused tests are in [`tests/x86_constant_reference_tests.cppm`](tests/x86_constant_reference_tests.cppm),
with the original PE/PDB evidence in [`tests/data/`](tests/data/). Build and registration are local
to [`CMakeLists.txt`](CMakeLists.txt); fidelity boundaries are in [`GHIDRA_PORT.md`](GHIDRA_PORT.md).
