# Review Report: Call-Fixup Installer

- [x] Scope confirmed: [`src/call_fixup_installer.cppm`](src/call_fixup_installer.cppm), [`tests/call_fixup_installer_tests.cppm`](tests/call_fixup_installer_tests.cppm), fixture source/script/report, [`CMakeLists.txt`](CMakeLists.txt), [`build.bat`](build.bat), [`README.md`](README.md), and [`GHIDRA_PORT.md`](GHIDRA_PORT.md).
- [x] Original paths resolved and inspected: `Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/disassembler/CallFixupAnalyzer.java`, `CallFixupChangeAnalyzer.java`, and `Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/clear/ClearFlowAndRepairCmd.java`.
- [x] Reviewed diff: working tree at 2026-09-15; no commit supplied.
- [x] Reviewer: Kilo.
- [x] Source, test, fixture, documentation, CMake, and wrapper inspection completed.
- [x] Validation status recorded honestly: no build, CTest, formatter, or tidy command was run because this was a read-only audit.

## Findings

### Critical

No findings.

### High

#### CALLFIXUP-HIGH-001: No compiler-spec call-fixup is installed in the built-in analyzer

- [ ] Remediation status: open.
- Severity: high.
- Affected component: [`src/call_fixup_installer.cppm#L76-L107`](src/call_fixup_installer.cppm#L76-L107) and [`../analyzer_builtin.cpp#L45`](../analyzer_builtin.cpp#L45).
- Technical evidence: original `CallFixupAnalyzer.java#L86-L110` obtains the compiler-spec `PcodeInjectLibrary`, finds target mappings, and calls `function.setCallFixup(fixupName)`, then uses the payload's fall-through behavior. Native line 99 adds only a `SymbolRecord`; the built-in registration constructs `CallFixupInstallerAnalyzer` with its default empty rule vector. Native `Function` has no call-fixup field or injection payload.
- Expected behavior: compiler-spec target names resolve to actual function call-fixup state and injected p-code is available to later decompilation, with no-return/fall-through derived from the payload.
- Actual behavior: built-in analysis has no rules; manually supplied rules create an evidence symbol but never install a call-fixup payload.
- Impact: the primary analyzer behavior is absent in normal execution and decompiler output cannot observe the fixup.
- Root cause: no compiler-spec/PcodeInjectLibrary bridge exists and the replacement stores evidence instead of the function property.
- Recommended fix: expose compiler-spec target/payload data and a native function call-fixup field/injection boundary, then apply the original target and payload rules.
- Regression risks: call-fixup p-code changes decompiler control/data flow; compare compiler-spec fixtures and no-return payloads before enabling.
- Relevant tests or validation: [`tests/call_fixup_installer_tests.cppm`](tests/call_fixup_installer_tests.cppm#L15-L73) injects a rule manually and checks evidence, not payload application.

#### CALLFIXUP-HIGH-002: ClearFlowAndRepair behavior is reduced to a local CFG rebuild

- [ ] Remediation status: open.
- Severity: high.
- Affected component: [`src/call_fixup_installer.cppm#L57-L72`](src/call_fixup_installer.cppm#L57-L72), [`shared/src/analyzer_context.cppm#L808-L927`](../shared/src/analyzer_context.cppm#L808-L927), and original `Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/clear/ClearFlowAndRepairCmd.java`.
- Technical evidence: original `CallFixupAnalyzer.java#L256-L420` computes repaired call locations, protected locations, clear instruction/data sets, bad bookmarks, and invokes `ClearFlowAndRepairCmd`; the resolved command's `#L75-L247` clears code/data/symbols and `#L417-L621` repairs fall-throughs, disassembles destinations, starts analysis, and repairs/removes/recreates function bodies. Native code accepts `ReferenceKind::external` at lines 61-64, sets `CALL_RETURN`, and calls `rebuild_function_body`, whose implementation only follows already decoded instructions and rebuilds known blocks.
- Expected behavior: repair all affected flow, protect relevant locations, clear invalid instruction/data/bookmark state, re-disassemble and re-analyze destinations, and repair function bodies.
- Actual behavior: only existing native references and known instructions are recomputed; stale code/data/bookmark state is not cleared or redisassembled.
- Impact: callers of no-return/call-fixup functions can retain wrong fall-through instructions and bodies.
- Root cause: `rebuild_function_body` is not an implementation of `ClearFlowAndRepairCmd` and no clear/disassembler command boundary is connected.
- Recommended fix: port the clear/repair command or expose an equivalent transaction that preserves protected locations, code/data clearing, bookmark cleanup, re-disassembly, and function repair.
- Regression risks: flow repair can remove code and trigger downstream analyzers; use original fall-through, offcut, protected, data-reference, and cancellation cases.
- Relevant tests or validation: shared CFG tests cover only native `set_flow_override`/rebuild mechanics, not the original clear command.

### Medium

#### CALLFIXUP-MEDIUM-001: CallFixupAnalyzer and CallFixupChangeAnalyzer lifecycle contracts are merged incorrectly

- [ ] Remediation status: open.
- Severity: medium.
- Affected component: [`src/call_fixup_installer.cppm#L79-L82`](src/call_fixup_installer.cppm#L79-L82).
- Technical evidence: original `CallFixupAnalyzer.java#L43-L52` is a `FUNCTION_ANALYZER`, default-enabled, priority `DISASSEMBLY.after()`, and supports one-time analysis. Original `CallFixupChangeAnalyzer.java#L21-L28` subclasses it as `FUNCTION_MODIFIERS_ANALYZER` with `supportsOneTimeAnalysis=false`. Native exposes one descriptor with `function_added` and `function_changed` events and no separate lifecycle.
- Expected behavior: initial call-fixup installation and later function-modifier reanalysis have distinct analyzer registration semantics.
- Actual behavior: one native analyzer is used for both event categories and does not express one-time/reanalysis distinction.
- Impact: changes can be skipped or repeated at the wrong phase, especially after compiler-spec/function updates.
- Root cause: two original classes were collapsed into one event descriptor.
- Recommended fix: preserve two registrations or add explicit lifecycle metadata matching both original classes.
- Regression risks: scheduler order and duplicate fixup application can change; add event/reanalysis tests.
- Relevant tests or validation: the native descriptor test checks only name/priority.

#### CALLFIXUP-MEDIUM-002: Fixture and native tests validate different behaviors

- [ ] Remediation status: open.
- Severity: medium.
- Affected component: [`tests/call_fixup_installer_tests.cppm`](tests/call_fixup_installer_tests.cppm), fixture script/report, and [`CMakeLists.txt#L6-L13`](CMakeLists.txt#L6-L13).
- Technical evidence: the fixture report is generated by original PyGhidra and observes a real function call-fixup. Native tests construct `CallFixupRule` values and inspect `SymbolRecord` state; CMake does not invoke the fixture script.
- Expected behavior: native tests must assert actual function call-fixup/injection state and repaired flow, while the original report remains an oracle.
- Actual behavior: a passing native test is compatible with no actual call-fixup support.
- Impact: test validity is limited to rule matching and evidence bookkeeping.
- Root cause: the required compiler-spec and clear/repair APIs are absent from the native test boundary.
- Recommended fix: add compiler-spec and repair fixtures after the missing model boundaries are implemented.
- Regression risks: injected p-code can affect body formation; assert both pre/post flow and function state.
- Relevant tests or validation: no commands were run during this audit.

### Low

No findings.

## Verified Strengths

- [x] Target spelling variants and `libID_conflict_` normalization are explicit.
- [x] Cancellation, no-return state, flow override, and body-rebuild calls are visible in the native approximation.

## Reviewed Areas With No Findings

- [x] Exact moved original paths were located with glob/grep and used in this review.
- [x] No implementation, test, fixture, documentation, or build source was changed.

## Validation Results

- [x] Read-only original/native comparison completed.
- [x] Read-only test, fixture, documentation, CMake, and wrapper inspection completed.
- [ ] Focused build/test: not run by request.
- [ ] `format.bat call_fixup_installer`: not run by request.
- [ ] `tidy.bat call_fixup_installer --check`: not run by request.

## Unresolved Questions

- [ ] The native compiler-spec injection provider and complete `ClearFlowAndRepairCmd` equivalent are not exposed to this analyzer.

## Residual Risks

- [ ] The current broad `external` reference acceptance may change which callers are repaired compared with original `RefType.isCall()` filtering.

## Follow-Up

- [ ] Highest proposed remediations: CALLFIXUP-HIGH-001 and CALLFIXUP-HIGH-002.
- [ ] Medium proposed remediations: CALLFIXUP-MEDIUM-001 and CALLFIXUP-MEDIUM-002.
- [ ] Final follow-up decision: keep implementation unchanged until compiler-spec and clear/repair parity is authorized.
