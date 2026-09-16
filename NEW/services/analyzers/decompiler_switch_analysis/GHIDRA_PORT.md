# Ghidra Port Evidence

The port follows `Ghidra/Features/Decompiler/src/main/java/ghidra/app/plugin/core/analysis/DecompilerSwitchAnalyzer.java`
and `Ghidra/Features/Decompiler/src/main/java/ghidra/app/cmd/function/DecompilerSwitchAnalysisCmd.java`.
The implementation is [`src/decompiler_switch_analysis.cppm`](src/decompiler_switch_analysis.cppm),
with fixture verification in [`tests/decompiler_switch_analysis_tests.cppm`](tests/decompiler_switch_analysis_tests.cppm).

## Status

- **Complete:** Uses the existing native decompiler flow and C/control-flow artifacts.
- **Complete:** Persists recovered switch destinations as analysis-source computed-jump references and the observable `switchD` label.
- **Partial:** The adapter currently supplies the x86-64 PE register/space description because `AnalysisContext` does not expose its architecture object.
- **Partial:** The public `DecompilationResult` exposes control-flow text and raw instructions but not native `JumpTable` objects. The port therefore intersects printed native block starts with raw instructions; it does not reproduce private jump-table labels or normalization metadata.
- **Pending:** A first-class public jump-table result would remove the text-artifact boundary and is required for exact parity on exotic multi-stage tables.
