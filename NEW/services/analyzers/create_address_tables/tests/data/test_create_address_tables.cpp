// MSVC x64 fixture for Ghidra's Create Address Tables analyzer.
//
// Exact Java implementation and dependency references:
// * Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/disassembler/AddressTableAnalyzer.java
//   (constructor, canAnalyze(), added(), processAddressTable(), and the bookmark contract).
// * Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/disassembler/AddressTable.java
//   (pointer-run recognition and table element validation used by AddressTableAnalyzer).
// * Ghidra/Features/Base/src/test/java/ghidra/app/plugin/core/disassembler/AddressTableAnalyzerTest.java
//   (undefined pointer storage, minimum table size, and Address Table bookmark assertions).
//
// Tested behavior:
// * fixture_table is an aligned run of valid 64-bit addresses in otherwise undefined
//   read-only data. The analyzer must recognize the run, create pointer data, and add an
//   Analysis bookmark named "Address Table" at its first entry.
// * fixture_after_table is a non-pointer sentinel. It prevents the run from being
//   interpreted as an unbounded table and makes the expected table boundary observable.
// * fixture_entry reads one table element so MSVC retains the table and emits relocation
//   records in the PE. The analyzer uses those loader facts only as a relocation guide;
//   it does not depend on another analysis analyzer.
//
// Why these constructs are chosen:
// * noinline functions provide real executable targets without relying on CRT startup;
//   the explicit export keeps their PE symbols and makes the generated addresses easy to
//   identify in the markdown extraction.
// * volatile storage prevents constant folding and dead-store elimination while /OPT:NOICF
//   and /OPT:NOREF preserve distinct functions and the table in the MSVC image.
// * the table is const and placed in .rdata so the imported bytes begin as undefined data,
//   matching the original analyzer's intended input rather than pre-defined C++ objects.
//
// Expected MSVC artifacts and Ghidra discoveries:
// * build.bat emits a deterministic PE32+ executable with .text and .rdata sections,
//   x64 relocations, and no CRT imports.
// * PyGhidra should discover one Address Table bookmark and pointer data at the contiguous
//   table entries. It must not claim the sentinel as another table entry.
// * run_ghidra.py writes only those analyzer-specific facts plus the enabled option list.

extern "C" volatile unsigned long long fixture_sink = 0;

#pragma section(".rdata$fixture", read)

// Provide the first valid executable address stored in the table.
extern "C" __declspec(dllexport) __declspec(noinline) void table_target_one() {
    fixture_sink = 0x11111111ULL;
}

// Provide the second valid executable address stored in the table.
extern "C" __declspec(dllexport) __declspec(noinline) void table_target_two() {
    fixture_sink = 0x22222222ULL;
}

// Provide the third valid executable address stored in the table.
extern "C" __declspec(dllexport) __declspec(noinline) void table_target_three() {
    fixture_sink = 0x33333333ULL;
}

// Provide the fourth valid executable address stored in the table.
extern "C" __declspec(dllexport) __declspec(noinline) void table_target_four() {
    fixture_sink = 0x44444444ULL;
}

using table_target = void (*)();

// NOLINTNEXTLINE(modernize-avoid-c-arrays): MSVC section allocation requires a native array.
extern "C" __declspec(dllexport) __declspec(allocate(".rdata$fixture")) const table_target fixture_table[] = {
    table_target_one,
    table_target_two,
    table_target_three,
    table_target_four,
};

extern "C" __declspec(dllexport) __declspec(allocate(".rdata$fixture")) const unsigned long long fixture_after_table =
    0x0102030405060708ULL;

// Retain the table and sentinel through a normal executable call path.
extern "C" __declspec(noinline) void fixture_entry() {
    table_target target = fixture_table[fixture_sink & 3ULL];
    target();
    fixture_sink ^= fixture_after_table;
}
