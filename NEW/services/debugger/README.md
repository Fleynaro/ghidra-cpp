# Debugger Services

This directory groups debugger backends behind the reusable core debugger vocabulary.

- [`win_dbg_eng`](win_dbg_eng/README.md) contains the Windows DbgEng implementation and its tests.
- [`CMakeLists.txt`](CMakeLists.txt) registers child debugger backends without changing the core contract layer.
- [`../CMakeLists.txt`](../CMakeLists.txt) adds this group to the service build, while [`../../core/contracts/debugger.cppm`](../../core/contracts/debugger.cppm) defines the backend-independent interface.

Future x64dbg, GDB, LLDB, or remote implementations should add sibling directories and implement the same [`IDebugger`](../../core/contracts/debugger.cppm) and [`IDebugSession`](../../core/contracts/debugger.cppm) interfaces. They must not add backend types to `core/domain`.
