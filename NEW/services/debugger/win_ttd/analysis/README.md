# TTD Trace Analysis

This directory contains offline bulk-analysis adapters for the Windows TTD service. It is separate from interactive replay cursor navigation and is driven by [`../win_ttd.cppm`](../win_ttd.cppm) only through private native engine ownership.

- [`TRACE_ANALYSIS_RESEARCH.md`](TRACE_ANALYSIS_RESEARCH.md) records the verified Microsoft Replay API mechanism and its constraints.
- [`function_call_analyzer.cppm`](function_call_analyzer.cppm) implements one bulk function-entry statistics pass and exposes only the platform-independent [`../../../../core/contracts/trace_analysis.cppm`](../../../../core/contracts/trace_analysis.cppm) contract.
- [`tests`](tests) contains real `.run` analysis assertions and separate open/replay/aggregation timing output. The recording fixture is local to this service at [`tests/data`](../tests/data), so TTD analysis does not depend on the live DbgEng service tree.
- Tests use the unified recorder under [`../recorder`](../recorder), the existing multi-thread debuggee, and real `.run`/`.idx` artifacts prepared by `build.bat ttd_all`.
