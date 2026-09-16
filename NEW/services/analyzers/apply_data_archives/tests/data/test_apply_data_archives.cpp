// MSVC x64 integration fixture for Ghidra's "Apply Data Archives" analyzer.
//
// Original behavior and traceability:
// * Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/analysis/ApplyDataArchiveAnalyzer.java
//   (added(), getDataTypeArchives(), getAutoDTMs(), and openBuiltinGDT()) selects a
//   built-in or detected DataTypeManager and applies matching function signatures.
// * `memcpy` is intentionally named like a standard C library function represented by
//   Ghidra's installed `generic_clib_64.gdt`; exporting it makes the symbol stable while
//   keeping the fixture independent of a CRT implementation.
// * The record object supplies ordinary data for the archive-applied signature to coexist
//   with real program data. run_ghidra.py records function signatures before and after.
// * The markdown is generated from the reopened program and never embeds a fabricated
//   archive result; an unavailable built-in archive is reported as an observed limitation.

extern "C" volatile unsigned int archive_fixture_sink = 0U;

// This record supplies ordinary initialized data for archive signature application.
struct archive_record {
    unsigned int count;
    const char* source;
};

// This CRT-free implementation has the exact exported name used by the generic archive.
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
extern "C" __declspec(noinline) void* memcpy(void* destination, const void* source, unsigned __int64 count) {
    auto* out = static_cast<unsigned char*>(destination);
    const auto* in = static_cast<const unsigned char*>(source);
    for (unsigned __int64 index = 0; index < count; ++index) {
        out[index] = in[index];
    }
    return destination;
}

// Calling the archive candidate keeps it in the executable and makes its signature useful.
extern "C" __declspec(noinline) void apply_data_archives_entry() {
    static const char text[] = "archive fixture";
    archive_record record{sizeof(text), text}; // NOLINT(modernize-use-designated-initializers)
    archive_fixture_sink = record.count;
    memcpy(&record, &record, sizeof(record));
}
