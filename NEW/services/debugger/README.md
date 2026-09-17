# Debugger Services

This directory groups debugger backends behind the reusable core debugger vocabulary.

- [`win_dbg_eng`](win_dbg_eng/README.md) contains the Windows DbgEng implementation and its tests.
- [`win_ttd`](win_ttd/README.md) contains the Windows TTD Replay implementation and its tests.
- [`CMakeLists.txt`](CMakeLists.txt) registers child debugger backends without changing the core contract layer.
- [`../CMakeLists.txt`](../CMakeLists.txt) adds this group to the service build, while [`../../core/contracts/debugger.cppm`](../../core/contracts/debugger.cppm) defines the backend-independent interface.

Future x64dbg, GDB, LLDB, or remote implementations should add sibling directories and implement the appropriate [`ILiveDebugSession`](../../core/contracts/debugger.cppm), [`IReplayDebugSession`](../../core/contracts/debugger.cppm), or shared [`IBaseDebugSession`](../../core/contracts/debugger.cppm) interface. They must not add backend types to `core/domain`.
