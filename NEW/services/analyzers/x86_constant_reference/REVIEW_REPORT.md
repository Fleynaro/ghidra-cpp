# Review Report: x86 Constant Reference

- [x] Scope confirmed: [`src/x86_constant_reference.cppm`](src/x86_constant_reference.cppm), [`tests/x86_constant_reference_tests.cppm`](tests/x86_constant_reference_tests.cppm), fixture source/PDB/script/report, [`CMakeLists.txt`](CMakeLists.txt), [`build.bat`](build.bat), [`README.md`](README.md), and [`GHIDRA_PORT.md`](GHIDRA_PORT.md).
- [x] Original sources inspected: `Ghidra/Processors/x86/src/main/java/ghidra/app/plugin/core/analysis/X86Analyzer.java`, `Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/analysis/ConstantPropagationAnalyzer.java`, and `Ghidra/Features/Base/ghidra_scripts/PropagateX86ConstantReferences.java`.
- [x] Reviewed diff: working tree at 2026-09-15; no commit supplied.
- [x] Reviewer: Kilo.
- [x] Source, test, fixture, documentation, CMake, and wrapper inspection completed.
- [x] Validation status recorded honestly: no build, CTest, formatter, or tidy command was run because this was a read-only audit.

## Findings

### Critical

No findings.

### High

#### X86-HIGH-001: X86Analyzer and PropagateX86ConstantReferences behavior is reduced to one LEA reference pass

