# Strict Audit Report: Variadic Function Signature Override

- [x] Scope confirmed: [`src/v.cppm`](src/v.cppm), [`tests/t.cppm`](tests/t.cppm), [`tests/data/run_ghidra.py`](tests/data/run_ghidra.py), [`tests/data/test_variadic_function_signature_override.md`](tests/data/test_variadic_function_signature_override.md), [`CMakeLists.txt`](CMakeLists.txt), and the checked-in PE/PDB fixtures.
- [x] Exact Ghidra sources inspected: [`FormatStringAnalyzer.java`](../../../../Ghidra/Features/DecompilerDependent/src/main/java/ghidra/app/plugin/core/string/variadic/FormatStringAnalyzer.java), [`FormatStringParser.java`](../../../../Ghidra/Features/DecompilerDependent/src/main/java/ghidra/app/plugin/core/string/variadic/FormatStringParser.java), and [`PcodeFunctionParser.java`](../../../../Ghidra/Features/DecompilerDependent/src/main/java/ghidra/app/plugin/core/string/variadic/PcodeFunctionParser.java).
- [x] Review date: 2026-09-15. No commit was supplied; the working tree was audited as-is.
- [x] Reviewer: Kilo.
- [x] Review method: source-to-source comparison, parser/analyzer test inspection, fixture assertion inspection, and CMake/test-registration inspection.
- [ ] Native build, test, formatter, and tidy commands were not run because this was requested as a strict read-only audit.

## Findings

### Critical

No findings.

### High

#### HIGH-001: Call-site signature overrides are not applied

