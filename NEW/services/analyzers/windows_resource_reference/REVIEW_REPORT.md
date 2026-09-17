# Review Report: Windows Resource Reference

- [x] Scope confirmed: [`src/windows_resource_reference.cppm`](src/windows_resource_reference.cppm), [`tests/windows_resource_reference_tests.cppm`](tests/windows_resource_reference_tests.cppm), `.rc`/header/fixture script/report, [`CMakeLists.txt`](CMakeLists.txt), [`build.bat`](build.bat), [`README.md`](README.md), and [`GHIDRA_PORT.md`](GHIDRA_PORT.md).
- [x] Original sources inspected: `Ghidra/Features/MicrosoftCodeAnalyzer/src/main/java/ghidra/app/plugin/prototype/MicrosoftCodeAnalyzerPlugin/WindowsResourceReferenceAnalyzer.java` and `Ghidra/Features/Decompiler/ghidra_scripts/WindowsResourceReference.java`.
- [x] Reviewed diff: working tree at 2026-09-15; no commit supplied.
- [x] Reviewer: Kilo.
- [x] Source, test, fixture, documentation, CMake, and wrapper inspection completed.
- [x] Validation status recorded honestly: no build, CTest, formatter, or tidy command was run because this was a read-only audit.

## Findings

### Critical

No findings.

### High

#### RESOURCE-HIGH-001: Decompiler call/argument analysis is replaced by a broad historical-operand scan

