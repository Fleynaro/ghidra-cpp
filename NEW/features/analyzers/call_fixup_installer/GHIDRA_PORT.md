# Ghidra Port Evidence

- Original: [`Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/disassembler/CallFixupAnalyzer.java`](../../../../Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/disassembler/CallFixupAnalyzer.java).
- Native source: [`src/call_fixup_installer.cppm`](src/call_fixup_installer.cppm).
- Native tests: [`tests/call_fixup_installer_tests.cppm`](tests/call_fixup_installer_tests.cppm).
- Fixture evidence: [`tests/data/test_call_fixup_installer.md`](tests/data/test_call_fixup_installer.md).

The port preserves analyzer identity, default enablement, priority `301`, target name fallback
(`name`, `_name`, `__name`), `libID_conflict_` removal, no-return handling, call-reference repair,
body rebuild, cancellation, and duplicate state behavior. It uses `SymbolRecord` only as an explicit
record of the selected fixup mapping.

The native `AnalysisContext::Function` currently has no call-fixup field and no compiler-spec
`PcodeInjectLibrary` accessor. The decompiler subsystem does expose structured injection providers,
but no public bridge supplies the active PE compiler-spec target map to an analyzer. Consequently the
module accepts rules from that missing adapter and never invents a compiler-spec mapping. Until that
bridge exists, the selected fixup is recorded as `kind=call_fixup`; actual function call-fixup
assignment and injected p-code remain pending rather than being falsely simulated.
