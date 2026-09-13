// MSVC x64 integration fixture for Ghidra's "ASCII Strings" analyzer.
//
// Original behavior and traceability:
// * Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/string/StringsAnalyzer.java
//   (StringsAnalyzer.added(), createStringIfValid(), and the registered string options)
//   scans initialized memory, scores candidate ASCII text with NGramUtils, rejects
//   instruction/data conflicts according to its options, and creates string data.
// * The readable strings below live in initialized read-only memory and are exported so the
//   report can restrict observations to fixture-owned ranges instead of PE metadata strings.
// * The short, unterminated, and non-ASCII cases are deliberate negative controls: the
//   default minimum length is five and the default requires a null terminator.
// * run_ghidra.py extracts defined string data and the exact analyzer options into
//   test_ascii_strings.md; the report is a behavioral extraction, not hand-written output.

extern "C" volatile const char* ascii_fixture_sink = nullptr;

// Global initialized strings ensure the candidate bytes remain in an accessible .rdata block.
extern "C" __declspec(dllexport) const char ascii_welcome[] = "The quick brown fox jumps over the lazy dog";
// This second sentence exercises a separate valid string range.
extern "C" __declspec(dllexport) const char ascii_protocol[] = "Ghidra ASCII string analysis discovers readable data";

// Exported negative controls let the report retain rejected context without including unrelated
// compiler, PE, or linker strings from the surrounding initialized memory.
extern "C" __declspec(dllexport) const char ascii_short[] = "no";
extern "C" __declspec(dllexport) const char ascii_unterminated[] = {'u', 'n', 't', 'e'};
extern "C" __declspec(dllexport) const unsigned char ascii_non_ascii[] = {0xC3, 0xA9, 0x00};

// This function keeps every fixture string reachable while leaving the bytes in .rdata.
extern "C" __declspec(noinline) void ascii_strings_entry() {
    ascii_fixture_sink = ascii_welcome;
    ascii_fixture_sink = ascii_protocol;
    ascii_fixture_sink = ascii_short;
    ascii_fixture_sink = ascii_unterminated;
    ascii_fixture_sink = reinterpret_cast<const char*>(ascii_non_ascii);
}
