# Shared Return Calls

Ports the destination-function and jump-reference phases of
[`SharedReturnAnalyzer.java`](../../../../Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/function/SharedReturnAnalyzer.java),
[`SharedReturnJumpAnalyzer.java`](../../../../Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/function/SharedReturnJumpAnalyzer.java),
and [`SharedReturnAnalysisCmd.java`](../../../../Ghidra/Features/Base/src/main/java/ghidra/app/cmd/analysis/SharedReturnAnalysisCmd.java).

The analyzer accepts only direct unconditional jump references to existing
functions, rejects function-entry and internal jumps, requires a single flow,
and applies the shared model's `CALL_RETURN` override plus synchronous caller
body repair. It also contains the conservative contiguous-function target
discovery rule from the command. Conditional branches remain disabled by the
Java default.

Implementation: [`src/shared_return_calls.cppm`](src/shared_return_calls.cppm)
Build/tests: [`CMakeLists.txt`](CMakeLists.txt)
Build wrapper: [`build.bat`](build.bat)
Focused tests: [`tests/shared_return_calls_tests.cppm`](tests/shared_return_calls_tests.cppm)
Fixture evidence: [`tests/data/`](tests/data/)
Shared parent APIs: [`../shared/`](../shared/)
Port evidence: [`GHIDRA_PORT.md`](GHIDRA_PORT.md)
