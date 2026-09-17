# Windows TTD Replay Service

This module adapts Microsoft's standalone TTD Replay API to the portable replay contract.

- [`win_ttd.cppm`](win_ttd.cppm) owns `IReplayEngine`, `ICursor`, native register contexts, cursor navigation, memory queries, thread/module snapshots, and watchpoint translation.
- [`tests`](tests) validates the factory, lifecycle errors, and real `.run` replay when `TTD_TEST_TRACE` and the Microsoft runtime are available.
- [`recorder`](recorder) contains the Windows `ITraceRecorder` adapter and its focused tests; it shares this service's TTD dependency/bootstrap boundary while keeping recording separate from replay contracts.
- [`CMakeLists.txt`](CMakeLists.txt) optionally discovers `Microsoft.TimeTravelDebugging.Apis`; the service remains buildable without the package and reports a structured unsupported diagnostic.
- [`setup_dependencies.bat`](setup_dependencies.bat) is the provisioning implementation; the recommended single entry point is `NEW\build.bat ttd_setup`, which invokes it through the CMake target `new_ghidra_win_ttd_setup`.
- `TTD_APIS_PACKAGE_DIR` selects the restored NuGet API package and `TTD_RUNTIME_DIR` optionally supplies `TTDReplay.dll` and `TTDReplayCPU.dll` for test/runtime output.
- [`../../../core/contracts/debugger.cppm`](../../../core/contracts/debugger.cppm) defines `IReplayDebugSession` and the shared `IBaseDebugSession` boundary.
- [`dependencies`](dependencies) contains the pinned API manifest, copied CMake package/import library inputs, and runtime staging script owned by this service.

Replay is read-only with respect to the recorded process. Cursor position and replay stepping are mutable session state, while memory, registers, modules, and threads are snapshots at that position.

The real backend has been compiled and exercised against the pinned Microsoft.TimeTravelDebugging.Apis 0.9.5 inputs and runtime DLLs owned by this service. Run `NEW\build.bat ttd_setup` to restore/update dependencies, then `NEW\build.bat ttd_replay` to require the real backend for an already prepared trace. For a complete fresh-checkout workflow, use `NEW\build.bat ttd_all`; it provisions missing dependencies, builds the multi-thread debuggee, records a temporary `.run`, and runs the full CTest suite through the real recorder and Replay API. `TTD_APIS_PACKAGE_DIR` and `TTD_RUNTIME_DIR` remain explicit overrides for external installations.

To reproduce the dependency setup without any external repository path:

```powershell
NEW\build.bat ttd_setup
$env:TTD_APIS_PACKAGE_DIR = (Resolve-Path NEW\services\debugger\win_ttd\dependencies\Microsoft.TimeTravelDebugging.Apis.0.9.5).Path
$env:TTD_RUNTIME_DIR = (Resolve-Path NEW\services\debugger\win_ttd\dependencies\runtime\x64).Path
NEW\build.bat ttd_replay
```

The normal `NEW\build.bat all` remains the non-recording full build. `ttd_all` is intentionally explicit because it launches `ttd.exe` and records a temporary process execution.
