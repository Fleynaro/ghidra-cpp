# Ghidra Port Evidence

The port follows `Ghidra/Features/Decompiler/src/main/java/ghidra/app/plugin/core/analysis/DecompilerFunctionAnalyzer.java`
and `Ghidra/Features/Decompiler/src/main/java/ghidra/app/cmd/function/DecompilerParameterIdCmd.java`.
The native implementation is [`src/decompiler_parameter_id.cppm`](src/decompiler_parameter_id.cppm),
with fixture tests in [`tests/decompiler_parameter_id_tests.cppm`](tests/decompiler_parameter_id_tests.cppm).

## Status

- **Complete:** Bounded machine code is passed through the production Sleigh decoder and native decompiler frontend.
- **Complete:** Completion is persisted through `AnalysisContext::set_parameter_id_complete` only after frontend output exists.
- **Partial:** The adapter currently supplies the x86-64 PE register/space description because `AnalysisContext` does not expose its architecture object.
- **Intentionally partial:** The current native model does not expose the decompiler's recovered high-variable objects or a signature transaction API, so this module does not copy speculative parameter names/types into `Function`.
- **Pending:** Timeout cancellation is represented by the analyzer token, but the frontend itself currently has no interrupt callback boundary.

This preserves decompiler behavior rather than substituting ABI heuristics.