- [ ] Remediation status: open.
- Severity: high.
- Title: the native analyzer records a bookmark but never performs the primary `HighFunctionDBUtil.writeOverride` operation.
- Exact references: [`src/v.cppm:240-253`](src/v.cppm#L240-L253), symbol `VariadicFunctionSignatureOverrideAnalyzer::analyze`; Ghidra [`FormatStringAnalyzer.java:327-363`](../../../../Ghidra/Features/DecompilerDependent/src/main/java/ghidra/app/plugin/core/string/variadic/FormatStringAnalyzer.java#L327-L363).
- Affected component: decompiler call-site signatures and downstream type propagation.
- Technical evidence: Ghidra creates a full `FunctionSignature`, adds the optional bookmark, and calls `HighFunctionDBUtil.writeOverride(function, address, functionSignature)`. Native creates only a `Function Signature Override` bookmark containing a type summary; the source explicitly comments that no call-site override table exists and performs no equivalent mutation.
- Expected behavior: each valid format-string call should receive a persistent call-site signature override while the callee's fixed/variadic signature remains intact.
- Actual behavior: native state contains only an informational bookmark. Decompiler calls retain their original variadic signature and no parameter definitions are stored at the call site.
- Impact: the analyzer's user-visible purpose is absent; consumers cannot use inferred argument types, and the native output cannot match Ghidra's `HighFunctionDBUtil.readOverride` state.
- Reproduction or failure scenario: analyze a call to `fixture_printf` with `"%d %s"`; native adds a bookmark but has no API/state from which a call-site consumer could read `int` and `char *` parameters.
- Root cause: `AnalysisContext` has no call-site override table or mutation API, and the implementation deliberately stops at evidence recording.
- Recommended fix: add a first-class call-site override model keyed by caller/call address, persist the full return/fixed/variadic parameter signature, and expose it to decompiler consumers.
- Regression risks: overrides must be invalidated when function bodies/signatures change and must not mutate the persistent callee prototype.
- Relevant tests or validation: [`tests/t.cppm`](tests/t.cppm) tests only the parser; no native analyzer test can currently observe the missing side effect.

#### HIGH-002: Call discovery does not follow `PcodeFunctionParser`'s format-argument slot contract

- [ ] Remediation status: open.
- Severity: high.
- Title: `format_at_call` scans arbitrary call p-code inputs instead of resolving the target function, parameter count, defined strings, and offcuts as Ghidra does.
- Exact references: [`src/v.cppm:48-82`](src/v.cppm#L48-L82), symbol `read_string`/`format_at_call`; [`src/v.cppm:215-231`](src/v.cppm#L215-L231); Ghidra [`PcodeFunctionParser.java:55-125`](../../../../Ghidra/Features/DecompilerDependent/src/main/java/ghidra/app/plugin/core/string/variadic/PcodeFunctionParser.java#L55-L125), especially lines 63-105; Ghidra [`FormatStringAnalyzer.java:138-157`](../../../../Ghidra/Features/DecompilerDependent/src/main/java/ghidra/app/plugin/core/string/variadic/FormatStringAnalyzer.java#L138-L157).
- Affected component: call-site candidate selection and format-string association.
- Technical evidence: Java accepts only `PcodeOp.CALL`, resolves the function at input 0, requires more inputs than fixed parameters, reads the format argument at the exact fixed-parameter slot, supports defined-string offcuts, and separately handles hidden strings from a typed parameter. Native iterates every reference to a matching target and returns the first readable `%` string from any `const` or `ram` input of the listing instruction, with a fixed 4096-byte/ASCII-only reader.
- Expected behavior: the format literal must be associated with the target's format parameter and the call must have variadic arguments; hidden/offcut/typed-string cases must follow Java semantics.
- Actual behavior: an unrelated constant string in the same call can be selected, calls lacking the required variadic argument count can be processed, and wide/hidden/offcut formats are not equivalent.
- Impact: inferred signatures can be applied to the wrong call or with the wrong format, causing incorrect type propagation once an override API exists.
- Reproduction or failure scenario: include two literal pointer inputs to a call, where the first unrelated string contains `%` and the actual format parameter does not. Native selects the unrelated string because it scans inputs in order.
- Root cause: the native model exposes raw instruction p-code but not decompiler `PcodeOpAST`/high-variable provenance or function parameter slots.
- Recommended fix: expose structured decompiler call operations and use the target parameter count/type plus the defined-string/offcut lookup rules before parsing.
- Regression risks: stricter slot matching may remove current fixture hits until the fixture's call metadata is represented accurately.
- Relevant tests or validation: no current test covers multiple string inputs, missing variadic arguments, offcut strings, or hidden typed strings.

### Medium

#### MEDIUM-001: Format grammar and type mapping are incomplete relative to `FormatStringParser`

- [ ] Remediation status: open.
- Severity: medium.
- Title: the native parser accepts a different grammar and emits different argument types for valid Ghidra formats.
- Exact references: [`src/v.cppm:96-202`](src/v.cppm#L96-L202), symbol `parse_format_string`; Ghidra [`FormatStringParser.java:81-108`](../../../../Ghidra/Features/DecompilerDependent/src/main/java/ghidra/app/plugin/core/string/variadic/FormatStringParser.java#L81-L108), [`:122-175`](../../../../Ghidra/Features/DecompilerDependent/src/main/java/ghidra/app/plugin/core/string/variadic/FormatStringParser.java#L122-L175), [`:312-381`](../../../../Ghidra/Features/DecompilerDependent/src/main/java/ghidra/app/plugin/core/string/variadic/FormatStringParser.java#L312-L381), and [`:620-884`](../../../../Ghidra/Features/DecompilerDependent/src/main/java/ghidra/app/plugin/core/string/variadic/FormatStringParser.java#L620-L884).
- Affected component: printf/scanf conversion parsing and generated parameter types.
- Technical evidence: Java supports positional `%n$`/`*n$` operands, `C`/`S`, `q`, typedef-backed `j`/`z`/`t` mappings, conversion-pair validation, and output `*` width arguments. Native treats digits as width, rejects `C`/`S`/`q`, ignores several length semantics, and skips `*` width arguments instead of adding an `int` argument. It also uses `scanf_output` as a blanket `indirect` flag rather than preserving each conversion's pointer semantics.
- Expected behavior: valid formats accepted by Java must yield the same argument count, ordering, indirection, and data types; malformed pairs must fail consistently.
- Actual behavior: several valid Java formats are rejected or produce a different signature, while some unsupported combinations are accepted until later conversion handling.
- Impact: even after adding override persistence, format-derived signatures would differ on common positional, width, wide-character, and platform-length cases.
- Reproduction or failure scenario: parse `"%*d %d"` in printf mode. Java adds an `int` argument for the dynamic width; native consumes `*` without adding one. Parse a positional format such as `"%2$d %1$s"`; Java maps positions, native treats `2`/`1` as widths.
- Root cause: the native parser is a compact hand-written subset rather than a complete port of the state machine and datatype-manager mapping.
- Recommended fix: port the complete parser state machine and model platform typedef/data-type lookup instead of hard-coding spellings.
- Regression risks: existing fixture expectations should be retained while broad grammar tests are added.
- Relevant tests or validation: [`tests/t.cppm:10-49`](tests/t.cppm#L10-L49) covers four simple cases and one malformed trailing `%` only.

#### MEDIUM-002: The test and report prove Java behavior, not native override behavior

- [ ] Remediation status: open.
- Severity: medium.
- Title: the current CTest target has parser-only coverage, while the PyGhidra fixture manually prepares state and runs Ghidra's Java analyzer.
- Exact references: [`tests/t.cppm:10-49`](tests/t.cppm#L10-L49); [`tests/data/run_ghidra.py:395-452`](tests/data/run_ghidra.py#L395-L452); [`tests/data/run_ghidra.py:453-469`](tests/data/run_ghidra.py#L453-L469); [`tests/data/test_variadic_function_signature_override.md:39-85`](tests/data/test_variadic_function_signature_override.md#L39-L85); [`CMakeLists.txt:8-15`](CMakeLists.txt#L8-L15).
- Affected component: native regression infrastructure and fixture provenance.
- Technical evidence: the script runs PDB Universal, manually sets `printf.setVarArgs(True)` when needed, manually restores a call reference and string data, then invokes `FormatStringAnalyzer.added` from Ghidra. It reads `HighFunctionDBUtil` overrides and asserts only that a bookmark exists at runtime. CMake registers only `analyzer_variadic_function_signature_override_tests`, and no target executes the script or consumes its Markdown.
- Expected behavior: native tests should invoke `VariadicFunctionSignatureOverrideAnalyzer` and assert a stored override; reference fixtures should be explicitly marked Java-only or wired through the required wrapper.
- Actual behavior: the report's `0 / 1` override rows are Java results, and the native analyzer can remain a bookmark-only no-op while all current CTest tests pass.
- Impact: the core `HIGH-001` gap is not detected.
- Reproduction or failure scenario: remove any future native call-site mutation; the four parser tests remain green and the checked-in Java report remains unchanged.
- Root cause: no native call-site override model exists and fixture orchestration is separate from CTest.
- Recommended fix: add a native call-site fixture/state assertion and make the PyGhidra report generation an explicit, wrapper-launched reference test if retained.
- Regression risks: exact override text may be frontend-version-dependent; assert structured fields rather than prototype formatting.
- Relevant tests or validation: source inspection only; no commands were run in this audit.

### Low

No findings.

## Verified Strengths

- [x] The parser handles several ordinary printf/scanf conversions, escaped percent, suppression, length modifiers, and malformed trailing input.
- [x] Native analyzer selection requires a variadic character-pointer parameter and a printf/scanf-like name before scanning call references.
- [x] The current implementation avoids corrupting the persistent callee signature while the call-site API is absent.

## Reviewed Areas With No Findings

- [x] The local CMake target and parser GoogleTests are internally consistent.
- [x] The PE fixture contains a real variadic call with `%d` and `%s` arguments and is suitable for future native integration coverage.

## Validation

- [x] Original analyzer/parser/p-code parser, native source, native tests, fixture source/script/report, and CMake registration were inspected.
- [ ] Focused build/test was not run by this audit.
- [ ] `format.bat analyzer` and `tidy.bat analyzer --check` were not run by this audit.
- [x] No implementation, test, fixture, or build source was edited.

## Unresolved Questions And Residual Risks

- [ ] The native decompiler model needs a persistent call-site override boundary before full analyzer semantics can be implemented.
- [ ] Platform-specific typedef resolution (`size_t`, `ptrdiff_t`, `intmax_t`, and related types) is not represented by the current parser API.

## Follow-Up Decision

- [ ] Highest-priority open items are `HIGH-001` and `HIGH-002`; `MEDIUM-001` and `MEDIUM-002` remain required for exact grammar/test parity.
