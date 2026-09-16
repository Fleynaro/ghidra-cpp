# Ghidra Port Evidence

The implementation follows `Ghidra/Features/DecompilerDependent/src/main/java/ghidra/app/plugin/core/string/variadic/FormatStringAnalyzer.java`
and its `FormatStringParser` helper. The original call-site application is
`HighFunctionDBUtil.writeOverride`; the native module is [`src/v.cppm`](src/v.cppm),
with grammar tests in [`tests/t.cppm`](tests/t.cppm).

## Status

- **Complete:** printf/scanf conversion parsing for integer, floating, character, string, pointer, count, flags, width, precision, length, suppression, and escaped-percent cases.
- **Complete:** Variadic target eligibility requires a variadic function with a final character pointer format parameter and a printf/scanf name.
- **Complete:** Format pointers are taken from mapped p-code constants and strings are read from the loaded image.
- **Partial:** The current `AnalysisContext` does not expose a call-site signature override table equivalent to `HighFunctionDBUtil.writeOverride`. The analyzer records an explicit `Function Signature Override` bookmark when enabled and never corrupts the persistent callee signature.
- **Pending:** A native call-site override API is required for exact decompiler propagation and output parity.
