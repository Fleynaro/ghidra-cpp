# Ghidra Port Evidence

## Scope

The source traces `Ghidra/Features/Decompiler/src/main/java/ghidra/app/plugin/core/analysis/DecompilerCallConventionAnalyzer.java`
and `Ghidra/Features/Decompiler/src/main/java/ghidra/app/cmd/function/DecompilerParallelConventionAnalysisCmd.java`.
The native frontend boundary is [`NEW/features/decompiler/src/decompiler.cppm`](../../decompiler/src/decompiler.cppm),
and the fixture test is [`tests/call_convention_id_tests.cppm`](tests/call_convention_id_tests.cppm).

## Status

- **Complete:** Eligible functions are sent through the native decompiler provider boundary.
- **Complete:** Explicit `__fastcall`, `__stdcall`, `__thiscall`, `__vectorcall`, and `__cdecl` printer tokens are recognized.
- **Complete:** No convention is changed when the frontend emits no explicit token.
- **Partial:** The current provider adapter receives no architecture object from `AnalysisContext`; the supplied provider facts are x86-64 PE facts matching the existing fixtures. Other processor models require an architecture export before parity is possible.
- **Partial:** Ghidra's parallel convention command can test multiple compiler models and custom storage metadata. The current `AnalysisContext` exposes neither compiler-model candidates nor a signature-source/custom-storage field, so this port performs one native frontend analysis and records only the available signature change.

No ABI register heuristic is used as a replacement for frontend behavior.
