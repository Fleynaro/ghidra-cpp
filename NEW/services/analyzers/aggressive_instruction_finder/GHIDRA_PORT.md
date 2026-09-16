# Ghidra Port Evidence

This document records the native port of **Aggressive Instruction Finder** and
its ARM variant.

The native source is [`src/aggressive_instruction_finder.cppm`](src/aggressive_instruction_finder.cppm),
the target is [`CMakeLists.txt`](CMakeLists.txt), the wrapper is [`build.bat`](build.bat),
and the focused fixture tests are [`tests/aggressive_instruction_finder_tests.cppm`](tests/aggressive_instruction_finder_tests.cppm).
Shared dependencies are documented in [`../shared/README.md`](../shared/README.md).

## Original Sources

- [`AggressiveInstructionFinderAnalyzer.java`](../../../../Ghidra/Features/Base/src/main/java/ghidra/app/plugin/prototype/analysis/AggressiveInstructionFinderAnalyzer.java)
  supplies the twenty-function guard, masked two-instruction frequency map,
  executable-block restriction, pseudo-subroutine validation, 4000-instruction
  bound, body/data check, disassembly, bookmark, and repeated one-time pass.
- [`ArmAggressiveInstructionFinderAnalyzer.java`](../../../../Ghidra/Features/Base/src/main/java/ghidra/app/plugin/prototype/analysis/ArmAggressiveInstructionFinderAnalyzer.java)
  supplies ARM/Thumb mode trials, filler-instruction rejection, duplicate-flow
  protection, valid terminator checks, body-distance checks, and repair behavior.
- [`PseudoDisassembler.java`](../../../../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/app/util/PseudoDisassembler.java)
  and [`SleighDebugLogger.java`](../../../../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/app/plugin/processors/sleigh/SleighDebugLogger.java)
  supply the original validity and instruction-mask concepts.

## Transferred Behavior

- The Java minimum of twenty known functions is enforced before analysis.
- Known start keys use exact Sleigh instruction bytes masked by the provider's
  instruction mask; candidates require a key observed at least four times.
- Candidates are scanned only in executable mapped regions and only while the
  corresponding native address is undefined.
- Pseudo-flow requires valid decoded instructions, rejects out-of-memory flow,
  rejects ARM filler/repeated loops, requires more than two instructions, and
  requires a call or jump-to-known-code evidence unless the start is highly
  repeated. Defined data intersecting the body is rejected.
- Accepted candidates are disassembled through `AnalysisContext` and receive
  the exact `Aggressive Instruction Finder` / `Found code` bookmark.

## Differences and Risks

- The native provider currently exposes no ARM `TMode` register context or
  `Processor` identity API. ARM-specific filler and duplicate safeguards are
  preserved for non-x86 decoded instructions, but ARM/Thumb mode trial and
  `ClearFlowAndRepairCmd` error-bookmark cleanup remain pending.
- Java uses a per-run hash cache and schedules one-time analysis for remaining
  undefined ranges. The native event manager re-runs idempotently after emitted
  code events, so no separate scheduler object is required.
- Native `AnalysisContext` deliberately does not create a function for an
  accepted candidate, matching the generic Java analyzer's disassembly-only
  effect.

## Verification

[`tests/aggressive_instruction_finder_tests.cppm`](tests/aggressive_instruction_finder_tests.cppm)
uses the real fixture and hardcodes the `0x140001300` bookmark from
[`tests/data/test_aggressive_instruction_finder.md`](tests/data/test_aggressive_instruction_finder.md).
It never reads Markdown at runtime. PyGhidra generation uses the required
[`TEST/run_ghidra_python.bat`](../../../../TEST/run_ghidra_python.bat) wrapper.