- [ ] Remediation status: open.
- Severity: high.
- Affected component: [`src/windows_resource_reference.cppm#L142-L193`](src/windows_resource_reference.cppm#L142-L193) and [`#L219-L249`](src/windows_resource_reference.cppm#L219-L249).
- Technical evidence: original script `WindowsResourceReference.java#L364-L423` obtains each `HighFunction`, identifies the called API, and follows the specific P-code argument at the API's exact parameter index. Native `candidate_ids` scans every immediate/address operand before the call plus every earlier constant fact, reverses them, and accepts the first resource match. This can use unrelated constants or calls and is not restricted to the API argument.
- Expected behavior: recover only the resource ID argument defined by the original API table, including decompiler constant propagation and the original call-site/function scope.
- Actual behavior: any earlier matching integer can produce a DATA reference for the current external call.
- Impact: false resource references and missed references when the ID is recovered through P-code rather than printed as an immediate.
- Root cause: the native code substitutes a heuristic instruction-history scan for the `HighFunction`/P-code API.
- Recommended fix: expose decompiler call/argument value recovery and port the script's exact argument-index table and `analyzeFunction` flow.
- Regression risks: decompiler recovery can expose multiple possible constants; preserve the original checked-property/constant semantics and test nested calls.
- Relevant tests or validation: the native test disassembles one fixture entry and checks two string references, not unrelated prior constants or API argument positions.

#### RESOURCE-HIGH-002: String-table target addresses do not match the original script

- [ ] Remediation status: open.
- Severity: high.
- Affected component: [`src/windows_resource_reference.cppm#L84-L112`](src/windows_resource_reference.cppm#L84-L112) and [`tests/windows_resource_reference_tests.cppm#L16-L39`](tests/windows_resource_reference_tests.cppm#L16-L39).
- Technical evidence: native `string_table_address` returns `base + offset` at the length prefix of the selected entry. The original script's `findResource`/string-table routine (`WindowsResourceReference.java#L547-L610`) resolves the loader-created data symbol for the selected string entry. The checked-in original report records targets `0x1400051DA` and `0x140005212` at [`tests/data/test_windows_resource_reference.md#L42-L47`](tests/data/test_windows_resource_reference.md#L42-L47), while the native test asserts `0x1400051D8` and `0x1400051DA` at lines 32-38.
- Expected behavior: references target the same loader-created per-string data locations as the original script.
- Actual behavior: native tests bless a different pair of addresses and the implementation returns raw payload offsets rather than the original data-symbol targets.
- Impact: references point at the wrong string-table record/content and the native test can pass while diverging from the original fixture.
- Root cause: PE payload offsets were treated as equivalent to Ghidra loader data-symbol addresses.
- Recommended fix: define the native resource-data address contract from the original loader symbols and compare exact address/reference results against the original report.
- Regression risks: resource layout, empty strings, locale, and table padding affect offsets; add tables with empty and non-empty entries.
- Relevant tests or validation: current native expectations directly contradict the generated original report.

### Medium

#### RESOURCE-MEDIUM-001: API-family mapping and table semantics are not exact

- [ ] Remediation status: open.
- Severity: medium.
- Affected component: [`src/windows_resource_reference.cppm#L32-L73`](src/windows_resource_reference.cppm#L32-L73) and [`#L128-L133`](src/windows_resource_reference.cppm#L128-L133).
- Technical evidence: original `WindowsResourceReference.java#L135-L225` distinguishes direct resource lookups from `addResourceTableReferences`; `LoadMenuA` uses the menu table path while `LoadMenuW` uses the direct resource path. Native maps both through the same `Menu` family and also uses broad `find("dialog")`, `find("menu")`, and `find("cursor")` name matching. Native generic lookup excludes type ID 6, a restriction not equivalent to the script's wildcard `findResource` behavior.
- Expected behavior: exact API-name, parameter-index, resource-type, direct/table, locale, and wildcard rules from the script.
- Actual behavior: broad name matching and one generic resolver are used for multiple distinct original paths.
- Impact: APIs can resolve wrong resources or fail for supported calls.
- Root cause: the script's routine table was compressed into a family string without preserving operation mode.
- Recommended fix: port the routine table and separate direct/resource-table/wildcard lookup operations.
- Regression risks: expanding supported APIs may expose duplicate references; test every original routine family and unsupported API.
- Relevant tests or validation: fixture covers only two `LoadStringW` calls despite the original script supporting many APIs.

#### RESOURCE-MEDIUM-002: Native tests and fixture oracle validate different boundaries

- [ ] Remediation status: open.
- Severity: medium.
- Affected component: [`tests/windows_resource_reference_tests.cppm`](tests/windows_resource_reference_tests.cppm), [`tests/data/run_ghidra.py`](tests/data/run_ghidra.py), and [`CMakeLists.txt#L6-L13`](CMakeLists.txt#L6-L13).
- Technical evidence: CMake registers only GoogleTest. The PyGhidra script runs the original analyzer/script and writes the generated report; it is not invoked by CTest. Native tests assert native resource offsets and do not compare the before/after DATA-reference delta from the report.
- Expected behavior: native parity tests must validate exact call selection, address, reference type/source, duplicate handling, locale, and resource-table semantics.
- Actual behavior: a Java report and native-specific address assertions coexist without an automated comparison.
- Impact: fixture evidence can expose a mismatch without failing native tests.
- Root cause: the oracle is not connected to the native test graph and the resource model differs.
- Recommended fix: add native expected-resource metadata and a parity comparison, while keeping wrapper-driven Java validation separate.
- Regression risks: resource symbols may be loader setup rather than analyzer changes; snapshot before/after state as the report does.
- Relevant tests or validation: no commands were run during this audit.

### Low

No findings.

## Verified Strengths

- [x] Resource leaf/type filtering, string-table arithmetic, duplicate reference suppression, cancellation, and analysis bookmarks are explicit.
- [x] The `.rc`, resource header, fixture build, script, and generated report are all present.

## Reviewed Areas With No Findings

- [x] Exact moved-source issue was resolved for the unrelated ClearFlow/CallFixup audit; this module's original script path is present and inspected.
- [x] No implementation, test, fixture, documentation, or build source was changed.

## Validation Results

- [x] Read-only original/native comparison completed.
- [x] Read-only test, fixture, documentation, CMake, and wrapper inspection completed.
- [ ] Focused build/test: not run by request.
- [ ] `format.bat windows_resource_reference`: not run by request.
- [ ] `tidy.bat windows_resource_reference --check`: not run by request.

## Unresolved Questions

- [ ] The native loader's canonical address for an individual string-table item must be specified before exact parity can be implemented.

## Residual Risks

- [ ] Locale selection and named-resource paths are not covered by the native tests.

## Follow-Up

- [ ] Highest proposed remediations: RESOURCE-HIGH-001 and RESOURCE-HIGH-002.
- [ ] Medium proposed remediations: RESOURCE-MEDIUM-001 and RESOURCE-MEDIUM-002.
- [ ] Final follow-up decision: keep implementation unchanged until exact script/loader parity is authorized.
