// MSVC x64 fixture for Ghidra's Data Reference analyzer.
//
// Exact Java implementation and dependency references:
// * Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/analysis/DataOperandReferenceAnalyzer.java
//   (the exact name "Data Reference", priority after reference analysis, and the deliberate
//   createFunctions() override that forbids functions from data pointers).
// * Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/analysis/OperandReferenceAnalyzer.java
//   (added(), memory-reference filtering, ASCII/Unicode/pointer recognition, and data-origin
//   reference processing).
// * Ghidra/Features/Base/src/test.slow/java/help/screenshot/AutoAnalysisPluginScreenShots.java
//   (the user-visible Data Reference analysis option name).
//
// Tested behavior:
// * fixture_pointer_data contains relocatable pointers to two strings. The PE loader supplies
//   data-origin memory references; DataOperandReferenceAnalyzer must follow them and define the
//   referenced strings/pointers without creating functions from the pointer values.
// * fixture_unreferenced_text is deliberately not referenced by data. It is a negative case:
//   this analyzer must not report or define it merely because it is a valid ASCII sequence.
//
// Why these constructs are chosen:
// * const pointer data in .rdata is emitted as a real relocation-bearing pointer array, which
//   gives Ghidra the data-to-data references consumed by OperandReferenceAnalyzer.added().
// * volatile reads keep both array elements and both strings in the final image while leaving
//   the source independent of the Windows CRT.
// * exported functions are only retention and identification aids; the fixture does not claim
//   that the analyzer itself creates functions from data pointers.
//
// Expected MSVC artifacts and Ghidra discoveries:
// * build.bat emits a deterministic PE32+ image with .rdata relocations and no library imports.
// * Ghidra should show references whose source code units are data, pointer/string data at the
//   two referenced targets, and no function created at either string address.
// * run_ghidra.py extracts only data-origin references and defined data types relevant to this
//   analyzer. It does not infer results from source names.

extern "C" volatile unsigned long long fixture_sink = 0;

#pragma section(".rdata$fixture", read)

__declspec(allocate(".rdata$fixture")) extern "C" __declspec(dllexport) const char fixture_text_one[] =
    "data-reference-alpha";

__declspec(allocate(".rdata$fixture")) extern "C" __declspec(dllexport) const char fixture_text_two[] =
    "data-reference-beta";

__declspec(allocate(".rdata$fixture")) extern "C" __declspec(dllexport) const char fixture_unreferenced_text[] =
    "unreferenced-negative";

__declspec(allocate(".rdata$fixture")) extern "C" __declspec(dllexport) const char* const fixture_pointer_data[] = {
    fixture_text_one,
    fixture_text_two,
};

// Keep the exported code path alive while the analyzer examines only data-origin references.
extern "C" __declspec(dllexport) __declspec(noinline) void data_reference_target() {
    fixture_sink = static_cast<unsigned long long>(fixture_pointer_data[0][0]) +
                   static_cast<unsigned long long>(fixture_pointer_data[1][0]);
}

// Retain the pointer array and provide a normal executable entry for the PE loader.
extern "C" __declspec(noinline) void fixture_entry() {
    data_reference_target();
    fixture_sink ^= reinterpret_cast<unsigned long long>(fixture_pointer_data);
}
