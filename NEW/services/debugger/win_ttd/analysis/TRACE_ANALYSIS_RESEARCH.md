# Microsoft TTD Trace Analysis Research

## Scope And Sources

This report was written before implementing the function-call miner. The primary source is the checked-in Microsoft sample [`TEST/debugger/TTD/ReplayApi/TraceAnalysis/TraceAnalysis.cpp`](../../../../../TEST/debugger/TTD/ReplayApi/TraceAnalysis/TraceAnalysis.cpp). The installed Microsoft.TimeTravelDebugging.Apis 0.9.5 headers used for the native build are under [`../dependencies/Microsoft.TimeTravelDebugging.Apis.0.9.5/sdk/include/TTD`](../dependencies/Microsoft.TimeTravelDebugging.Apis.0.9.5/sdk/include/TTD). Additional verified API call sites are [`TEST/debugger/TTD/ReplayApi/TraceDebugger/TraceDebugger.cpp`](../../../../../TEST/debugger/TTD/ReplayApi/TraceDebugger/TraceDebugger.cpp), [`TEST/debugger/TTD/ReplayApi/inc/ReplayHelpers.h`](../../../../../TEST/debugger/TTD/ReplayApi/inc/ReplayHelpers.h), and [`TEST/debugger/TTD/docs/Concepts.md`](../../../../../TEST/debugger/TTD/docs/Concepts.md).

The checked-in Microsoft sample is authoritative where generated API documentation disagrees with the 0.9.5 headers. In particular, the samples use `ReplayForward`/`ReplayBackward`, `PositionRange::Min`/`Max`, and `MemoryWatchpointData{Address, Size, AccessMask}`.

## What A Replay Segment Is

The 0.9.5 public API does **not** expose a `ReplaySegment` class, list, iterator, or a public segment-analysis object. Replay segments are scheduler-owned contiguous execution fragments. The API exposes their boundaries indirectly:

- `ICursorView::ThreadContinuityBreakCallback` (`IReplayEngine.h:1655-1660`) fires when a segment/thread continuity boundary is reached.
- `ICursorView::ReplayProgressCallback` (`IReplayEngine.h:1647-1653`) runs on the caller's replay thread when all segments before a position have completed.
- `ReplayFlags::ReplaySegmentsSequentially` exists (`IReplayEngine.h:926-935`), but the default replay scheduler can execute independent segments in parallel and the analysis sample intentionally relies on that parallelism.
- `IThreadView` is the safe callback view. It provides thread-local position, thread identity, program counter, register context, and memory queries (`IReplayEngine.h:1490-1528`). The callback must not access the mutable `ICursorView`.

Therefore “Replay Segment analysis” means segment-local callback collection plus continuity/progress synchronization, not manually iterating a segment object.

## Microsoft Analysis Pattern

`TraceAnalysis.cpp` implements whole-trace code coverage as follows:

1. Create one `UniqueCursor` from `IReplayEngineView::NewCursor` (`341-347`). The engine/cursor use RAII and the cursor must die before the engine.
2. Register `ThreadContinuityCallback` (`354-355`) to flush state at each segment boundary.
3. Register `ReplayProgressCallback` (`361-363`) to consume completed segment data at a scheduler barrier. The sample optionally dispatches merge work with `std::async` (`311-339`) while retaining only one pending merge.
4. Set `ReplayFlags::ReplayAllSegmentsWithoutFiltering` (`368`) so the entire trace is analyzed.
5. Add an execute memory watchpoint spanning the guest address space and set `EventMask::MemoryWatchpoint` (`370-380`). The high-frequency callback receives `MemoryWatchpointResult` and an `IThreadView`.
6. In the hot callback, append minimal data to `thread_local SegmentGatheredData` (`43-49`, `93-143`). No global lock or global counter is used in the hot path.
7. At continuity break, exchange/reset TLS state, compress it, and enqueue it under one short mutex (`146-175`).
8. At progress barriers, drain completed segments, merge them in position order, and keep the scheduler from getting too far ahead (`185-309`). A final progress merge is forced after replay (`392-400`).

