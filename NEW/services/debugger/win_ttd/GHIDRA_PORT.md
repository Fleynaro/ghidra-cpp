# Ghidra Port Evidence

## Scope

The service ports the standalone Microsoft Replay API boundary, not WinDbg's live debugger. The required package headers/import library are pinned under [`dependencies/Microsoft.TimeTravelDebugging.Apis.0.9.5`](dependencies/Microsoft.TimeTravelDebugging.Apis.0.9.5), while the official API behavior is represented by the copied runtime/bootstrap workflow in [`dependencies`](dependencies) and the checked-in Microsoft API call mappings in this module.

The original Microsoft sample call sites studied for the port remain available under [`TEST/debugger/TTD/ReplayApi`](../../../../TEST/debugger/TTD/ReplayApi); they are research references only, not build or runtime dependencies.

## Mapping

- `MakeReplayEngine` and `Initialize` map to `IReplayDebugSession::open_trace`.
- `NewCursor`, `SetPosition`, `ReplayForward`, and `ReplayBackward` map to the replay position and step methods.
- `QueryMemoryBuffer`, `GetCrossPlatformContext`, `GetThreadList`, and `GetModuleList` map to portable snapshots.
- `AddMemoryWatchpoint` is exposed as a one-shot seek operation, because replay watchpoints are timeline queries rather than persistent live breakpoints.

## Status and limitations

- **Real implemented and tested:** trace lifecycle, opaque positions, forward/backward stepping, memory reads, x64 scalar register reads, active thread snapshots, module snapshots, process metadata, and one-shot memory watchpoint navigation. The native path was compiled against `Microsoft.TimeTravelDebugging.Apis` 0.9.5, loaded `TTDReplay.dll`/`TTDReplayCPU.dll`, opened a `.run` recorded from the multi-thread debugger fixture, and passed contract assertions for modules, threads, registers, memory, positions, seeking, and both replay directions.
- **Partial:** register descriptors currently cover x64 scalar registers; x86, ARM, ARM64, vector register enumeration, and symbolized stack walking require additional architecture adapters.
- **Intentionally unsupported:** process launch/attach, memory writes, live breakpoints, and name-based symbol resolution. These are live-only or require a separate symbol provider.
- **Fallback only:** when the optional API package is not discoverable, CMake can still build a portability fallback that returns a clear `unsupported` diagnostic. `NEW\build.bat ttd_replay` sets `NEW_GHIDRA_REQUIRE_TTD_REPLAY=ON` and refuses this fallback; ordinary aggregate builds may retain it for machines without the Microsoft SDK.

## Future instruction profiling

The API's memory watchpoint callback with `DataAccessMask::Execute`, together with replay segment continuity and progress callbacks documented in [`TraceAnalysis.cpp`](../../../../TEST/debugger/TTD/ReplayApi/TraceAnalysis/TraceAnalysis.cpp), is the extension point for collecting executed instruction addresses and reducing them into basic blocks, edges, and functions. The session deliberately keeps cursor ownership private so a future analysis service can add a dedicated cursor rather than coupling profiling to debugger state.
