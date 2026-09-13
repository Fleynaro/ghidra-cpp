// MSVC x64 fixture for Ghidra's Variadic Function Signature Override analyzer.
//
// The exact Java behavior being exercised is in
// Ghidra/Features/DecompilerDependent/src/main/java/ghidra/app/plugin/core/string/variadic/FormatStringAnalyzer.java:
// run() collects defined strings containing '%' (lines 101-110), identifies imported
// variadic character-pointer functions (lines 122-132), and asks PcodeFunctionParser
// to inspect each call (lines 183-215). The parser contract is implemented in the
// adjacent PcodeFunctionParser.java, especially parseFunctionForCallData() lines 55-91.
// A successful analysis then creates one ParameterDefinition per parsed format
// argument and persists it with HighFunctionDBUtil.writeOverride() (lines 291-363).
//
// The PDB describes fixture_printf as taking only its fixed const-char format parameter. The
// actual call below passes an integer and a char pointer selected by %d and %s. This
// makes the fixture test the analyzer's format-string inference, rather than merely
// checking that a PDB signature was imported. run_ghidra.py extracts the before/after
// P-code call inputs from the analyzed program into the generated markdown report.

// This volatile sink keeps the argument-producing expressions observable in a CRT-free binary.
extern "C" volatile unsigned int fixture_sink = 0U;

// The string contains two supported output specifiers handled by FormatStringParser.java.
extern "C" const char fixture_format[] = "value=%d name=%s\n";
extern "C" const char fixture_name[] = "fixture";

// A real local variadic function supplies the PDB baseline without depending on a CRT import.
extern "C" __declspec(noinline) int fixture_printf(const char* format, ...) {
    fixture_sink = static_cast<unsigned int>(format[0]);
    return 0;
}

// Calls the variadic function with arguments that must be recovered from the format string.
extern "C" __declspec(noinline) int format_string_call() {
    int value = 37;
    fixture_sink = static_cast<unsigned int>(value);
    return fixture_printf(fixture_format, value, fixture_name);
}

// A linker entry point is used so that no CRT startup or termination code is required.
extern "C" __declspec(noinline) void fixture_entry() {
    fixture_sink = static_cast<unsigned int>(format_string_call());
}
