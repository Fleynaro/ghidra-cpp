# Ghidra Port Evidence

This document records the port of Ghidra's Function Analyzer, whose user-facing
name is **Subroutine References**. It complements [`README.md`](README.md) and
the implementation in [`src/subroutine_references.cppm`](src/subroutine_references.cppm).

## Original Sources

- [`Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/function/FunctionAnalyzer.java`](../../../../Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/function/FunctionAnalyzer.java)
  supplies `added()`, `fallthroughCall()`, placeholder filtering, priority, and
  call-reference target collection.
- [`Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/disassembler/EntryPointAnalyzer.java`](../../../../Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/disassembler/EntryPointAnalyzer.java)
  supplies the preceding disassembly/event boundary.
- [`Ghidra/Features/Base/src/main/java/ghidra/app/cmd/function/CreateFunctionCmd.java`](../../../../Ghidra/Features/Base/src/main/java/ghidra/app/cmd/function/CreateFunctionCmd.java)
  supplies function-code-unit validation, body creation, placeholder repair,
  overlap subtraction, and `fixupFunctionBody()` behavior.
- [`Ghidra/Features/Base/src/main/java/ghidra/app/cmd/function/CreateThunkFunctionCmd.java`](../../../../Ghidra/Features/Base/src/main/java/ghidra/app/cmd/function/CreateThunkFunctionCmd.java)
  supplies simple thunk creation and thunk destination semantics.
- [`Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/model/block/FollowFlow.java`](../../../../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/model/block/FollowFlow.java)
  supplies flow traversal, call exclusion, function-entry boundaries, and
  cancellation points.

The C++ implementation reuses [`pe_loader`](../../pe_loader/README.md) for
mapped executable regions and [`sleigh_runtime`](../../sleigh_runtime/README.md)
for instruction flow and p-code. It does not parse PE bytes or decode x86
instructions itself.

## Transferred Behavior

- Call references are collected by source event and deduplicated by target.
- Unconditional, conditional, computed, and external call-reference categories
  are considered when the decoded instruction is a call flow.
- A call whose target equals the actual fall-through is ignored.
- Existing functions are preserved; only exact one-address nonterminal or
  oversized-instruction placeholders are repaired.
- Function creation constructs the body synchronously, follows non-call flow,
  terminates on returns and unresolved flow, preserves decoded instruction
  bytes, and derives basic/simple block views.
- Unconditional simple jump thunks create a target function first and record the
  thunk destination in `Function::thunk_target`.
- Overlapping bodies are carved using the entry-order rule from
  `subtractBodyFromExisting()`, with original function snapshots restored when
  creation cannot retain a valid entry.
- `CALL_RETURN` flow overrides rebuild the caller body, matching
  `FindNoReturnFunctionsAnalyzer.fixCallingFunctionBody()`.
- The analyzer observes code, reference, and flow-change events and checks
  cancellation while scanning and creating targets.
- `create_only_thunks` implements the original optional thunk-only target filter.

## Architecture Decision

Original Ghidra has no `FunctionBodyAnalyzer`. `CreateFunctionCmd` calls
`getFunctionBody()` and `FollowFlow` synchronously, and block models provide
views over the resulting body. The former native `function_body` analyzer was
therefore removed from the build and builtin registry. Its directory now holds
focused tests for the shared `AnalysisContext::create_function()` and
`rebuild_function_body()` implementation; see
[`../function_body/README.md`](../function_body/README.md).

## Known Differences and Risks

- The native `Instruction` model does not expose the complete p-code side-effect
  analysis used by `CreateThunkFunctionCmd.getThunkedAddr()`. The port accepts
  only bounded fall-through-to-resolved-unconditional-flow candidates and does
  not claim equivalence for register-side-effect-heavy or delayed-slot thunks.
- Native PE symbols do not yet provide the full Ghidra symbol-table namespace and
  external-address model. Local mapped call targets are fully tested; external
  function creation remains limited by the current `AnalysisContext` address
  model.
- Delay-slot-specific `FollowFlow` behavior is pending because Sleigh's current
  instruction model does not expose delay-slot depth.

## Verification

The integration fixture is [`tests/data/test_subroutine_references.cpp`](tests/data/test_subroutine_references.cpp).
Its Ghidra report is human-readable evidence only. The GTest module
[`tests/subroutine_references_tests.cppm`](tests/subroutine_references_tests.cppm)
hardcodes the function bodies and call rows from the report and never reads the
`.md` file. Focused fixture generation uses
[`tests/data/build.bat`](tests/data/build.bat), and original analysis uses the
repository-required [`TEST/run_ghidra_python.bat`](../../../../TEST/run_ghidra_python.bat)
wrapper around [`tests/data/run_ghidra.py`](tests/data/run_ghidra.py).
