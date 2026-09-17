# Review Report: Constant Propagation

- [x] Scope confirmed: [`src/constant_propagation.cppm`](src/constant_propagation.cppm), [`tests/constant_propagation_tests.cppm`](tests/constant_propagation_tests.cppm), fixture source/script/report, [`CMakeLists.txt`](CMakeLists.txt), [`build.bat`](build.bat), [`README.md`](README.md), and the absence of a module `GHIDRA_PORT.md`.
- [x] Original sources inspected: `Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/analysis/ConstantPropagationAnalyzer.java`, `Ghidra/Features/Base/src/main/java/ghidra/program/util/SymbolicPropogator.java`, and `ConstantPropagationContextEvaluator` call paths.
- [x] Reviewed diff: working tree at 2026-09-15; no commit supplied.
- [x] Reviewer: Kilo.
- [x] Source, test, fixture, documentation, CMake, and wrapper inspection completed.
- [x] Validation status recorded honestly: no build, CTest, formatter, or tidy command was run because this was a read-only audit.

## Findings

### Critical

No findings.

### High

#### CONSTANT-PROP-HIGH-001: The native evaluator is not the original SymbolicPropogator contract

- [ ] Remediation status: open.
- Severity: high.
- Affected component: [`src/constant_propagation.cppm#L34-L285`](src/constant_propagation.cppm#L34-L285) and [`#L321-L419`](src/constant_propagation.cppm#L321-L419).
- Technical evidence: original `ConstantPropagationAnalyzer.java#L480-L514` configures `SymbolicPropogator` and `ConstantPropagationContextEvaluator` with parameter/return/stored-reference checks, writable-memory trust, speculative/store-load bounds, and complex-data creation. Native evaluates a hand-selected p-code opcode switch and records only `ConstantFact` values. It does not create the original data/reference/settings effects, execute the full p-code evaluator, or expose the original option set.
- Expected behavior: propagate constants through the original symbolic executor, memory/reference semantics, unknown values, speculative limits, parameter/return/stored-reference checks, and data-marking behavior.
- Actual behavior: only the implemented opcode subset produces facts; unsupported or model-specific behavior is cleared or ignored, with no original listing side effects.
- Impact: values, references, data types, and downstream analyzer inputs diverge on realistic functions even when simple arithmetic passes.
- Root cause: a local fixed-point map was substituted for `SymbolicPropogator` and `ConstantPropagationContextEvaluator`.
- Recommended fix: port the symbolic executor/evaluator contract or expose a native equivalent with the same hooks and option semantics; do not label the opcode subset equivalent.
- Regression risks: full symbolic propagation can create many references/data objects and alter analyzer order; port original tests incrementally.
- Relevant tests or validation: native tests cover COPY/add/negate/shift and divide-by-zero invalidation only at [`tests/constant_propagation_tests.cppm#L15-L123`](tests/constant_propagation_tests.cppm#L15-L123).

#### CONSTANT-PROP-HIGH-002: Analysis scope and scheduling ignore the original added-set contract

- [ ] Remediation status: open.
- Severity: high.
- Affected component: [`src/constant_propagation.cppm#L313-L419`](src/constant_propagation.cppm#L313-L419).
- Technical evidence: original `ConstantPropagationAnalyzer.java#L175-L220` removes uninitialized blocks and processes functions/addresses in the supplied analysis set, while `#L480-L514` flows each selected function body. Native ignores the event span and iterates every function at lines 331-333 on every invocation. Its descriptor at lines 313-319 triggers `code_added`/`function_added` and has no original instruction-analyzer/options lifecycle.
- Expected behavior: analyze only affected/eligible functions and honor the original analyzer's priority, enablement, and option-driven flow scope.
- Actual behavior: every native invocation rescans all functions with blocks, regardless of event addresses or changed set.
- Impact: repeated/stale facts, unnecessary work, and different downstream event behavior; newly changed functions may be handled in a different order.
- Root cause: event-driven scheduling was replaced by a global scan.
- Recommended fix: pass the affected address set into propagation and preserve original lifecycle/options; add repeated-event and cancellation tests.
- Regression risks: narrowing the scope may remove facts currently produced by global scans; compare event traces and facts.
- Relevant tests or validation: no test asserts event-set restriction or repeated analysis behavior.

### Medium

#### CONSTANT-PROP-MEDIUM-001: Native fixture and tests do not exercise the claimed loop/switch/memory behavior

- [ ] Remediation status: open.
- Severity: medium.
- Affected component: [`tests/data/test_constant_propagation.cpp`](tests/data/test_constant_propagation.cpp), [`tests/data/run_ghidra.py`](tests/data/run_ghidra.py), [`tests/data/test_constant_propagation.md`](tests/data/test_constant_propagation.md), and [`tests/constant_propagation_tests.cppm`](tests/constant_propagation_tests.cppm).
- Technical evidence: fixture source claims loop/switch arithmetic, global stores, and conditional joins at lines 15-55, but the generated report records only unchanged function entries and explicitly says native tests do not parse it at `test_constant_propagation.md#L1-L23`. Native tests manually define four one-byte synthetic instructions and never analyze the compiled fixture's loop/switch.
- Expected behavior: integration tests should decode the actual fixture and assert constant facts, memory effects, joins, and any references/data changes.
- Actual behavior: the fixture is a human-readable Java oracle and native tests are straight-line hand-built p-code smoke tests.
- Impact: branch, loop convergence, memory alias, and actual Sleigh decoding regressions are untested.
- Root cause: CMake registers only GoogleTest and the native test support uses synthetic instructions.
- Recommended fix: add decoded-fixture integration assertions and keep the PyGhidra report as a separate original oracle.
- Regression risks: compiler output is not stable across toolchains; pin bytes or use standalone decompiler/decoded p-code fixtures.
- Relevant tests or validation: no commands were run during this audit.

### Low

#### CONSTANT-PROP-LOW-001: Required port evidence is incomplete

- [ ] Remediation status: open.
- Severity: low.
- Affected component: module documentation.
- Technical evidence: unlike the other requested Java ports, `services/analyzers/constant_propagation` has `README.md` but no `GHIDRA_PORT.md`, despite the repository rule requiring port evidence for each Java feature.
- Expected behavior: document original sources, transferred/omitted behavior, limitations, tests, and build dependencies in a module-local `GHIDRA_PORT.md`.
- Actual behavior: port-specific status is absent.
- Impact: omissions and intentional deviations are harder to audit and can be mistaken for completed behavior.
- Root cause: documentation synchronization was not completed for this module.
- Recommended fix: add the required port evidence when implementation status is next changed.
- Regression risks: documentation can overstate parity if written before behavior is classified.
- Relevant tests or validation: file inventory was inspected read-only.

## Verified Strengths

- [x] The native evaluator has explicit truncation, sign extension, integer/boolean arithmetic, memory reads, joins, cancellation, and unknown-write invalidation paths.
- [x] The fixture source is purposeful and documents its intended control-flow cases.

## Reviewed Areas With No Findings

- [x] CMake target/test registration and fixture build wrapper were inspected.
- [x] No implementation, test, fixture, documentation, or build source was changed.

## Validation Results

- [x] Read-only original/native comparison completed.
- [x] Read-only test, fixture, documentation, CMake, and wrapper inspection completed.
- [ ] Focused build/test: not run by request.
- [ ] `format.bat constant_propagation`: not run by request.
- [ ] `tidy.bat constant_propagation --check`: not run by request.

## Unresolved Questions

- [ ] The complete native equivalent of `SymbolicPropogator` and its data/reference evaluator APIs is not exposed in the reviewed boundary.

## Residual Risks

- [ ] Memory aliasing, floating/other p-code operations, speculative references, and complex data creation remain outside the tested subset.

## Follow-Up

- [ ] Highest proposed remediations: CONSTANT-PROP-HIGH-001 and CONSTANT-PROP-HIGH-002.
- [ ] Medium proposed remediation: CONSTANT-PROP-MEDIUM-001.
- [ ] Low proposed remediation: CONSTANT-PROP-LOW-001.
- [ ] Final follow-up decision: keep implementation unchanged until symbolic-propagation parity is authorized.
