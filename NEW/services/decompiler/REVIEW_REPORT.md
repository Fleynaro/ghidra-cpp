# Independent Provenance Review

## Review Metadata

- [x] **Scope confirmed:** `services/decompiler`, including the provenance API, markup capture, CMake registration, documentation, and `tests/provenance_mapping_tests.cppm`.
- [x] **Reviewed diff:** current uncommitted working-tree changes on 2026-09-16.
- [x] **Reviewer:** Kilo independent review.
- [x] **Review objective:** determine whether a future Decompiled Code Viewer can reliably map a clicked Clang token/node to all relevant P-code operations and original ASM instruction addresses, and whether the test proves both directions.
- [x] **Assumption:** a viewer needs stable token identity, source location or rendering order, and a non-vacuous reverse lookup from an independently selected ASM address.

## Critical Findings

No findings.

## High Findings

### PROV-HIGH-001: Reverse-address assertions are partly vacuous

- [x] **Remediation status:** Fixed in the current working tree.
- **Severity:** High.
- **Title:** The reverse ASM address -> PcodeOp -> Clang token direction is not independently tested for most constructs.
- **Source reference:** `tests/provenance_mapping_tests.cppm:587-590`, `:600-603`, `:616-619`, and `:628-631` (`addresses_for_tokens` followed by `expect_asm_to_pcode` / `expect_asm_to_tokens`).
- **Affected component:** `DecompilerProvenance.BidirectionalSleighMarkupAndNativeGraph` reverse mapping coverage.
- **Technical evidence before remediation:** Loop, conditional, switch, and call addresses were first derived from the selected token's `opref` by `addresses_for_tokens`.
- **Expected behavior:** The test should select known instruction addresses from the fixture bytes before inspecting markup, find every PcodeOp at each address, then find the associated token/node(s), and assert the expected construct. The selected address must not originate from the token under test.
- **Actual behavior after remediation:** `expect_asm_to_pcode` and `expect_asm_to_nodes` now consume independently selected instruction addresses and the production `InstructionProvenance` index. Loop, conditional, switch, and call addresses are no longer derived from markup nodes.
- **Impact:** A future reverse index could be missing, incomplete, or wrong while this test still passes. A viewer click on a loop branch, condition instruction, switch dispatch, or CALL could therefore fail in production despite a green test suite.
- **Reproduction/failure scenario:** Replace the reverse lookup with an empty address index and retain the forward `opref` lookup. The current derived-address assertions still obtain addresses from tokens and can continue to pass; an independent ASM click would have no result.
- **Root cause:** The former helper `addresses_for_tokens` coupled the input of the reverse assertion to the output of the forward assertion.
- **Implemented fix:** Added `InstructionProvenance`, independent address-driven helpers, and reverse assertions for branch/arithmetic instructions, switch dispatch/case bodies, field load/store, and CALL.
- **Regression risks:** Overly strict one-to-one assertions would reject valid many-to-one mappings; the replacement should allow sets of PcodeOps and tokens per address.
- **Relevant validation:** Focused provenance test and complete decompiler test target pass.

### PROV-HIGH-002: No stable viewer-facing token/node identity or source span exists

- [x] **Remediation status:** Fixed in the current working tree.
- **Severity:** High.
- **Title:** `DecompilationResult` exposes raw markup text but no structured token handles, node IDs, parent IDs, or source spans.
- **Source reference:** `src/decompiler.cppm:568-582` (`DecompilationResult::clang_markup`) and `tests/provenance_mapping_tests.cppm:148-273` (`MarkupToken` and the test-only XML scanner).
- **Affected component:** Public decompiler result consumed by a future Decompiled Code Viewer.
- **Technical evidence before remediation:** The production API returned one XML string and the test reconstructed tokens with a private parser.
- **Expected behavior:** A viewer-facing contract should provide a structured markup document or a documented stable token/node identity. Each displayed token/node should be addressable without matching text or relying on incidental XML order, and should expose its parent/statement and provenance references.
- **Actual behavior after remediation:** `ClangMarkupNodeProvenance` provides stable per-result IDs, element/content, selectable status, parent/statement IDs, field/Varnode references, operation sets, and originating addresses. The raw XML remains available for serialization.
- **Impact:** A click target cannot be persisted or reliably correlated with the rendered C/Clang view. The viewer can show an approximate mapping, but not a robust contract for selecting a specific occurrence such as the second `field_0x4` or one of several `+` operators.
- **Reproduction/failure scenario:** Render a function with two identical field names or repeated operators. Text search returns multiple candidates, while `DecompilationResult` provides no stable token ID or source span to distinguish them.
- **Root cause before remediation:** Provenance was exposed only as a serialized markup blob and native snapshots.
- **Implemented fix:** Added production-owned `ClangMarkupNodeProvenance` records with stable IDs, hierarchy, selectable token metadata, complete operation/address origins, and Varnode references. The production parser uses `pugixml`, not test-only text matching.
- **Regression risks:** IDs must be documented as stable only for the lifetime/version of one result, not globally across decompilations. Serialization changes must preserve parent-child and provenance relationships.
- **Relevant validation:** Focused provenance test now consumes only the production node/index objects and passes.

