# Ghidra Port Evidence

## Scope

The implementation is a C++23 DbgEng service, not a copy of the original Python agent. Behavior was mapped from:

- [`Ghidra/Debug/Debugger-agent-dbgeng/src/main/py/src/ghidradbg/util.py`](../../../../Ghidra/Debug/Debugger-agent-dbgeng/src/main/py/src/ghidradbg/util.py), especially the dedicated `DbgWorker` and same-thread dispatch rules.
- [`Ghidra/Debug/Debugger-agent-dbgeng/src/main/py/src/ghidradbg/commands.py`](../../../../Ghidra/Debug/Debugger-agent-dbgeng/src/main/py/src/ghidradbg/commands.py), for process/thread/register/memory/stack/module and breakpoint behavior.
- [`Ghidra/Debug/Debugger-agent-dbgeng/src/main/py/src/ghidradbg/hooks.py`](../../../../Ghidra/Debug/Debugger-agent-dbgeng/src/main/py/src/ghidradbg/hooks.py), for callback capture and event ordering.
- Microsoft documentation for [`WaitForEvent`](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/dbgeng/nf-dbgeng-idebugcontrol-waitforevent), [`SetExecutionStatus`](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/dbgeng/nf-dbgeng-idebugcontrol-setexecutionstatus), callback objects, and `IDebugBreakpoint`.

## Design Mapping

- **Complete:** native interface ownership is isolated to [`win_dbg_eng.cppm`](win_dbg_eng.cppm), with a dedicated `std::jthread`; callers enqueue commands and never call DbgEng directly.
- **Complete:** execution commands set execution status and are completed only after the engine thread returns from bounded `WaitForEvent` calls. The callback methods capture immutable data and return promptly.
- **Complete:** the active wait has a configurable watchdog (`SessionOptions::operation_timeout`); the documented cross-thread `SetInterrupt` path wakes a blocked wait for pause, cancellation/shutdown, or timeout, while all other native calls remain on the engine thread.
- **Complete:** only one execution waiter is admitted at a time, conflicting commands return a generic conflict error, and shutdown resolves queued/pending tasks before releasing native interfaces.
- **Complete:** process/thread identity, register values, virtual memory, memory mappings, stack frames, modules, code breakpoints, data breakpoints, exceptions, and transient generic events are translated without native types crossing the contract.
- **Intentionally different:** Ghidra's trace model has richer target-object and snapshot metadata. The current core contract returns immutable snapshots and a transient ordered event queue; persistence remains outside the debugger service.
- **Intentionally different:** DbgEng has no universal step-out execution status. `step_out` installs a temporary breakpoint at the caller frame's return address and resumes until that breakpoint.
- **Partial:** module symbol/source metadata is best-effort because DbgEng can report unloaded or unresolved symbols. Consumers receive empty optional text rather than a fabricated name.
- **Intentionally different:** DbgEng's data-breakpoint API has no execute-only data-access mode, so the backend rejects `WatchpointAccess::execute` rather than silently treating it as a read/write watchpoint. `BreakpointKind::hardware` is accepted as a semantic code breakpoint request but uses DbgEng's code-breakpoint mechanism.
- **Partial:** the current service exposes a single active process context per session. DbgEng process enumeration is retained, but cross-process switching is not yet a first-class contract operation.
- **Pending:** remote DbgEng process-server connection options and richer register writes require additional generic request values; no local-only type was added to the core contract to preclude those future backends.

## Testing

[`tests/debugger_contract_tests.cppm`](tests/debugger_contract_tests.cppm) imports the generic contract and uses only the backend factory to construct a session. It does not include `dbgeng.h` or mention native DbgEng constants. [`tests/data/debugger_debuggee.cpp`](tests/data/debugger_debuggee.cpp) is compiled by [`tests/data/build.bat`](tests/data/build.bat) with `/Zi`, `/Od`, and `/DEBUG:FULL` and contains nested calls, loops, heap/global state, three synchronized workers, deterministic memory writes, and a controlled exception path.
