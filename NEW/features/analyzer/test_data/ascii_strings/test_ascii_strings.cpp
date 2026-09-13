// MSVC x64 integration fixture for Ghidra's "ASCII Strings" analyzer.
//
// Original behavior and traceability:
// * Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/string/StringsAnalyzer.java
//   (StringsAnalyzer.added(), createStringIfValid(), and the registered string options)
//   scans initialized memory, scores candidate ASCII text with NGramUtils, rejects
//   instruction/data conflicts according to its options, and creates string data.
// * The readable strings below live in initialized read-only memory so the analyzer's
//   accessible-memory filter can discover them without depending on compiler symbols.
// * The short, unterminated, and non-ASCII cases are deliberate negative controls: the
//   default minimum length is five and the default requires a null terminator.
// * run_ghidra.py extracts defined string data and the exact analyzer options into
//   test_ascii_strings.md; the report is a behavioral extraction, not hand-written output.

extern "C" volatile const char* ascii_fixture_sink = nullptr;

// Global initialized strings ensure the candidate bytes remain in an accessible .rdata block.
extern "C" const char ascii_welcome[] = "The quick brown fox jumps over the lazy dog";
// This second sentence exercises a separate valid string range.
extern "C" const char ascii_protocol[] = "Ghidra ASCII string analysis discovers readable data";

// This function keeps every fixture string reachable while leaving the bytes in .rdata.
extern "C" __declspec(noinline) void ascii_strings_entry() {
    static const char kShort[] = "no";
    static const char kUnterminated[] = {'u', 'n', 't', 'e', 'r', 'm', 'i', 'n', 'a', 't', 'e', 'd'};
    static const unsigned char kNonAscii[] = {0xC3, 0xA9, 0x00};
    ascii_fixture_sink = ascii_welcome;
    ascii_fixture_sink = ascii_protocol;
    ascii_fixture_sink = kShort;
    ascii_fixture_sink = kUnterminated;
    ascii_fixture_sink = reinterpret_cast<const char*>(kNonAscii);
}
