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
// /Od and /Oy- retain the ordinary x64 stack-frame prologues used by the
// negative candidate. The executable section also contains a deliberately
// exported raw byte sequence whose three 0xCC prefix bytes and SUB RSP bytes match the
// x86-64 Windows FunctionStart pattern. The harness removes only the function
// at that positive pattern mark, retains the negative candidate as a
// pre-existing function, and compares function/bookmark state before and after
// the real analyzer. No expected address is inferred from compiler ordering.

extern "C" volatile unsigned int function_start_sink = 0;

#pragma section(".text$funcstart_fixture", read, execute)

// This raw executable sequence is the positive discoverable pattern. The
// FunctionStart pattern marks the SUB instruction three bytes after the INT3 pad.
extern "C" __declspec(dllexport) __declspec(allocate(".text$funcstart_fixture"))
const unsigned char function_start_positive_pattern[] = {0xCC, 0xCC, 0xCC, 0x48, 0x83, 0xEC, 0x28, 0xB8, 0x2A,
                                                         0x00, 0x00, 0x00, 0x48, 0x83, 0xC4, 0x28, 0xC3};

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
