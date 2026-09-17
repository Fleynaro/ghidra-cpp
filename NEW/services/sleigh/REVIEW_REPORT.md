# Strict Sleigh Runtime API Audit

## Scope

- [x] Reviewed [`sleigh_runtime.cppm`](sleigh_runtime.cppm), [`sleigh_runtime_adapter.cppm`](sleigh_runtime_adapter.cppm), and the analyzer/decompiler consumers of `Instruction`, `FlowInfo`, `PcodeOp`, and `ProcessorContext`.
- [x] Compared flow and context contracts with [`Instruction.java`](../../../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/model/listing/Instruction.java) and [`ProcessorContext.java`](../../../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/model/lang/ProcessorContext.java).
- [x] Reviewer: Kilo.
- [x] Review date: 2026-09-15.
- [x] Review type: source audit plus measured performance investigation and implementation validation.
- [x] Recommendations target the runtime/API boundary, not analyzer-local heuristics.
- [x] Current core-contract boundary addendum reviewed; its detailed findings are recorded in [`../REVIEW_REPORT.md`](../REVIEW_REPORT.md) and [`../../core/contracts/REVIEW_REPORT.md`](../../core/contracts/REVIEW_REPORT.md).

## Findings

### Critical

#### SLEIGH-CRITICAL-001: SLA table construction is repeated for every test-created decoder

