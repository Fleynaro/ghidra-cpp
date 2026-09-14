# Ghidra Port Evidence

- Analyzer base: [`Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/analysis/AbstractDemanglerAnalyzer.java`](../../../../Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/analysis/AbstractDemanglerAnalyzer.java).
- Microsoft parser package: [`Ghidra/Features/MicrosoftDemangler/src/main/java/ghidra/app/util/demangler/microsoft/`](../../../../Ghidra/Features/MicrosoftDemangler/src/main/java/ghidra/app/util/demangler/microsoft/).
- Native implementation: [`src/demangler_microsoft.cppm`](src/demangler_microsoft.cppm).
- Native tests: [`tests/demangler_microsoft_tests.cppm`](tests/demangler_microsoft_tests.cppm).
- Fixture evidence: [`tests/data/test_demangler_microsoft.md`](tests/data/test_demangler_microsoft.md).

The repository contains the original Java mdemangler grammar but no native mdemangler library.
The module therefore implements the grammar in one autonomous C++23 module rather than a handful
of hard-coded names. It covers Microsoft name scopes, template components, special member names,
primitive/extended types, tagged types, cv-qualified pointers, references, type back references,
member/static/global calling conventions, and implicit member receivers.

The analyzer preserves name/signature application, `Demangler Microsoft` identity, priority `897`,
and external-symbol updates. `AnalysisContext` has no namespace tree or plate-comment API, so the
qualified namespace is retained in `SymbolRecord::namespace_name` while the primary display name is
the original leaf name. The native context's existing `set_function_signature` is used for all
recoverable local functions; unsupported type productions remain valid demangled names with an
`unknown` type instead of aborting the complete symbol pass.
