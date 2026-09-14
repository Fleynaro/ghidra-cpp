# Review Report: Microsoft Demangler

- [x] Scope confirmed: [`src/demangler_microsoft.cppm`](src/demangler_microsoft.cppm), [`tests/demangler_microsoft_tests.cppm`](tests/demangler_microsoft_tests.cppm), fixture source/script/report, [`CMakeLists.txt`](CMakeLists.txt), [`build.bat`](build.bat), [`README.md`](README.md), and [`GHIDRA_PORT.md`](GHIDRA_PORT.md).
- [x] Original sources inspected: `Ghidra/Features/MicrosoftDemangler/src/main/java/ghidra/app/util/demangler/microsoft/MicrosoftDemangler.java`, `MicrosoftMangledContext.java`, `MicrosoftDemanglerOptions.java`, `MicrosoftDemanglerUtil.java`, `MsCInterpretation.java`, all `mdemangler` implementation classes under that source tree, and `Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/analysis/AbstractDemanglerAnalyzer.java`.
- [x] Original tests inspected: `Ghidra/Features/MicrosoftDemangler/src/test/java/ghidra/app/util/demangler/microsoft/MicrosoftDemanglerTest.java`, `MicrosoftDemanglerExtraTest.java`, and `MicrosoftDemanglerAnalyzerTest.java`.
- [x] Reviewed diff: working tree at 2026-09-15; no commit supplied.
- [x] Reviewer: Kilo.
- [x] Source, test, fixture, documentation, CMake, and wrapper inspection completed.
- [x] Validation status recorded honestly: no build, CTest, formatter, or tidy command was run because this was a read-only audit.

## Findings

### Critical

#### DEMANGLER-CRITICAL-001: The complete mdemangler implementation is absent