## Medium Findings

### PROV-MEDIUM-001: A token is validated against only one PcodeOp and one address

- [x] **Remediation status:** Fixed in the current working tree.
- **Severity:** Medium.
- **Title:** The test does not prove complete many-to-many provenance for expressions and statements.
- **Source reference:** `src/decompiler.cppm:543-555` (`PcodeOpProvenance`), `src/decompiler_impl.cppm:298-313` (`capture_native_provenance`), and `tests/provenance_mapping_tests.cppm:301-313` (`expect_token_chain`).
- **Affected component:** Token/Pcode/ASM provenance completeness.
- **Technical evidence before remediation:** Each test token had at most one operation parsed from one `opref`.
- **Expected behavior:** A viewer click should be able to show the complete set of relevant PcodeOps and ASM addresses, while still permitting a primary anchor operation. Compound expressions and statements must allow multiple origins.
- **Actual behavior after remediation:** Every node receives the union of operation references in its nearest statement subtree; each node retains all originating addresses. PcodeOps and instructions receive reverse node ID sets.
- **Impact:** Viewer navigation may highlight only one instruction for an expression whose calculation spans several instructions, or miss a load, address calculation, comparison, and branch that all explain the clicked construct.
- **Reproduction/failure scenario:** Use a field access preceded by pointer arithmetic and a load, or a condition compiled into compare plus conditional branch. Remove one secondary token/Pcode relation; the current test can still pass if the primary `opref` remains.
- **Root cause before remediation:** The snapshot model mirrored individual `opref` attributes only.
- **Implemented fix:** Added `operation_refs`, `originating_addresses`, PcodeOp-to-node IDs, and instruction-to-node IDs. Integrity checks cover every emitted node reference and every reverse Pcode relation.
- **Regression risks:** Some display tokens intentionally have one anchor while neighboring syntax has no direct operation; tests must distinguish optional syntax inheritance from missing semantic provenance.
- **Relevant validation:** Focused and full decompiler tests pass with complete set integrity assertions.

### PROV-MEDIUM-002: The visible `case` keyword itself has no direct operation reference

- [x] **Remediation status:** Fixed in the current working tree.
- **Severity:** Medium.
- **Title:** Switch coverage maps the case value token, not every clickable part of the case label.
- **Source reference:** `src/printc.cppm:3233-3240`, `src/prettyprint.cppm:299-319`, and `tests/provenance_mapping_tests.cppm:605-619`.
- **Affected component:** Switch/case viewer navigation.
- **Technical evidence before remediation:** `EmitMarkup::tagCaseLabel` wrote `opref` on `<value>`, while generic syntax text was emitted as `<syntax>`; the test selected only `<value>` nodes.
- **Expected behavior:** Clicking either the case label/value or the case keyword should resolve to the case-entry PcodeOp and its ASM address, or the UI contract should explicitly document that only the value token is clickable.
- **Actual behavior after remediation:** Production provenance links a visible `case`/`default` syntax node to its adjacent value node's operation set. The test explicitly selects and validates syntax case nodes.
- **Impact:** A viewer click on the visually prominent `case` keyword may produce no mapping or may map to the wrong case when labels are repeated or grouped.
- **Reproduction/failure scenario:** Render a switch and click the `case` word rather than the numeric value. The current production result has no token-level `opref` on that syntax node.
- **Root cause before remediation:** The original emitter intentionally annotates the case value only.
- **Implemented fix:** Added a production syntax-to-value anchor relation during structured markup capture and tested both value and visible case syntax nodes.
- **Regression risks:** Mapping all punctuation and keywords to operations may create noisy or misleading highlights; keep the primary semantic case anchor explicit.
- **Relevant validation:** Switch assertions pass for both `<value>` and `<syntax>case</syntax>` nodes.

### PROV-MEDIUM-003: Reverse lookup requires a linear scan and has no explicit instruction index/range

