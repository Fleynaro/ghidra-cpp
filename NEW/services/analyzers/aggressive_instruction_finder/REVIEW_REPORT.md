# Strict Audit Report

## Scope
- [x] Reviewed `src/aggressive_instruction_finder.cppm`, its GoogleTest module, fixture source, generated report, CMake, and build wrapper.
- [x] Compared against `Ghidra/Features/Base/src/main/java/ghidra/app/plugin/prototype/analysis/AggressiveInstructionFinderAnalyzer.java` and `PseudoDisassembler.java`.
- [x] Baseline and focused tests were executed on 2026-09-15.

## Critical
No findings.

## High
- [x] **AGG-HIGH-001:** Terminal-flow validation rejected valid `RET` candidates before terminal handling. Fixed in `src/aggressive_instruction_finder.cppm`; the regression fixture now passes.
- [ ] **AGG-HIGH-002:** ARM-specific `ArmAggressiveInstructionFinderAnalyzer` behavior is absent; the native module only covers the generic analyzer.
- [ ] **AGG-HIGH-003:** Native scheduling still scans all executable bytes and lacks Java residual-set one-time scheduling.

## Medium
- [ ] **AGG-MEDIUM-001:** Native signature keys use mnemonic/length shape instead of Sleigh masked bytes and disassembly context.
- [ ] **AGG-MEDIUM-002:** Test coverage lacks exact threshold, invalid candidate, existing-bookmark, and reanalysis cases.

## Low
No findings.

## Validation
- [x] `build.bat aggressive_instruction_finder` passes after the verified terminal-flow fix.
- [x] No test reads Markdown at runtime.
- [ ] Full 1:1 parity is not established because the ARM and scheduler contracts remain incomplete.

## Follow-up
- [x] Confirmed baseline failure was an analyzer bug, not a bad expectation.
- [ ] Highest-priority remaining remediation: AGG-HIGH-002 and AGG-HIGH-003.
