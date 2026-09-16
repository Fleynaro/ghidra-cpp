// MSVC x64 integration fixture for Ghidra's "Call Convention ID" analyzer.
//
// Original behavior and traceability:
// * Ghidra/Features/Decompiler/src/main/java/ghidra/app/plugin/core/analysis/DecompilerCallConventionAnalyzer.java
//   (canAnalyze(), findLocations(), and runDecompilerAnalysis()) only considers
//   p-code languages with multiple calling conventions, skips fixed/custom functions,
//   and decompiles unknown functions with defined parameter types.
// * The x64 Windows compiler specification has both `__fastcall` and `__thiscall`;
//   the loader normally gives a function a default convention, so run_ghidra.py
//   deliberately resets this exported function to Ghidra's unknown convention and
//   adds integer parameters. That setup mirrors the analyzer's explicit eligibility
//   predicates without relying on PDB files or unstable compiler debug metadata.
// * This source supplies real control flow and arithmetic for the decompiler. The
//   script extracts the function's convention and signature before and after analysis.

extern "C" volatile unsigned int convention_fixture_sink = 0U;

// Multiple integer parameters make the decompiler's recovered storage observable.
extern "C" __declspec(noinline) int convention_target(int first, int second, int third) {
    convention_fixture_sink += static_cast<unsigned int>(first);
    return first + second * 2 + third * 3;
}

// The caller prevents the target from being discarded and supplies a call graph edge.
extern "C" __declspec(noinline) void call_convention_id_entry() {
    convention_fixture_sink = static_cast<unsigned int>(convention_target(1, 2, 3));
}