- [x] **Remediation status:** Fixed in the current working tree.
- **Severity:** Medium.
- **Title:** The public snapshot is difficult to consume efficiently and cannot describe instruction ranges.
- **Source reference:** `src/decompiler.cppm:548-555` and `:570-582`; `tests/provenance_mapping_tests.cppm:275-299`.
- **Affected component:** Runtime Viewer lookup from ASM address to PcodeOps and tokens.
- **Technical evidence before remediation:** `DecompilationResult` exposed vectors only and the test performed linear scans.
- **Expected behavior:** The result should offer a direct or cheaply constructible reverse index and enough range information to highlight an instruction or instruction span.
- **Actual behavior after remediation:** `InstructionProvenance` groups all PcodeOps and Clang node IDs by address and retains decoded instruction length when available. PcodeOps, Varnodes, and Clang nodes also carry reverse IDs.
- **Impact:** Large functions can incur repeated scans, and a viewer cannot distinguish or highlight a multi-byte instruction range from the provenance object alone.
- **Reproduction/failure scenario:** Repeatedly click tokens in a large function and scan `pcode_provenance`/markup each time; complexity grows with every click and duplicate-address operations require ad hoc grouping.
- **Root cause before remediation:** The snapshot was designed primarily for assertions.
- **Implemented fix:** Build immutable value-owned reverse indexes during result creation for ASM address, PcodeOp ID, Varnode ID, and Clang node ID relations.
- **Regression risks:** Exposing maps increases result size; store compact IDs and preserve vector data for serialization compatibility.
- **Relevant validation:** Focused tests validate reverse index membership, operation address consistency, node IDs, and instruction lengths.

### PROV-MEDIUM-004: Provenance markup is generated unconditionally by a second printer pass

- [x] **Remediation status:** Fixed in the current working tree.
- **Severity:** Medium.
- **Title:** Every decompilation pays for a second complete Clang print even when no viewer needs provenance.
- **Source reference:** `src/decompiler_impl.cppm:2429-2450`.
- **Affected component:** Decompiler latency and all existing `Decompiler::decompile` callers.
- **Technical evidence before remediation:** The normal printer emitted `c_source`, then a fresh `PrintLanguage` always emitted markup.
- **Expected behavior:** Provenance capture should be opt-in or generated by one shared pass when requested, while ordinary consumers should retain the previous cost profile.
- **Actual behavior after remediation:** `FunctionDescription::capture_provenance` defaults to `false`; only Viewer-oriented callers request the second markup/index pass.
- **Impact:** Batch decompilation, CLI use, and analyzers incur additional CPU and memory cost even when only plain C or P-code is requested.
- **Reproduction/failure scenario:** Decompile a large corpus through an existing caller that reads only `c_source`; each function still allocates and serializes a full XML markup document.
- **Root cause before remediation:** The viewer-oriented artifact was added directly to the default result path.
- **Implemented fix:** Added the explicit `capture_provenance` request flag while preserving plain C behavior for existing callers.
- **Regression risks:** Default behavior must continue to provide existing fields unchanged; viewer callers must request provenance before decompilation starts.
- **Relevant validation:** Full decompiler target passes; provenance tests explicitly request capture.

### PROV-MEDIUM-005: The requested constructs are split across five functions rather than one integrated example

- [x] **Remediation status:** Fixed in the current working tree.
- **Severity:** Medium.
- **Title:** Cross-construct interactions are not covered by the provenance test.
- **Source reference:** `tests/provenance_mapping_tests.cppm:386-516` (five fixture functions) and `:523-528` (five independent results).
- **Affected component:** End-to-end provenance coverage for realistic decompiled functions.
- **Technical evidence before remediation:** Nested member access, loop/local, if/else, switch, and call were each decompiled as separate compact functions.
- **Expected behavior:** At least one integrated fixture should exercise provenance when control-flow structuring and typed member/call propagation coexist, because those interactions are where token anchors and PcodeOp ownership commonly diverge.
- **Actual behavior after remediation:** `IntegratedControlFlowFixture` decompiles one real x86-64 body containing the guard, loop, switch/cases, nested typed field load, call argument, local, parameters, and arithmetic, and checks both directions through the same markup graph.
- **Impact:** The test does not establish that a viewer can navigate a realistic complex function or that operation IDs remain correct across nested blocks and multiple constructs in one markup document.
- **Reproduction/failure scenario:** Add a member load inside a switch case inside a loop and a call in one branch. No current assertion exercises the resulting nested markup hierarchy or shared Pcode origins.
- **Root cause before remediation:** The available existing fixtures isolated features, and the test stopped at a fixture set rather than adding an integrated adaptation.
- **Implemented fix:** Added an adapted integrated x86-64 body with an outer guard/loop, recovered indirect switch table, typed nested field access passed into a direct callee, locals, parameters, and arithmetic.
- **Regression risks:** Integrated assembly must be validated against actual Sleigh control-flow recovery; do not weaken structural assertions to accommodate an invalid fixture.
- **Relevant validation:** `IntegratedControlFlowFixture` and the complete decompiler suite pass.

## Low Findings

### PROV-LOW-001: The test parser is a permissive hand-written XML scanner

