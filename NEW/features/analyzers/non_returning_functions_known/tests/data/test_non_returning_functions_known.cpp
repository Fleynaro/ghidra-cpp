// MSVC x64 PE integration fixture for NoReturnFunctionAnalyzer.java.
//
// Authoritative Java sources:
// Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/analysis/NoReturnFunctionAnalyzer.java
// and Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/analysis/NonReturningFunctionNames.java,
// together
// with Ghidra/Features/Base/data/noReturnFunctionConstraints.xml and
// PEFunctionsThatDoNotReturn. The analyzer loads the PE-constrained name list,
// strips leading underscores, creates a function for a matching label when
// needed, sets Function.setNoReturn(true), and creates a bookmark by default.
//
// Artifact rationale and expected observable result:
// This CRT-free PE exports a local function named exactly "abort", which is a
// real name in PEFunctionsThatDoNotReturn. The script extracts the matching
// function's no-return flag and Non-Returning Function bookmark from Ghidra's
// saved listing. The function returns in fixture code because the analyzer's
// contract is name-based metadata, not execution; the harness never runs it.

extern "C" volatile unsigned int non_returning_known_sink = 0;

// The exact exported name is the authoritative PE no-return database match.
extern "C" __declspec(dllexport) __declspec(noinline) void abort() {
    non_returning_known_sink = 0xab;
}

// A differently named export is the negative control for exact-name matching.
extern "C" __declspec(dllexport) __declspec(noinline) void returning_control() {
    non_returning_known_sink = 0xcd;
}

// References both functions so they remain in the standalone executable.
extern "C" __declspec(noinline) void fixture_entry() {
    abort();
    returning_control();
}
