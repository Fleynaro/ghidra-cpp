# Ghidra Port Evidence

## Source And Scope

- Original analyzer: [`Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/string/StringsAnalyzer.java`](../../../../Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/string/StringsAnalyzer.java).
- Original searcher: [`Ghidra/Features/Base/src/main/java/ghidra/program/util/string/StringSearcher.java`](../../../../Ghidra/Features/Base/src/main/java/ghidra/program/util/string/StringSearcher.java).
- Original recognizer: [`Ghidra/Features/Base/src/main/java/ghidra/util/ascii/AsciiCharSetRecognizer.java`](../../../../Ghidra/Features/Base/src/main/java/ghidra/util/ascii/AsciiCharSetRecognizer.java).
- Native implementation: [`src/ascii_strings.cppm`](src/ascii_strings.cppm).
- Native tests: [`tests/ascii_strings_tests.cppm`](tests/ascii_strings_tests.cppm) and the fixture report [`tests/data/test_ascii_strings.md`](tests/data/test_ascii_strings.md).

## Preserved Behavior

- Analyzer name, default enablement, priority `905`, minimum length floor `4`, default minimum length `5`, null termination, ASCII character set, start/end alignment validation, initialized-memory scanning, instruction conflict rejection, existing-data rejection when configured, and string data size including the terminator are preserved.
- String creation uses `AnalysisContext::add_string`, which performs the existing non-overlap and data-event state transition.

## Explicit Limitation

The repository does not expose a public native equivalent of `NGramUtils`, `StringModel`, or its
`.sng` probability loader. The port therefore retains the recognizer and all listing gates but does
not apply the statistical n-gram score. This is intentionally documented rather than represented as
equivalent behavior. The checked-in fixture's two accepted strings and three negative controls are
asserted directly by the focused test.

The native `AnalysisContext` also has no clear-data command or reference-property map, so existing
substring replacement and one-time property-map lifecycle behavior cannot be performed. Candidates
that overlap existing native data are rejected by the public `add_data` contract.
