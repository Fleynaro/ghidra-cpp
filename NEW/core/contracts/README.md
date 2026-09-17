# Core Contracts

The contract modules define service, provider, operation, command, persistence, projection, and event-bus boundaries.

- [`contracts.cppm`](contracts.cppm) is the public aggregate module.
- [`operation.cppm`](operation.cppm) defines cancellation, operation ownership, priorities, and task results.
- [`project_query.cppm`](project_query.cppm) is the revision-stamped read-only project view.
- [`pcode_decoder.cppm`](pcode_decoder.cppm), [`pe_loader.cppm`](pe_loader.cppm), [`decompiler.cppm`](decompiler.cppm), and [`function_id.cppm`](function_id.cppm) define feature services.
- [`command.cppm`](command.cppm), [`event_store.cppm`](event_store.cppm), [`projection.cppm`](projection.cppm), and [`event_bus.cppm`](event_bus.cppm) define runtime infrastructure boundaries without naming SQLite or native Ghidra classes.
- [`debugger.cppm`](debugger.cppm) defines backend-independent session, execution, process-selection, thread, register, memory-transfer, stack, module, breakpoint, watchpoint, and event operations. Waiting operations return the established [`Task`](operation.cppm) type.
- [`debugger.cppm`](debugger.cppm) separates [`IBaseDebugSession`](debugger.cppm) inspection from [`ILiveDebugSession`](debugger.cppm) lifecycle/mutation and [`IReplayDebugSession`](debugger.cppm) trace navigation; `IDebugSession` remains a live compatibility alias.
- [`trace_recorder.cppm`](trace_recorder.cppm) defines the independent [`ITraceRecorder`](trace_recorder.cppm) recording boundary.

Implementations live under [`../../services`](../../services) and [`../../runtime`](../../runtime); this layer remains independent of those concrete components.
