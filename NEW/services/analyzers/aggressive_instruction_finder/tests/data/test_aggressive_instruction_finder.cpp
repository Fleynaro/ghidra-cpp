// MSVC x64 integration fixture for Ghidra's "Aggressive Instruction Finder" analyzer.
//
// Original behavior and traceability:
// * Ghidra/Features/Base/src/main/java/ghidra/app/plugin/prototype/analysis/AggressiveInstructionFinderAnalyzer.java
//   (added(), function-start hashing, PseudoDisassembler validation, and bookmark creation)
//   requires at least 20 existing functions and instructions, hashes repeated starts, then
//   scans undefined executable bytes for a valid subroutine before disassembling them.
// * The twenty-four seed functions intentionally share a simple shape, creating repeated
//   two-instruction starts for the analyzer's frequency test. `aggressive_candidate` is a
//   real no-inline function that the harness disassembles first and then clears from the
//   listing, leaving its valid bytes as the undefined candidate the analyzer must rediscover.
// * The analyzer is disabled by default in Java; run_ghidra.py explicitly enables it and
//   leaves `Create Analysis Bookmarks` enabled so the authoritative discovery artifact is
//   observable without guessing at image addresses.
// * The generated markdown extracts function counts and the candidate's final analysis
//   bookmark state from the analyzed program.

extern "C" volatile unsigned int aggressive_sink = 0U;

// Repeated no-inline starts supply the minimum population and hash frequency.
// Every seed has the same first two instructions; /OPT:NOICF keeps their entries distinct.
// Seed 00 is the canonical repeated-start control.
extern "C" __declspec(noinline) void seed_00() {
    aggressive_sink += 0U;
    aggressive_sink += 0U;
}
// Each remaining seed repeats the same two-instruction prefix for the hash map.
// Seed 01 is an independent function with the same prefix.
extern "C" __declspec(noinline) void seed_01() {
    aggressive_sink += 0U;
    aggressive_sink += 0U;
}
// Seed 02 is an independent function with the same prefix.
extern "C" __declspec(noinline) void seed_02() {
    aggressive_sink += 0U;
    aggressive_sink += 0U;
}
// Seed 03 is an independent function with the same prefix.
extern "C" __declspec(noinline) void seed_03() {
    aggressive_sink += 0U;
    aggressive_sink += 0U;
}
// Seed 04 is an independent function with the same prefix.
extern "C" __declspec(noinline) void seed_04() {
    aggressive_sink += 0U;
    aggressive_sink += 0U;
}
// Seed 05 is an independent function with the same prefix.
extern "C" __declspec(noinline) void seed_05() {
    aggressive_sink += 0U;
    aggressive_sink += 0U;
}
// Seed 06 is an independent function with the same prefix.
extern "C" __declspec(noinline) void seed_06() {
    aggressive_sink += 0U;
    aggressive_sink += 0U;
}
// Seed 07 is an independent function with the same prefix.
extern "C" __declspec(noinline) void seed_07() {
    aggressive_sink += 0U;
    aggressive_sink += 0U;
}
// Seed 08 is an independent function with the same prefix.
extern "C" __declspec(noinline) void seed_08() {
    aggressive_sink += 0U;
    aggressive_sink += 0U;
}
// Seed 09 is an independent function with the same prefix.
extern "C" __declspec(noinline) void seed_09() {
    aggressive_sink += 0U;
    aggressive_sink += 0U;
}
// Seed 10 is an independent function with the same prefix.
extern "C" __declspec(noinline) void seed_10() {
    aggressive_sink += 0U;
    aggressive_sink += 0U;
}
// Seed 11 is an independent function with the same prefix.
extern "C" __declspec(noinline) void seed_11() {
    aggressive_sink += 0U;
    aggressive_sink += 0U;
}
// Seed 12 is an independent function with the same prefix.
extern "C" __declspec(noinline) void seed_12() {
    aggressive_sink += 0U;
    aggressive_sink += 0U;
}
// Seed 13 is an independent function with the same prefix.
extern "C" __declspec(noinline) void seed_13() {
    aggressive_sink += 0U;
    aggressive_sink += 0U;
}
// Seed 14 is an independent function with the same prefix.
extern "C" __declspec(noinline) void seed_14() {
    aggressive_sink += 0U;
    aggressive_sink += 0U;
}
// Seed 15 is an independent function with the same prefix.
extern "C" __declspec(noinline) void seed_15() {
    aggressive_sink += 0U;
    aggressive_sink += 0U;
}
// Seed 16 is an independent function with the same prefix.
extern "C" __declspec(noinline) void seed_16() {
    aggressive_sink += 0U;
    aggressive_sink += 0U;
}
// Seed 17 is an independent function with the same prefix.
extern "C" __declspec(noinline) void seed_17() {
    aggressive_sink += 0U;
    aggressive_sink += 0U;
}
// Seed 18 is an independent function with the same prefix.
extern "C" __declspec(noinline) void seed_18() {
    aggressive_sink += 0U;
    aggressive_sink += 0U;
}
// Seed 19 is an independent function with the same prefix.
extern "C" __declspec(noinline) void seed_19() {
    aggressive_sink += 0U;
    aggressive_sink += 0U;
}
// Seed 20 is an independent function with the same prefix.
extern "C" __declspec(noinline) void seed_20() {
    aggressive_sink += 0U;
    aggressive_sink += 0U;
}
// Seed 21 is an independent function with the same prefix.
extern "C" __declspec(noinline) void seed_21() {
    aggressive_sink += 0U;
    aggressive_sink += 0U;
}
// Seed 22 is an independent function with the same prefix.
extern "C" __declspec(noinline) void seed_22() {
    aggressive_sink += 0U;
    aggressive_sink += 0U;
}
// Seed 23 is an independent function with the same prefix.
extern "C" __declspec(noinline) void seed_23() {
    aggressive_sink += 0U;
    aggressive_sink += 0U;
}

// The candidate has a call and a terminal return, which satisfies the real validator.
extern "C" __declspec(noinline) void aggressive_candidate() {
    aggressive_sink += 0U;
    aggressive_sink += 0U;
    seed_00();
}

// Calling all seeds prevents /OPT:REF from removing the repeated starts.
extern "C" __declspec(noinline) void aggressive_instruction_finder_entry() {
    seed_00();
    seed_01();
    seed_02();
    seed_03();
    seed_04();
    seed_05();
    seed_06();
    seed_07();
    seed_08();
    seed_09();
    seed_10();
    seed_11();
    seed_12();
    seed_13();
    seed_14();
    seed_15();
    seed_16();
    seed_17();
    seed_18();
    seed_19();
    seed_20();
    seed_21();
    seed_22();
    aggressive_candidate();
}
