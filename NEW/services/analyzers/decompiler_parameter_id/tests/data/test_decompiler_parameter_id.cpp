// MSVC x64 fixture for Ghidra's Decompiler Parameter ID analyzer.
//
// Exact Java implementation and dependency references:
// * Ghidra/Features/Decompiler/src/main/java/ghidra/app/plugin/core/analysis/DecompilerFunctionAnalyzer.java
//   (the exact analyzer name, PE/small-program default contract, priority, and added() call).
// * Ghidra/Features/Decompiler/src/main/java/ghidra/app/cmd/function/DecompilerParameterIdCmd.java
//   (dependency-graph construction, source-type reset, decompilation, and parameter commit).
// * Ghidra/Features/Decompiler/src/main/java/ghidra/app/decompiler/DecompileResults.java
//   (the decompiler result consumed by the command).
//
// Tested behavior:
// * parameter_fixture has three real Microsoft x64 ABI parameters, uses each parameter in
//   observable code, and returns a value. After analysis, Ghidra should expose a recovered
//   parameter list and stable storage/type facts for this function.
// * fixture_entry calls parameter_fixture through its normal direct-call path, retaining the
//   target and providing a small call graph for DecompilerParameterIdCmd.
//
// Why these constructs are chosen:
// * extern "C" makes the target name stable and isolates parameter recovery from Microsoft
//   name demangling; this fixture tests only parameter identification.
// * noinline, /Ob0, and observable volatile writes prevent inlining and preserve the call graph.
// * /Zi and /DEBUG:FULL retain the matching CodeView/PDB records so PDB Universal can supply
//   meaningful int, const char *, and int * types before Decompiler Parameter ID runs.
// * pointer and character parameters exercise integer, pointer, and output-storage recovery
//   without requiring CRT types, exception handling, or runtime initialization.
//
// Expected MSVC artifacts and Ghidra discoveries:
// * build.bat emits a small deterministic PE32+ executable whose default PE compiler model is
//   Microsoft x64. The image has no CRT imports and contains unwind metadata for the functions.
// * The PyGhidra preparation step disassembles and creates functions before analysis because
//   the Java analyzer operates on existing Function objects; those setup actions are not
//   reported as analyzer discoveries.
// * run_ghidra.py extracts parameter count, names, data types, and storage text before and after
//   the target analyzer and rejects undefined/unknown post-analysis placeholders.

extern "C" volatile unsigned int fixture_output = 0;

// Use all ABI parameters and publish the output value so parameter recovery has observable use sites.
extern "C" __declspec(dllexport) __declspec(noinline) int parameter_fixture(int first, const char* text, int* output) {
    int value = first;
    if (text != nullptr) {
        value += static_cast<unsigned char>(text[0]);
    }
    if (output != nullptr) {
        *output = value;
    }
    fixture_output = value;
    return value;
}

// Retain a direct call edge from the PE entry to the parameter-recovery target.
extern "C" __declspec(noinline) void fixture_entry() {
    int result = 0;
    result = parameter_fixture(7, "parameter-id", &result);
    fixture_output ^= static_cast<unsigned int>(result);
}
