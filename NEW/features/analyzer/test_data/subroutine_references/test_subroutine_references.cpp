// MSVC x64 integration fixture for Ghidra's "Subroutine References" analyzer.
//
// Behavioral contract covered by this fixture:
//
// * The original analyzer is an instruction analyzer. It scans existing direct
//   call references, collects each call target, ignores a target that is also
//   the call instruction's fall-through address, removes targets that already
//   have a function symbol, and asks AutoAnalysisManager to create the rest.
// * caller_one, caller_two, and caller_three are deliberately not exported or
//   named in the PE. They are reached by direct calls from fixture_entry, while
//   target_shared is reached by all three callers. target_alpha is reached by
//   one caller and target_beta by one caller. The target functions have no exception
//   metadata and are therefore unknown before analysis; the callers may be
//   initially represented by the PE's x64 unwind metadata, which is loader data
//   rather than an unrelated analyzer creating them.
// * fixture_entry directly calls caller_one and caller_two, which the MSVC x64
//   PE's unwind metadata makes initially known functions. FunctionAnalyzer.java
//   must therefore remove those targets from its creation set instead of
//   attempting to create duplicate functions. This is the explicit negative
//   case; it follows the analyzer's real existing-function filter.
// * The fixture is compiled without the CRT and without linker folding. The
//   Python harness disassembles all executable bytes before analysis, with code
//   analysis disabled, so no unrelated analyzer is responsible for discovering
//   or creating these functions.
//
// The source behavior is taken from:
// * Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/function/FunctionAnalyzer.java
//   (added(), fallthroughCall(), and the function-entry filtering pass).
// * Ghidra/Features/Base/src/main/java/ghidra/app/cmd/function/CreateFunctionCmd.java
//   (automatic body construction and the requirement that a code unit exists at
//   the requested entry before a function can be created).

// Observable global state keeps each caller's second call live and prevents
// MSVC from turning the final call into a tail jump.
extern "C" volatile unsigned int fixture_sink = 0U;

// A no-inline target with observable volatile code prevents the compiler from
// deleting the target or replacing its direct call with an inline expression.
extern "C" __declspec(noinline) void target_shared() {
    fixture_sink = 0x11U;
}

// This target is called only by caller_one and must still be discovered from a
// direct CALL reference rather than from a pre-existing function definition.
extern "C" __declspec(noinline) void target_alpha() {
    fixture_sink = 0x22U;
}

// This target is called only by caller_two and is intentionally different from
// target_alpha so the report contains independent call targets.
extern "C" __declspec(noinline) void target_beta() {
    fixture_sink = 0x33U;
}

// The first caller contributes one shared-target call and one unique-target
// call. The noinline attribute preserves both direct CALL instructions.
extern "C" __declspec(noinline) void caller_one() {
    target_shared();
    fixture_sink += 1U;
    target_alpha();
    fixture_sink += 1U;
}

// The second caller contributes another shared-target call and the beta call.
extern "C" __declspec(noinline) void caller_two() {
    target_shared();
    fixture_sink += 2U;
    target_beta();
    fixture_sink += 2U;
}

// A third caller creates the multiple-callers-to-one-target case without adding
// another unique target that would obscure the report.
extern "C" __declspec(noinline) void caller_three() {
    target_shared();
    fixture_sink += 3U;
}

// The linker entry provides direct calls to the callers, while the callers
// provide all target cases.
extern "C" __declspec(noinline) void fixture_entry() {
    caller_one();
    caller_two();
    caller_three();
}
