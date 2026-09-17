# Windows TTD Recorder

`win_ttd.cppm` implements [`ITraceRecorder`](../../../../core/contracts/trace_recorder.cppm) with Microsoft's `ttd.exe`. The `create_win_ttd_recorder()` factory returns the contract type. It resolves `TTD_EXE` first and otherwise relies on `ttd.exe` on `PATH`, launches with `CreateProcessW`, propagates the requested UTF-16 environment policy, and emits `RecordingResult` diagnostics for launch, exit, missing-artifact, and cancellation outcomes.

The focused tests are in [`tests`](tests), with registration in [`tests/CMakeLists.txt`](tests/CMakeLists.txt). They cover command construction, invalid requests, unsupported options, environment blocks, factory construction, and opt-in real recording/cancellation. The service target and `ReCode::TtdRecorder` alias are defined in [`CMakeLists.txt`](CMakeLists.txt); parent integration is in [`../CMakeLists.txt`](../CMakeLists.txt).

TTD is intentionally unsupported on non-Windows builds. The recorder does not modify the debugger contract or replay service.

`TTD_EXE` is optional: when set, it selects the recorder executable; otherwise the service resolves `ttd.exe` through the normal Windows `PATH`.

TTD recording may require an elevated Windows terminal. The service does not silently elevate; an elevation-required launch returns a remediation-specific diagnostic.

Use [`../setup_dependencies.bat`](../setup_dependencies.bat) to restore the shared Microsoft API package and runtime staging before running the unified [`../README.md`](../README.md) replay/recording workflow.

The real recorder test is opt-in with `TTD_TEST_PROGRAM=<absolute executable>`. For the existing multi-thread debuggee, set `TTD_TEST_AUTO_EXIT=1`; set `TTD_KEEP_TRACE=1` to retain its temporary artifact, then pass that path as `TTD_TEST_TRACE=<finalized .run>` for the replay integration test in the sibling service. These variables are test-only inputs and are never compiled into the service.
