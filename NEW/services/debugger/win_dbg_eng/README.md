# WinDbgEng Debugger Service

This directory contains the Windows DbgEng implementation of the generic debugger contract.

- [`win_dbg_eng.cppm`](win_dbg_eng.cppm) owns the DbgEng client, engine-thread command queue, callback translation, target queries, execution operations, and breakpoint/watchpoint mapping.
- [`tests`](tests) contains generic-contract integration tests and the symbolized MSVC debuggee under [`tests/data`](tests/data).
- [`CMakeLists.txt`](CMakeLists.txt) builds the backend as `NewGhidra::WinDbgEng` and links the Windows-provided `dbgeng` library only for MSVC builds.
- The parent [`../CMakeLists.txt`](../CMakeLists.txt) registers this backend with the service graph; [`../../CMakeLists.txt`](../../CMakeLists.txt) aggregates it with the other services.
- The backend depends only on [`../../../core`](../../../core/README.md) and does not depend on `NEW/runtime`.

`core/contracts/debugger.cppm` and `core/domain/debugger.cppm` are backend-independent. This service is the adapter that maps DbgEng's `IDebugClient`, `IDebugControl`, `IDebugRegisters`, `IDebugDataSpaces`, `IDebugSymbols`, `IDebugSystemObjects`, and callbacks to those values.