- [x] **Remediation status:** Fixed in the current working tree by using the production XML parser.
- **Severity:** Low.
- **Title:** Malformed markup can be accepted by the test parser.
- **Source reference:** `tests/provenance_mapping_tests.cppm:148-273`.
- **Affected component:** Reliability of test-only markup interpretation.
- **Technical evidence before remediation:** The test parser found the next `>` and popped stacks without checking element names or XML validity.
- **Expected behavior:** Test markup should be parsed by the repository's XML parser or should validate tag names, nesting, and malformed input explicitly.
- **Actual behavior after remediation:** The production capture path parses the markup with `pugixml` and rejects malformed XML before returning the result. The test consumes structured production records.
- **Impact:** False confidence in future markup format changes; failures may be delayed until a viewer uses a construct outside the current fixtures.
- **Root cause before remediation:** A dependency-free parser was added to the test for convenience.
- **Implemented fix:** Reused the repository's `pugixml` dependency in the production capture path and removed the permissive test parser.
- **Regression risks:** XML parser integration may add a test dependency but should not change production markup.
- **Relevant validation:** Existing XML tests and the complete decompiler target pass.

### PROV-LOW-002: Provenance key uniqueness and serialized P-code consistency are not asserted

- [x] **Remediation status:** Fixed for key uniqueness and reference integrity; raw diagnostic text parity remains intentionally outside the Viewer contract.
- **Severity:** Low.
- **Title:** The test resolves the first matching key and does not validate key integrity or P-code text parity.
- **Source reference:** `tests/provenance_mapping_tests.cppm:275-293` and `:530-541`.
- **Affected component:** Snapshot consistency checks.
- **Technical evidence before remediation:** `find_operation` and `find_varnode` returned the first matching record without key uniqueness checks.
- **Expected behavior:** Native identity keys should be unique within a result, every emitted reference should resolve exactly once, and representative snapshot fields should agree with the corresponding serialized P-code records.
- **Actual behavior after remediation:** The test asserts unique Pcode/Varnode identities, stable node IDs, complete node-to-operation reverse links, and instruction-to-operation address consistency. Raw diagnostic text parity remains intentionally unspecified.
- **Impact:** A viewer may receive inconsistent or ambiguous IDs without a test failure.
- **Root cause before remediation:** The test focused on selected construct presence rather than complete result invariants.
- **Implemented fix:** Added uniqueness and structural reverse-link assertions over the value-owned provenance graph.
- **Regression risks:** Serialized P-code is a diagnostic format and may not have a stable grammar; prefer structural parsing over exact string comparison.
- **Relevant validation:** Focused and full decompiler suites pass with the new invariants.

## Verified Strengths

- [x] The production markup path is the real `EmitMarkup` implementation; it is not generated by parsing rendered C text (`src/decompiler_impl.cppm:2500-2520`).
- [x] `opref` is correctly tied to native `PcodeOp::getTime()` and `varref` to native `Varnode::getCreateIndex()` (`src/prettyprint.cppm:164-205`, `:236-249`, `:299-309`).
- [x] Native PcodeOps and Varnodes are captured from live `Funcdata` (`src/decompiler_impl.cppm:269-313`).
- [x] The nested field test checks both field offsets and the `obj` HighVariable name (`tests/provenance_mapping_tests.cppm:430-460`).
- [x] Existing functionality remained green; provenance capture is now explicitly opt-in through `FunctionDescription::capture_provenance`.

## Validation Results

- [x] Focused test passed: `DecompilerProvenance.BidirectionalSleighMarkupAndNativeGraph`.
- [x] Focused decompiler CTest subset passed: 6/6.
- [x] Full `build.bat all` integration validation passed: 49/49 tests.
- [x] Formatting completed with `format.bat decompiler`.
- [x] `tidy.bat decompiler --check` completed without applied fixes.
- [ ] **Static-analysis limitation:** clang-tidy processed 0 translation units because all 178 C++ module consumers were skipped by the MSVC `.ifc` limitation; this is not equivalent to a clean analyzed pass.
- [x] `git diff --check` reported no whitespace errors.

## Unresolved Questions and Residual Risks

- [x] The production contract now exposes the complete set while retaining individual PcodeOp identities; the Viewer can choose primary or full highlighting.
- [x] The Viewer can consume `clang_nodes` directly; `clang_markup` remains available for serialization/debugging.
- [ ] `case`/`default` are linked; policy for other punctuation and non-semantic syntax remains a Viewer design choice.
- [x] Node ID lifetime is documented as one `DecompilationResult`; IDs are not promised to be globally stable across separate decompilations.

## Final Follow-Up Decision

- [x] The core reverse/forward provenance contract is now suitable as the Viewer integration baseline.
- [x] The integrated multi-construct fixture from `PROV-MEDIUM-005` is now present and passing.
- [x] No confirmed pre-existing runtime regression was found during this review; the findings are coverage and API-contract defects/risks that can allow Viewer mapping failures.