The sample explicitly reports that reducing allocations and merging ranges when a segment buffer fills can halve runtime (`130-138`). This is the required scalability model for large traces.

## Callback And Speculation Rules

Replay is multithreaded and speculative (`TEST/debugger/TTD/docs/Concepts.md:134-140`). A callback may observe speculative execution that is later abandoned, and an event can be observed again after replay resumes. Callback code must not call or mutate `ICursorView`; use the supplied `IThreadView` for thread-local memory/register/context queries. The progress callback is synchronous with the scheduler and the scheduler waits for it (`TraceAnalysis.cpp:311-320`), so heavy merging should be throttled or moved off-thread while preserving ordered final aggregation.

## Function Call Mining Mapping

The first miner will reuse the same one-pass execute-watchpoint/segment pipeline, but it will count **function-entry execution hits** rather than decode every call instruction. This is the robust bulk-analysis interpretation for the first pass: an invocation is counted when replay executes the catalogued function entry address. It handles direct calls and callback/function-pointer entries uniformly, including the worker entry used by the fixture.

- `MemoryWatchpointResult.Address` identifies the executed instruction address for the execute watchpoint.
- The analyzer resolves that address against loaded module base/size metadata and a caller-supplied module-relative function catalog. The identity is `(module name, relative entry address)`, not a PDB string or an absolute ASLR address.
- The hot callback performs one catalog lookup and increments a per-segment map keyed by stable function identity. It does not query a cursor, decode a whole instruction stream, or run one query per function.
- The continuity callback moves that map into a completed-segment queue. The progress callback merges maps into the final result.

This deliberately counts entry executions, so it includes callback/function-pointer invocations and the initial program entry when it is present in the catalog. It does not count returns, runtime/library entries absent from the catalog, or arbitrary symbol names. Tail calls are counted as entry executions of the destination, which is the documented semantic boundary for this first pass. An executed address not found in the catalog is ignored and can be reported in an unresolved counter without becoming a fake function.

## Why This Is The Correct Approach

- `TTD.Calls("foo").Count()` performs a separate data-model query per function and cannot provide one scalable pass over an evolving catalog.
- Repeated `IReplayDebugSession::step_forward()` would interpret every instruction through the interactive cursor, serialize execution, and violate the Replay Segment scheduler model.
- One watchpoint/query per function multiplies replay work and creates coordination overhead proportional to the number of functions.
- TLS segment-local aggregation keeps the hot callback lock-free; only segment completion and progress merging synchronize. This matches Microsoft's `TraceAnalysis.cpp` design and remains extensible to future basic-block/edge metrics without changing the interactive debugger API.

## Limitations And Follow-Up

- The first concrete adapter is x64 because the current Windows build/test toolchain and fixture are x64. The portable contract does not contain x64 or TTD types; future architecture decoders can be added behind the service.
- Direct-call decoding is intentionally conservative. Unsupported encodings are unresolved, never guessed.
- The `.idx` companion is an engine-managed acceleration artifact. When present, `Initialize` loads it; when absent, the Replay Engine can still replay the trace and owns any index construction. The analyzer receives the trace path and never parses `.idx` itself. The bulk callback model remains unchanged for either indexed or freshly indexed traces.
- Function call counts are aggregate and deterministic for the auto-exit fixture; cross-thread ordering is not part of the result.
- A future basic-block miner should add another analysis request/result variant and reuse the same segment-local aggregation skeleton, not add stepping to `IReplayDebugSession`.

## Measured Performance Sample

On the x64 Windows development machine, the real auto-exit fixture produced these measurements in the dedicated tests (the recorder and analysis are intentionally measured separately):

- Recorder test wall time: approximately 0.4-0.7 seconds for the small fixture.
- Analysis wall time: 375 ms.
- Replay API trace-open time: 29.4 ms.
- Bulk segment replay time: 341.8 ms.
- Result aggregation time: 0.013 ms.

These numbers are a small-fixture baseline, not a GTA V extrapolation. The important scalability property is that replay executes scheduler segments in parallel and the hot callback performs only TLS/local-map work; the analyzer never invokes interactive `step_forward()`.