- [ ] Remediation status: open.
- Severity: high.
- Affected component: [`src/x86_constant_reference.cppm#L79-L114`](src/x86_constant_reference.cppm#L79-L114) and registration at [`../analyzer_builtin.cpp#L50-L63`](../analyzer_builtin.cpp#L50-L63).
- Technical evidence: original `X86Analyzer.java#L47-L99` overrides `flowConstants`, invokes the full `ConstantPropagationContextEvaluator`, creates LEA operand references from symbolic register values, and rejects invalid flow references. Original `PropagateX86ConstantReferences.java#L57-L149` additionally propagates stores/loads, marks computed flow destinations, and `#L151-L291` explores unknown branch/table values, fixes function bodies through `AddressTable`, and labels recovered cases. Native code only recognizes `lea`, chooses one operand/constant fact, and adds one DATA reference.
- Expected behavior: retain generic constant propagation while adding x86 LEA/store/load/reference hooks and the script's speculative computed-branch/address-table recovery.
- Actual behavior: no branch/table recovery, no `AddressTable` fixup/labels, and no general store/load/reference propagation are implemented.
- Impact: switch and computed-flow recovery cases from the original x86 feature are entirely omitted.
- Root cause: the original subclass/script behavior was split into a narrow `X86ConstantReferenceAnalyzer` rather than ported as an x86 propagation extension.
- Recommended fix: port the x86 evaluator hooks and speculative address-table algorithm, or explicitly scope this module to LEA-only and remove parity claims.
- Regression risks: speculative recovery can change function bodies and labels; compare original switch-table fixtures before enabling it.
- Relevant tests or validation: [`tests/data/test_x86_constant_reference.md`](tests/data/test_x86_constant_reference.md) covers one LEA operand reference only.

#### X86-HIGH-002: Propagated values are not path/context-specific

- [ ] Remediation status: open.
- Severity: high.
- Affected component: [`src/x86_constant_reference.cppm#L50-L59`](src/x86_constant_reference.cppm#L50-L59) and [`#L97-L103`](src/x86_constant_reference.cppm#L97-L103).
- Technical evidence: `propagated_value` scans all `constant_facts()` in reverse and returns the latest fact with matching storage and an earlier instruction, without checking the function block, incoming path, or the LEA operation's exact symbolic context. Original `X86Analyzer` obtains the value from `VarnodeContext` during `SymbolicPropogator.flowConstants` for the current instruction/path.
- Expected behavior: use the value valid at the current instruction on the current flow path, including joins and invalidations.
- Actual behavior: a stable fact from another block/path can be reused as if it were the current register value.
- Impact: wrong DATA references can be created at LEAs after path-dependent register changes.
- Root cause: an append-only fact list was substituted for the original symbolic execution context.
- Recommended fix: expose path-aware constant state or run the x86 hook inside the native propagator's current flow state.
- Regression risks: path-aware joins may remove references currently produced by the broad scan; add branch/merge/overwrite cases.
- Relevant tests or validation: current native test has one straight-line LEA and no path merge.

### Medium

#### X86-MEDIUM-001: x86-only processor contract and absolute-address semantics are changed

- [ ] Remediation status: open.
- Severity: medium.
- Affected component: [`src/x86_constant_reference.cppm#L25-L35`](src/x86_constant_reference.cppm#L25-L35) and [`#L82-L84`](src/x86_constant_reference.cppm#L82-L84).
- Technical evidence: original `X86Analyzer.java#L40-L44` accepts only the `x86` processor. Native accepts both `pe::Machine::i386` and `pe::Machine::amd64`. Native `resolve_address` first accepts an image VA and then invents an RVA-to-VA fallback; original `X86Analyzer.java#L62-L65` constructs the address directly with `instr.getMinAddress().getNewAddress(lval)` and requires it in program memory.
- Expected behavior: run only for the original x86 processor and use the original program-address interpretation.
- Actual behavior: x64 programs can receive the x86-specific analyzer and small values can be reinterpreted as RVAs.
- Impact: unsupported x64/RVA references can be added and differ from original address validation.
- Root cause: PE machine metadata and loader translation were used as a broader substitute for language/address semantics.
- Recommended fix: gate by language processor and preserve program-address interpretation; add explicit RVA behavior only if separately specified.
- Regression risks: tightening the gate removes current x64 approximation results; update docs/tests to reflect original x86 scope.
- Relevant tests or validation: original x86 fixture is built as 32-bit at [`tests/data/build.bat#L4-L26`](tests/data/build.bat#L4-L26).

#### X86-MEDIUM-002: Native test is synthetic and loads the 32-bit fixture through the x64 helper

- [ ] Remediation status: open.
- Severity: medium.
- Affected component: [`tests/x86_constant_reference_tests.cppm`](tests/x86_constant_reference_tests.cppm), fixture build/script/report, and [`../shared/test_support/analyzer_test_support.cppm#L20-L32`](../shared/test_support/analyzer_test_support.cppm#L20-L32).
- Technical evidence: the native test defines a synthetic LEA/instruction and checks one reference; it does not decode the checked-in PE. Shared `load_fixture` hard-codes `x86-64.sla`, while the fixture build selects `VsDevCmd -arch=x86` and reports an x86 PE.
- Expected behavior: integration testing should decode the actual x86 fixture with an x86 profile and exercise constant propagation plus analyzer registration.
- Actual behavior: the native test proves only the replacement record logic and can avoid the wrong-language integration path.
- Impact: real x86 decode/propagation regressions are invisible.
- Root cause: one x64 fixture loader is reused for all feature tests.
- Recommended fix: add architecture-aware fixture loading and an actual decoded x86 test with before/after references.
- Regression risks: architecture-aware setup can alter fixture discovery; assert selected language and PE machine.
- Relevant tests or validation: no commands were run during this audit.

### Low

No findings.

## Verified Strengths

- [x] LEA recognition, pointer threshold, mapped-memory checks, cancellation, duplicate suppression, and explicit DATA reference creation are present.
- [x] The original x86 Java/script sources and checked-in x86 fixture are traceable.

## Reviewed Areas With No Findings

- [x] CMake target/test registration and module wrapper were inspected.
- [x] No implementation, test, fixture, documentation, or build source was changed.

## Validation Results

- [x] Read-only original/native comparison completed.
- [x] Read-only test, fixture, documentation, CMake, and wrapper inspection completed.
- [ ] Focused build/test: not run by request.
- [ ] `NEW/format.bat x86_constant_reference`: not run by request.
- [ ] `NEW/tidy.bat x86_constant_reference --check`: not run by request.

## Unresolved Questions

- [ ] The native path-aware symbolic state and address-table repair APIs required by the original x86 behavior are not exposed to this module.

## Residual Risks

- [ ] Any future x64 enablement would be a behavioral extension, not an original X86Analyzer port.

## Follow-Up

- [ ] Highest proposed remediations: X86-HIGH-001 and X86-HIGH-002.
- [ ] Medium proposed remediations: X86-MEDIUM-001 and X86-MEDIUM-002.
- [ ] Final follow-up decision: keep implementation unchanged until x86 propagation and fixture-language parity is authorized.
