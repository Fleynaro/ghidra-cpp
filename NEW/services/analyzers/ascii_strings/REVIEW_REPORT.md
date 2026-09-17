# Review Report: ASCII Strings

- [x] Scope confirmed: [`src/ascii_strings.cppm`](src/ascii_strings.cppm), [`tests/ascii_strings_tests.cppm`](tests/ascii_strings_tests.cppm), fixture source/script/report, [`CMakeLists.txt`](CMakeLists.txt), [`build.bat`](build.bat), [`README.md`](README.md), and [`GHIDRA_PORT.md`](GHIDRA_PORT.md).
- [x] Original sources inspected: `Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/string/StringsAnalyzer.java`, `Ghidra/Features/Base/src/main/java/ghidra/program/util/string/StringSearcher.java`, `AbstractStringSearcher.java`, `Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/string/NGramUtils.java`, `StringAndScores.java`, and `Ghidra/Features/Base/src/main/java/ghidra/util/ascii/AsciiCharSetRecognizer.java`.
- [x] Reviewed diff: working tree at 2026-09-15; no commit supplied.
- [x] Reviewer: Kilo.
- [x] Source, test, fixture, documentation, CMake, and wrapper inspection completed.
- [x] Validation status recorded honestly: no build, CTest, formatter, or tidy command was run because this was a read-only audit.

## Findings

### Critical

No findings.

### High

#### ASCII-HIGH-001: Statistical string-model acceptance is omitted

- [ ] Remediation status: open.
- Severity: high.
- Affected component: [`src/ascii_strings.cppm#L54-L91`](src/ascii_strings.cppm#L54-L91).
- Technical evidence: the native implementation explicitly bypasses `NGramUtils` and accepts a candidate when it contains one alphanumeric byte (`has_text_character`) at lines 54-59. Original `StringsAnalyzer.java#L289-L299` starts the model session and `#L390-L401` normalizes, scores, and rejects candidates below the model threshold; `NGramUtils.java#L117-L145` and `#L425-L501` implement model loading, length thresholds, trigram scoring, and threshold comparison.
- Expected behavior: model selection/reload, ASCII normalization, minimum scored length, trigram score, and length-specific threshold must determine whether a found sequence becomes a string.
- Actual behavior: any otherwise valid byte sequence containing a letter or digit is accepted without a model file, score, or threshold.
- Impact: false positives and false negatives differ from the original analyzer; the negative controls do not exercise model rejection.
- Reproduction or failure scenario: a null-terminated printable sequence made mostly of punctuation but containing one digit passes the native `has_text_character` gate although the original `StringAndScores`/`NGramUtils` decision may reject it.
- Root cause: no native equivalent of `NGramUtils`, `StringAndScores`, or the `.sng` model is connected to `analyze`.
- Recommended fix: port the model/session/normalization/scoring contract or explicitly expose the port as non-equivalent and add parity vectors from the original model.
- Regression risks: changing acceptance can alter existing data ownership and overlap outcomes; add model-specific golden cases before enabling it by default.
- Relevant tests or validation: [`tests/ascii_strings_tests.cppm#L15-L45`](tests/ascii_strings_tests.cppm#L15-L45) checks only two ordinary strings and three byte negatives, not statistical decisions.

### Medium

#### ASCII-MEDIUM-001: StringSearcher width and matcher behavior is incomplete

- [ ] Remediation status: open.
- Severity: medium.
- Affected component: [`src/ascii_strings.cppm#L100-L172`](src/ascii_strings.cppm#L100-L172).
- Technical evidence: current code reads raw bytes and recognizes only one-byte ASCII at lines 118-130. Original `StringSearcher.java#L26-L56` delegates to `AbstractStringSearcher`, whose `#L35-L87` constructs UTF-8, UTF-16, and UTF-32 matchers with endian/alignment variants. The current `AnalysisOptions` has no equivalent width-search contract even though the original search model carries it internally.
- Expected behavior: use the original matcher boundary, memory-buffer traversal, alignment variants, termination semantics, and data type width when the supported width mode is selected.
- Actual behavior: only one-byte sequences can be produced and every candidate is stored with width `1` at line 167.
- Impact: the port cannot reproduce the complete string-search model or any UTF-16/UTF-32 search invocation.
- Reproduction or failure scenario: a valid wide-character sequence in a program with the original all-character-width search mode has no native equivalent and produces no `StringRecord`.
- Root cause: the port collapsed `StringSearcher` and all matcher classes into a byte loop.
- Recommended fix: preserve the matcher abstraction and width-specific `StringRecord` creation, or clearly constrain the public contract to ASCII-only and test that limitation.
- Regression risks: width-aware matching can create overlapping candidates; parity tests must cover endian, alignment, and terminator offsets.
- Relevant tests or validation: fixture source [`tests/data/test_ascii_strings.cpp`](tests/data/test_ascii_strings.cpp) contains only byte strings; no width test exists.

#### ASCII-MEDIUM-002: Fixture and CTest do not validate original analyzer behavior

- [ ] Remediation status: open.
- Severity: medium.
- Affected component: [`tests/ascii_strings_tests.cppm`](tests/ascii_strings_tests.cppm), [`tests/data/run_ghidra.py`](tests/data/run_ghidra.py), and [`CMakeLists.txt#L6-L13`](CMakeLists.txt#L6-L13).
- Technical evidence: CMake registers only the native GoogleTest executable. The PyGhidra script generates [`tests/data/test_ascii_strings.md`](tests/data/test_ascii_strings.md) by running Java Ghidra and is not invoked by CTest. Native tests directly call the replacement analyzer and assert only two `StringRecord` values.
- Expected behavior: a parity test must compare the native result with the original result for model acceptance, data conflicts, references, alignment, and cancellation.
- Actual behavior: the generated report is an unconnected Java oracle, while the native test can pass despite the omitted model.
- Impact: a green focused test would not establish equivalence and can hide the central mismatch.
- Root cause: fixture execution is documented but not wired into the CMake test graph, and native assertions are narrower than the original contract.
- Recommended fix: add deterministic native model/parity vectors and a separately gated wrapper-driven oracle test that reports unavailable environments rather than silently passing.
- Regression risks: PyGhidra tests are environment-dependent; keep native tests independent and label oracle tests explicitly.
- Relevant tests or validation: no commands were run during this audit.

### Low

No findings.

## Verified Strengths

- [x] The native code has explicit initialized-memory, alignment, null-termination, instruction-overlap, existing-data, reference, cancellation, and end-padding gates.
- [x] Fixture source, generated report, CMake target, and module wrapper are present and navigable.

## Reviewed Areas With No Findings

- [x] ASCII fixture compilation wrapper was inspected; no source edit was made.
- [x] The local CMake target and test registration were inspected; validation was not executed.

## Validation Results

- [x] Read-only original/native source comparison completed.
- [x] Read-only test, fixture, documentation, CMake, and wrapper inspection completed.
- [ ] Focused build/test: not run by request.
- [ ] `format.bat ascii_strings`: not run by request.
- [ ] `tidy.bat ascii_strings --check`: not run by request.

## Unresolved Questions

- [ ] The native public model API needed to load `StringModel.sng` and reproduce `NGramUtils` is not present in this module.

## Residual Risks

- [ ] Existing-data clearing and precise source/reference precedence may diverge outside the fixture's three negatives.

## Follow-Up

- [ ] Highest proposed remediation: ASCII-HIGH-001.
- [ ] Medium proposed remediations: ASCII-MEDIUM-001 and ASCII-MEDIUM-002.
- [ ] Final follow-up decision: keep implementation unchanged until parity remediation is authorized.
