# Windows PE x86 External Parameters

[`src/params.cppm`](src/params.cppm)
ports the x86 PUSH traversal from `PropagateExternalParametersAnalyzer`. It uses the existing
[`../reference/`](../reference/) reference analyzer plus signatures already stored by the public
native `AnalysisContext`; it does not guess PDB declarations or duplicate PE import parsing.

Focused tests are in [`tests/params_tests.cppm`](tests/params_tests.cppm),
fixture evidence is in [`tests/data/`](tests/data/), and build registration is in
[`CMakeLists.txt`](CMakeLists.txt). The exact missing API boundary is documented in
[`GHIDRA_PORT.md`](GHIDRA_PORT.md).
