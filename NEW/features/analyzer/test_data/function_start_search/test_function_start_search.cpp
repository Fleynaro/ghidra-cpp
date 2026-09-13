// MSVC x64 PE integration fixture for FunctionStartAnalyzer.java and its phase analyzers.
//
// Authoritative Java sources:
// Ghidra/Features/BytePatterns/src/main/java/ghidra/app/analyzers/FunctionStartAnalyzer.java
// is the requested analyzer; its phase analyzers are
// Ghidra/Features/BytePatterns/src/main/java/ghidra/app/analyzers/FunctionStartPostAnalyzer.java,
// Ghidra/Features/BytePatterns/src/main/java/ghidra/app/analyzers/FunctionStartDataPostAnalyzer.java,
// and Ghidra/Features/BytePatterns/src/main/java/ghidra/app/analyzers/FunctionStartFuncAnalyzer.java.
// The main analyzer searches
// architecture-specific patterns, schedules disassembly/function creation, and
// optionally bookmarks matches; the phase analyzers only participate when their
// pattern prerequisites exist.
//
// Artifact rationale and expected observable result:
// /Od and /Oy- force ordinary x64 stack-frame prologues beginning with SUB RSP,
// imm, while /OPT:NOREF and /OPT:NOICF retain stable function boundaries. The
// script extracts candidate export offsets, all function entries before and
// after analysis, and Function Start Search bookmarks. In the verified
// standalone PyGhidra environment these real x64 prologues produced no
// scheduled pattern-created function, so the generated report records zero
// created entries/bookmarks instead of inventing a match; the limitation is
// documented in the fixture README.

extern "C" volatile unsigned int function_start_sink = 0;

// Stack locals make the compiler emit the x86-64 Windows stack-allocation pattern.
extern "C" __declspec(dllexport) __declspec(noinline) unsigned int function_start_candidate_a() {
    volatile unsigned int locals[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    return locals[0] ^ locals[3] ^ locals[7];
}

// A second candidate verifies that discovery is not a one-match artifact.
extern "C" __declspec(dllexport) __declspec(noinline) unsigned int function_start_candidate_b() {
    volatile unsigned int locals[8] = {3, 5, 7, 11, 13, 17, 19, 23};
    return locals[1] + locals[4] - locals[6];
}

// Keeps both candidates in the PE without making them pre-existing functions in
// the harness; the export symbols are only used to identify expected offsets.
extern "C" __declspec(noinline) void fixture_entry() {
    function_start_sink = function_start_candidate_a() + function_start_candidate_b();
}
