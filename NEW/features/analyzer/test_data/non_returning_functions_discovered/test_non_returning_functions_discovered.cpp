// MSVC x64 PE integration fixture for FindNoReturnFunctionsAnalyzer.java.
//
// Authoritative implementation:
// Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/analysis/FindNoReturnFunctionsAnalyzer.java,
// especially added(), detectNoReturn(), and
// checkNonReturningIndicators(). On x86-family processors an INT3 immediately
// after a call is evidence that the called function does not return; the default
// threshold is three indications.
//
// Artifact rationale and expected observable result:
// __debugbreak() is the MSVC intrinsic that emits a real INT3 instruction. Each
// exported caller invokes the same exported target and then executes that
// instruction, giving the analyzer three independent call references to count.
// The script disassembles bytes and seeds only exported function starts, then
// extracts the target's no-return flag, caller flow overrides, and bookmarks
// after analysis. It never marks or repairs these locations itself.

#include <intrin.h>

extern "C" volatile unsigned int discovered_sink = 0;

// The common target is the function whose evidence count must reach three.
extern "C" __declspec(dllexport) __declspec(noinline) void discovered_target() {
    discovered_sink = 0x10;
}

// Each caller retains a call with a fall-through INT3 indication. The code after
// the breakpoint is unreachable at runtime but remains in the PE for analysis.
extern "C" __declspec(dllexport) __declspec(noinline) void discovered_caller_one() {
    discovered_target();
    __debugbreak();
    discovered_sink += 1;
}

// Independent second evidence instance for the same target.
extern "C" __declspec(dllexport) __declspec(noinline) void discovered_caller_two() {
    discovered_target();
    __debugbreak();
    discovered_sink += 2;
}

// Independent third evidence instance reaches the default threshold exactly.
extern "C" __declspec(dllexport) __declspec(noinline) void discovered_caller_three() {
    discovered_target();
    __debugbreak();
    discovered_sink += 3;
}

// Keeps all callers and the target reachable without adding another evidence
// site that would make the threshold less explicit.
extern "C" __declspec(noinline) void fixture_entry() {
    discovered_caller_one();
    discovered_caller_two();
    discovered_caller_three();
}