- [ ] Remediation status: open.
- Severity: critical.
- Affected component: [`src/demangler_microsoft.cppm`](src/demangler_microsoft.cppm#L45-L345).
- Technical evidence: the native module is one handwritten parser with a small primitive-type switch and broad `unknown` fallback. The original MicrosoftDemangler module delegates to the `mdemangler` object hierarchy and its parser/context/options classes; the original tree contains the full grammar, object model, back-reference handling, output options, and demangled-object construction. `MicrosoftDemanglerOptions.java#L53-L173` exposes remaining-character policy, C-style interpretation, anonymous namespace output, and UDT tag output that the native API does not model.
- Expected behavior: parse the supported MSVC decorated-name grammar and produce the original `DemangledObject` hierarchy, exact spelling, context-sensitive C-style interpretation, errors, back-references, anonymous namespaces, operators, templates, special members, function namespaces, and option-controlled output.
- Actual behavior: many inputs are accepted as `valid` after finding `@@`, unsupported productions become `unknown`, and only a compact `MicrosoftDemangledSymbol` is returned.
- Impact: this is not a complete mdemangler port; real symbols can be misnamed, mis-typed, or silently accepted with fabricated output.
- Reproduction or failure scenario: original `MicrosoftDemanglerExtraTest.java#L181-L231` exercises anonymous-namespace/vftable back-references and function namespaces; there is no native representation for the expected object hierarchy or exact output. The original C-style tests at `#L242-L450` also require architecture/function context that the native `demangle(std::string_view)` API does not receive.
- Root cause: the original parser/object package was replaced by an intentionally smaller standalone grammar rather than ported.
- Recommended fix: port the complete `mdemangler` package and context/options contract before labeling this module equivalent; keep the compact parser under a clearly different partial API if needed.
- Regression risks: exact demangled spelling and signature application are highly compatibility-sensitive; port original tests before changing analyzer integration.
- Relevant tests or validation: native tests cover only `?add@...` and `?scale@...` in [`tests/demangler_microsoft_tests.cppm#L14-L59`](tests/demangler_microsoft_tests.cppm#L14-L59).

### High

#### DEMANGLER-HIGH-001: Analyzer scope and application semantics differ from AbstractDemanglerAnalyzer

- [ ] Remediation status: open.
- Severity: high.
- Affected component: [`src/demangler_microsoft.cppm#L347-L405`](src/demangler_microsoft.cppm#L347-L405).
- Technical evidence: original `AbstractDemanglerAnalyzer.java#L87-L174` iterates primary and non-primary symbol-table symbols, processes memory symbols before externals, applies skip/source/namespace rules, and calls `DemangledObject.applyTo`. Native code visits only `context.external_symbols()` and PE exported symbols at lines 393-404, then applies `parsed.short_name`; it does not process arbitrary decorated memory symbols or alternate symbols. At line 369 both branches return `parsed.short_name`, so `demangler_apply_namespaces` is ignored.
- Expected behavior: demangle all eligible primary/non-primary symbols in the requested set, preserve external/thunk relationships, apply exact demangled names/namespaces/signatures/calling conventions under options, and log invalid/error cases.
- Actual behavior: only imported symbols and non-forwarded exports are considered; namespace output and most demangled-object metadata are discarded.
- Impact: valid decorated symbols outside the PE export/IAT subsets remain unchanged or receive incomplete names/signatures.
- Root cause: the native context exposes a narrow symbol/export list and the analyzer was written around it instead of porting `AbstractDemanglerAnalyzer.apply` semantics.
- Recommended fix: add symbol-table iteration/source filtering and a full demangled-object application boundary; honor namespace/signature options and error policy.
- Regression risks: applying non-primary symbols can alter aliases and thunk names; use original analyzer tests for primary/alternate/external cases.
- Relevant tests or validation: the generated fixture report checks two exports only; no native test toggles namespace/signature options.

### Medium

#### DEMANGLER-MEDIUM-001: Native tests are basic smoke tests, not complete grammar or fixture parity tests

- [ ] Remediation status: open.
- Severity: medium.
- Affected component: [`tests/demangler_microsoft_tests.cppm`](tests/demangler_microsoft_tests.cppm), fixture script/report, and [`CMakeLists.txt#L6-L13`](CMakeLists.txt#L6-L13).
- Technical evidence: CMake registers only the native GoogleTest executable. `tests/data/run_ghidra.py` generates a Java-Ghidra report, and the native tests do not parse that report. The native assertions cover two ordinary x64 functions and do not cover invalid input, operators, templates, anonymous namespaces, C-style symbols, back-references, non-primary symbols, or option changes.
- Expected behavior: tests must establish exact original output and application for the grammar and analyzer cases claimed by the port.
- Actual behavior: a parser can pass all native tests while omitting most original functionality.
- Impact: test validity is limited to the two selected examples.
- Root cause: the original broad Java test corpus was not ported and the fixture oracle is not connected to CTest.
- Recommended fix: port the original demangler vectors and add native analyzer tests for every supported context/option category.
- Regression risks: exact spelling changes can be subtle; compare both structured fields and rendered output.
- Relevant tests or validation: no commands were run during this audit.

### Low

No findings.

## Verified Strengths

- [x] The native parser handles the two fixture symbols and preserves explicit source references.
- [x] CMake, fixture build, and wrapper files are present.

## Reviewed Areas With No Findings

- [x] No source/build edit was made during the audit.
- [x] The generated Java fixture report was inspected as original behavior evidence, not native validation.

## Validation Results

- [x] Read-only original/native source comparison completed.
- [x] Read-only test, fixture, documentation, CMake, and wrapper inspection completed.
- [ ] Focused build/test: not run by request.
- [ ] `NEW/format.bat demangler_microsoft`: not run by request.
- [ ] `NEW/tidy.bat demangler_microsoft --check`: not run by request.

## Unresolved Questions

- [ ] The complete native equivalent of the original `mdemangler` object and output-option boundary is not present.

## Residual Risks

- [ ] Unsupported productions are currently represented as successful `unknown` components rather than original demangle failures.

## Follow-Up

- [ ] Highest proposed remediation: DEMANGLER-CRITICAL-001.
- [ ] High proposed remediation: DEMANGLER-HIGH-001.
- [ ] Medium proposed remediation: DEMANGLER-MEDIUM-001.
- [ ] Final follow-up decision: keep implementation unchanged until complete-parser parity is authorized.
