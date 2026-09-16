# Ghidra Port Evidence

- Original: [`Ghidra/Features/MicrosoftCodeAnalyzer/src/main/java/ghidra/app/plugin/prototype/MicrosoftCodeAnalyzerPlugin/PropagateExternalParametersAnalyzer.java`](../../../../Ghidra/Features/MicrosoftCodeAnalyzer/src/main/java/ghidra/app/plugin/prototype/MicrosoftCodeAnalyzerPlugin/PropagateExternalParametersAnalyzer.java).
- Native source: [`src/params.cppm`](src/params.cppm).
- Native tests: [`tests/params_tests.cppm`](tests/params_tests.cppm).
- Fixture evidence: [`tests/data/test_windows_pe_x86_propagate_external_parameters.md`](tests/data/test_windows_pe_x86_propagate_external_parameters.md).

The port preserves the exact name, default enablement, `905` priority, x86/AMD64 PE gate,
call-reference filtering, function-body PUSH ordering, sufficient-pushes requirement, signature
parameter ordering, and optional bookmark behavior.

The original uses external `Function`/`Parameter` objects and writes comments and data types with
`SetCommentCmd` and `CreateDataCmd`. The public native model has no PDB loader, external-function
parameter provider, EOL comment, or data-type mutation API. The module therefore does nothing when
those signatures are absent. When a signature is explicitly present, it records the same parameter
locations through the existing `SymbolRecord` and bookmark methods; it does not fabricate Windows
API signatures or claim an unsupported comment/data-type mutation occurred.
