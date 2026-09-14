# Strict Sleigh Runtime API Audit

## Scope

- [x] Reviewed [`sleigh_runtime.cppm`](sleigh_runtime.cppm), [`sleigh_runtime_adapter.cppm`](sleigh_runtime_adapter.cppm), and the analyzer/decompiler consumers of `Instruction`, `FlowInfo`, `PcodeOp`, and `ProcessorContext`.
- [x] Compared flow and context contracts with [`Instruction.java`](../../../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/model/listing/Instruction.java) and [`ProcessorContext.java`](../../../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/model/lang/ProcessorContext.java).
- [x] Reviewer: Kilo.
- [x] Review date: 2026-09-15.
- [x] Review type: strict read-only source audit; no implementation or configuration changes were made.
- [x] Recommendations target the runtime/API boundary, not analyzer-local heuristics.

## Findings

### Critical

No findings.

### High

#### SLEIGH-HIGH-001: The runtime collapses an instruction's flow set to one effect

- [ ] Remediation complete.
- Severity: high.
- Title: `FlowInfo` and `find_flow` cannot preserve multiple control-flow destinations or effects.
- Source: [`sleigh_runtime.cppm:151-157`](sleigh_runtime.cppm#L151-L157), [`sleigh_runtime_adapter.cppm:198-257`](sleigh_runtime_adapter.cppm#L198-L257), symbols `FlowInfo` and `find_flow`.
- Affected component: function discovery, CFG construction, jump-table/indirect-flow recovery, reference creation, and decompiler flow input.
- Technical evidence: `FlowInfo` has one `kind` and one optional `target`; `find_flow` returns immediately after the first control-flow operation and stores only one conditional target. Original `Instruction` exposes `getFlows()` and `getDefaultFlows()` arrays at [`Instruction.java:187-202`](../../../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/model/listing/Instruction.java#L187-L202).
- Expected behavior: a decoded instruction must preserve every default/explicit flow edge, fall-through status, indirect/computed classification, and any call/branch effects needed by later analysis.
- Actual behavior: additional branch/call effects in one p-code sequence are discarded; only one target reaches `AnalysisContext::define_instruction`, which creates at most one flow reference and one traversed destination.
- Impact: valid multi-edge instructions and architecture-specific flow constructs produce incomplete bodies, missing references, and incorrect CFGs. The result cannot be repaired after the information has been discarded by the runtime.
- Reproduction or failure scenario: provide a Sleigh instruction whose p-code emits two direct flow operations or a conditional flow plus another explicit destination; `decode().flow` contains only the first selected effect and the omitted target never reaches the analyzer context.
- Root cause: the public result model treats flow as a scalar classification rather than a collection of typed edges.
- Recommended fix: expose a flow-edge vector with per-edge kind, target, fall-through, terminal, delay-slot, and indirect metadata; keep a derived primary summary only for compatibility.
- Regression risks: consumers must stop assuming one target, and function-body traversal must preserve existing call fall-through rules while adding all edge types.
- Relevant tests or validation: current runtime tests assert one target for representative x86 instructions but do not cover a multi-flow p-code sequence or an architecture-specific instruction with more than one explicit edge.

### Medium

#### SLEIGH-MEDIUM-001: Processor context is reset per instruction and cannot represent address-range context

- [ ] Remediation complete.
- Severity: medium.
- Title: `Decoder::decode` accepts only call-local defaults, not the program's address-indexed processor context.
- Source: [`sleigh_runtime.cppm:159-168`](sleigh_runtime.cppm#L159-L168), [`sleigh_runtime_adapter.cppm:760-796`](sleigh_runtime_adapter.cppm#L760-L796), symbols `ProcessorContext` and `Decoder::Implementation::decode`.
- Affected component: mode-dependent decoding, segmented/context-sensitive processors, Sleigh p-code correctness, and any analyzer using non-x86 specifications.
- Technical evidence: every decode replaces `ContextInternal` at line 791 and applies only the supplied `ProcessorContext::values` as defaults. The original `ProgramContext` stores register values over address ranges, returns values at a specific address, and supports changing ranges at [`ProgramContext.java:25-122`](../../../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/model/listing/ProgramContext.java#L25-L122).
- Expected behavior: decoding at an address must observe context values defined for that address, including non-default values and range boundaries, while preserving context changes made by analysis.
- Actual behavior: two addresses decoded with the same call-local defaults are indistinguishable even if the program has a context change between them; context state cannot be persisted or queried by the analyzer context.
- Impact: an otherwise valid SLA can decode the wrong instruction constructor, flow target, operand, or p-code for context-sensitive architectures.
- Reproduction or failure scenario: define a context register value for a second address range in an equivalent program model, decode both addresses through the runtime API, and observe that only the one global default is used because there is no address-range input or storage.
- Root cause: `ProcessorContext` was reduced to a vector of defaults and the legacy context database is recreated for every call.
- Recommended fix: add an address-indexed context database/range provider to the runtime boundary and make decode query it at the instruction address; retain explicit defaults as the fallback.
- Regression risks: x86 default decoding must remain unchanged; test context ranges, clears, overlapping ranges, and context values that are illegal across existing instructions.
- Relevant tests or validation: the existing context-isolation tests verify reset/no-leak behavior, but that is not equivalent to the original address-indexed context contract.

#### SLEIGH-MEDIUM-002: P-code operand provenance is declared but never materialized

- [ ] Remediation complete.
- Severity: medium.
- Title: `PcodeOp::source_operand` is always empty in the production capture path.
- Source: [`sleigh_runtime.cppm:137-149`](sleigh_runtime.cppm#L137-L149), [`sleigh_runtime_adapter.cppm:45-68`](sleigh_runtime_adapter.cppm#L45-L68), symbols `PcodeOp` and `PcodeCapture::dump`.
- Affected component: operand-specific data/scalar/reference analysis and decompiler source mapping.
- Technical evidence: the public struct offers `source_operand`, but `PcodeCapture::dump` constructs `PcodeOp` with opcode/output/inputs only and never associates the emitted operation with the Sleigh operand that produced it. The source comment itself identifies the missing `OperandObject` association from `SymbolicPropogator.java`.
- Expected behavior: p-code operations that originate from an instruction operand should preserve the operand index or an equivalent stable provenance relation.
- Actual behavior: consumers must infer operand identity from textual operands or operation position, which is ambiguous for repeated, implicit, vector, and expanded operands.
- Impact: reference type/operand-index conflicts cannot be resolved with the original precision, and decompiler or analyzer results can attach a reference to the wrong operand.
- Reproduction or failure scenario: decode an instruction with two operands that each expand into several p-code operations; every operation has `source_operand == null`, so the API cannot distinguish the two source operands.
- Root cause: the runtime captures raw p-code but omits the parser/prototype operand association.
- Recommended fix: expose stable operand provenance from the Sleigh capture boundary and preserve it through the decompiler provider conversion.
- Regression risks: some legacy SLA paths have no association; represent unknown explicitly and test both known and unknown provenance.
- Relevant tests or validation: current Sleigh tests validate opcode and varnode sequences but do not assert source operand mapping.

### Low

No findings.

## Verified Strengths

- [x] P-code varnodes are copied into owning public values before legacy runtime storage can be reused.
- [x] Decode input is bounded to the architectural 16-byte window and rejects undersized instruction input.
- [x] LOAD/STORE target address spaces are preserved separately from legacy constant selectors.
- [x] Runtime opcode values were checked against [`opcodes.hh`](../../../Ghidra/Features/Decompiler/src/decompile/cpp/opcodes.hh), including the intentionally unused slot 45.

## Reviewed Areas With No Findings

- [x] Basic p-code materialization for the covered x86 and ARM fixtures.
- [x] Move-only ownership and decoder teardown declarations.
- [x] Explicit SLA path resolution behavior.

## Validation

- [x] `git diff --check` completed without whitespace errors.
- [x] `ctest --test-dir NEW/build -N` listed `sleigh_runtime_tests` and dependent consumers.
- [ ] Runtime build/test execution was not performed because this was a strict read-only audit and execution may write logs or fixtures.

## Unresolved Questions And Residual Risks

- [ ] The exact delay-slot and callother flow metadata needed by all supported processor specifications remains undefined in the public result model.
- [ ] A context provider must define conflict behavior for overlapping address ranges before non-x86 context-sensitive analyzers can rely on it.

## Follow-Up Decision

- [ ] No fixes were authorized or applied.
- [ ] Highest-priority Sleigh finding for remediation is SLEIGH-HIGH-001.
