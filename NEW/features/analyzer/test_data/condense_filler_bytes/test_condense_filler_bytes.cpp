// MSVC x64 integration fixture for Ghidra's "Condense Filler Bytes" analyzer.
//
// Original behavior and traceability:
// * Ghidra/Features/Base/src/main/java/ghidra/app/analyzers/CondenseFillerBytesAnalyzer.java
//   (determineFillerValue(), added(), countUndefineds(), and replaceFillerBytes())
//   samples undefined bytes immediately after functions, selects the most frequent
//   byte pattern in Auto mode, and replaces matching runs with AlignmentDataType.
// * The separately ordered functions encourage linker padding between function bodies;
//   the report measures the resulting undefined runs before and Alignment data after.
// * The source does not assume a specific linker fill byte. Auto mode intentionally
//   follows the Java implementation's frequency rule rather than hard-coding 0xCC.
// * run_ghidra.py extracts alignment data and adjacent bytes into the markdown report.

extern "C" volatile unsigned int condense_sink = 0U;

// These no-inline functions provide multiple function-to-filler boundaries for sampling.
#pragma code_seg(push, filler_fixture_code, ".text$F")
// Function A contributes one linker boundary and one filler sample.
extern "C" __declspec(noinline) void filler_function_a() {
    condense_sink += 1U;
}
// Function B contributes a second linker boundary and sample.
extern "C" __declspec(noinline) void filler_function_b() {
    condense_sink += 2U;
}
// Function C contributes a third linker boundary and sample.
extern "C" __declspec(noinline) void filler_function_c() {
    condense_sink += 3U;
}
// Function D contributes a fourth linker boundary and sample.
extern "C" __declspec(noinline) void filler_function_d() {
    condense_sink += 4U;
}
#pragma code_seg(pop, filler_fixture_code)

// The entry calls every function so linker reference elimination cannot remove boundaries.
extern "C" __declspec(noinline) void condense_filler_bytes_entry() {
    filler_function_a();
    filler_function_b();
    filler_function_c();
    filler_function_d();
}
