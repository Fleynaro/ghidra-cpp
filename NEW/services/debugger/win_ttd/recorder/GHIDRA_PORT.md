# Ghidra Port Evidence

This service ports the trace-acquisition boundary to a native Windows TTD process. The generic vocabulary comes from [`core/domain/trace_recording.cppm`](../../../../core/domain/trace_recording.cppm) and [`core/contracts/trace_recorder.cppm`](../../../../core/contracts/trace_recorder.cppm). The process invocation follows Microsoft's TTD command-line contract: `-noUI -out <trace> -accepteula -launch <target command line>`.

The implementation is complete for synchronous process completion, output-path reporting, nonzero-exit diagnostics, cooperative operation cancellation, explicit `cancel()` termination, UTF-16 environment-block propagation, and the documented `record_children`/`-children` switch. Unknown custom options are rejected explicitly. It is intentionally Windows-only; non-Windows calls return `DiagnosticCode::unsupported`. It is grouped under the unified [`../README.md`](../README.md) Windows TTD service; the portable recorder contract remains independent.

Behavior is covered by [`tests/win_ttd_tests.cppm`](tests/win_ttd_tests.cppm) and built by [`CMakeLists.txt`](CMakeLists.txt). The opt-in integration test has invoked the installed `ttd.exe` against the existing multi-thread debugger fixture and verified that a real `.run` artifact was produced for the replay integration test.