- [x] Remediation complete.
- Severity: critical.
- Title: the runtime reparses and rebuilds the immutable compiled SLA symbol/pattern tables for every `Decoder` construction.
- Source: [`sleigh_runtime_adapter.cppm:746-895`](sleigh_runtime_adapter.cppm#L746-L895), symbols `SharedSleighRuntime` and `Decoder::Implementation`, and [`src/sleigh.cppm:646-684`](src/sleigh.cppm#L646-L684), symbols `ghidra::Sleigh::attach`, `reset`, and `initialize`.
- Affected component: decoder construction, all consumers that create short-lived decoders, SLA loading, table allocation, and test setup.
- Technical evidence: a profiled Debug build (`SLEIGH_RUNTIME_PROFILE=ON`) ran the complete executable with `--gtest_color=no`; phase scopes were placed around file reads, decompression, SLA table decoding, parser-cache construction, reset/context setup, assembly, p-code, and public-result materialization. The pre-change `sleigh_runtime_tests` executable registers 31 GoogleTest cases, while CTest registers 43 project-level tests; all 31 runtime cases passed.
- Baseline: the unmodified executable completed 31/31 tests in 19.096 seconds wall time; GoogleTest reported 19.852 seconds of test time on the direct baseline run. Individual x86 decode tests took approximately 0.60-0.83 seconds, while the constructor-only path test took 0.855 seconds.
- Measured phase totals across the profiled suite:
  - `sla_table_decode`: 29 calls, 12,187.82 ms total, 420.27 ms average, 23,378,856 allocations and 913,450,560 allocated bytes.
  - `decoder_constructor`: 30 calls, 12,743.49 ms total, 424.78 ms average, 23,624,253 allocations and 1,054,894,333 allocated bytes.
  - `sla_decompression`: 29 calls, 482.26 ms total, 16.63 ms average, 235,779 allocations and 126,369,272 allocated bytes.
  - `sla_io`: 29 calls, 13.65 ms total, 0.47 ms average, 58 allocations and 13,717,742 allocated bytes.
  - `parser_cache_construction`: 76 calls, 2.92 ms total, 0.04 ms average, 20,596 allocations and 2,559,072 allocated bytes.
  - `decode_reset_and_context`: 47 calls, 30.44 ms total, 0.65 ms average, 18,068 allocations and 1,847,320 allocated bytes.
  - `assembly_decode`, `pcode_decode`, and `result_materialization`: 47, 46, and 46 calls respectively, totaling 1.58 ms, 3.05 ms, and 39.68 ms. These are not the bottleneck.
- Expected behavior: immutable compiled SLA data should be loaded and table-built once per unchanged specification, while each decoder retains independent instruction bytes, context, parser state, and public result ownership.
- Actual behavior: each of the 30 valid decoder constructions repeats the full symbol/pattern table build; `.sla` I/O is less than 0.2% of constructor time, decompression is about 3.8%, and table construction is about 95.6% of constructor time. The repeated parse creates approximately 23.4 million Debug CRT allocations for the x86/ARM test setup alone.
- Impact: short-lived decoder usage spends nearly all suite time reconstructing identical immutable architecture metadata instead of decoding instructions. This prevents the requested 5-10 second target even though actual assembly and p-code decoding are sub-millisecond phases.
- Reproduction or failure scenario: run `\build\\features\\sleigh_runtime\\tests\\sleigh_runtime_tests.exe --gtest_color=no` before optimization, or create multiple `Decoder("x86-64.sla")` instances sequentially; each construction emits a new `Sleigh::initialize` table decode and repeats the allocation burst.
- Root cause: `Decoder::Implementation` owns a fresh `ghidra::Sleigh` and calls `initialize(sla_path)` for every instance. The legacy `Sleigh::initialize` path parses and materializes the complete SLA symbol tree when `isInitialized()` is false.
- Recommended fix: cache the immutable parsed SLA runtime per normalized specification path and bind each decoder to independent `ByteLoadImage` and `ContextInternal` state. Reuse parser storage where the bound state is unchanged, invalidate only mutable parse state between calls, and serialize access to a shared legacy translator if the legacy object remains mutable.
- Regression risks: stale cache entries if a specification file changes, cross-decoder state leakage, concurrent decode races, and parser-cache reuse under different processor contexts. Key cache entries by file identity/version and retain an explicit lock around shared mutable legacy operations; verify context isolation, move semantics, missing-file diagnostics, x86/ARM results, and malformed input.
- Relevant tests or validation: [`tests/sleigh_runtime_tests.cppm`](tests/sleigh_runtime_tests.cppm) covers x86/ARM decoding, context isolation, malformed input, path resolution, moved-from behavior, and independent decoder state. The profiling run preserved the pre-change checked-out test list and passed all 31 tests; final validation passed all 32 current executable tests.
- Remediation evidence: immutable SLA runtimes are now cached by normalized path, size, and modification timestamp in [`sleigh_runtime_adapter.cppm`](sleigh_runtime_adapter.cppm); each decoder keeps independent image/context objects and shared legacy access is serialized. [`src/sleigh.cppm`](src/sleigh.cppm) retains parser storage across compatible resets, and [`src/globalcontext.cppm`](src/globalcontext.cppm) clears context values without re-registering fields. The post-optimization profile loaded each of the two specifications once, and the final module executable passed 32/32 tests in 0.785 seconds of GoogleTest time. The full CTest integration run passed all 43 registered project tests, including `sleigh_runtime_tests`, in 23.26 seconds.

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
- [x] `ctest --test-dir build -N` listed `sleigh_runtime_tests` and dependent consumers.
- [x] `\features\\sleigh_runtime\\build.bat` built the formatted module and passed `sleigh_runtime_tests`.
- [x] Direct `sleigh_runtime_tests.exe --gtest_color=no` completed 32/32 tests with 0 failures and 785 ms reported test time.
- [x] `\build.bat all` completed the full build and all 49 registered CTest tests passed; `sleigh_runtime_tests` passed within that run.
- [x] `\features\\sleigh_runtime\\format.bat` formatted the module successfully.
- [ ] `\features\\sleigh_runtime\\tidy.bat --check` completed with a tooling failure: clang-tidy cannot parse the generated MSVC `.ifc` plus source compilation command and reports multi-source `/Fo` errors for existing module units; no source diagnostic was emitted for the modified implementation units.

## Unresolved Questions And Residual Risks

- [ ] The exact delay-slot and callother flow metadata needed by all supported processor specifications remains undefined in the public result model.
- [ ] A context provider must define conflict behavior for overlapping address ranges before non-x86 context-sensitive analyzers can rely on it.

## Follow-Up Decision

- [x] SLEIGH-CRITICAL-001 was authorized by the optimization request and remediated without weakening decoding coverage.
- [x] The authorized domain follow-up now returns `core::DecodedInstruction` from `IPCodeDecoder` and centralizes promotion in `core::materialize_decoded_instruction()`.
- [ ] Remaining highest-priority behavioral finding is SLEIGH-HIGH-001; it was not part of this performance change.
