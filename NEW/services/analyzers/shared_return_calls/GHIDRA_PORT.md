# Ghidra Port Evidence

This document records the native port of **Shared Return Calls** and its
relationship to the shared analyzer model.

The native source is [`src/shared_return_calls.cppm`](src/shared_return_calls.cppm),
the target is [`CMakeLists.txt`](CMakeLists.txt), the wrapper is [`build.bat`](build.bat),
and the focused tests are [`tests/shared_return_calls_tests.cppm`](tests/shared_return_calls_tests.cppm).
Shared dependencies are documented in [`../shared/README.md`](../shared/README.md).

## Original Sources

- [`SharedReturnAnalyzer.java`](../../../../Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/function/SharedReturnAnalyzer.java)
  supplies the analyzer name, priority, default option behavior, destination
  function pass, and command invocation.
- [`SharedReturnJumpAnalyzer.java`](../../../../Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/function/SharedReturnJumpAnalyzer.java)
  supplies jump-source discovery and destination-function filtering.
- [`SharedReturnAnalysisCmd.java`](../../../../Ghidra/Features/Base/src/main/java/ghidra/app/cmd/analysis/SharedReturnAnalysisCmd.java)
  supplies `processFunctionJumpReferences()`, single-flow validation,
  `CALL_RETURN` application, caller repair, and contiguous-function scans.
- [`SetFlowOverrideCmd.java`](../../../../Ghidra/Features/Base/src/main/java/ghidra/app/cmd/disassemble/SetFlowOverrideCmd.java)
  is represented by `AnalysisContext::set_flow_override()`.

## Transferred Behavior

- Only jumps to an exact existing function entry are converted in the ordinary
  pass; function-entry jumps, same-function internal jumps, conditional jumps,
  and multi-flow instructions are rejected.
- The native shared model converts the jump reference to an unconditional call,
  sets `FlowOverride::call_return`, emits a flow event, and synchronously
  rebuilds the containing caller body.
- The default contiguous-function behavior considers only unconditional jumps
  that cross a known neighboring function boundary before creating a missing
  target function.
- Analysis is idempotent and scans references discovered through memory, code,
  reference, and function events.

## Differences and Risks

- The native `AnalysisOptions` currently exposes the analyzer enablement but not
  separate fields for `Assume Contiguous Functions Only` and `Allow Conditional
  Jumps`; this port preserves the Java defaults (contiguous enabled,
  conditional disabled).
- Cold-entry maps, overlays, symbol-table namespaces, and background-command
  transactions are not present in the shared model. The implementation rejects
  cross-mapped targets and uses synchronous event mutations instead.
- Function body repair is delegated to [`AnalysisContext`](../shared/src/analyzer_context.cppm),
  whose existing `CALL_RETURN` handling mirrors the listing CFG effects.

## Verification

[`tests/shared_return_calls_tests.cppm`](tests/shared_return_calls_tests.cppm)
hardcodes the fixture addresses and the expected one-row `CALL_RETURN` delta
from [`tests/data/test_shared_return_calls.md`](tests/data/test_shared_return_calls.md).
Tests never read Markdown at runtime and use the real PE fixture for provider
memory validation. PyGhidra scripts must use
[`TEST/run_ghidra_python.bat`](../../../../TEST/run_ghidra_python.bat).
