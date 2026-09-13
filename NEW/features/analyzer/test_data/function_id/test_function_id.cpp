// MSVC x64 PE integration fixture for FidAnalyzer.java.
//
// Authoritative implementation:
// Ghidra/Features/FunctionID/src/main/java/ghidra/feature/fid/analyzer/FidAnalyzer.java,
// ApplyFidEntriesCommand.java, FidService.java, and FidServiceLibraryIngest.java.
// FidAnalyzer requires a language-compatible FID database, hashes existing
// functions through FidService, and applies labels/comments only after the real
// score and name-resolution rules pass.
//
// Artifact rationale and expected observable result:
// build.bat links this source twice so the library and target contain identical
// function bytes. run_ghidra.py populates a packed .fidb through
// FidFileManager.createNewFidDatabase(), FidFile.getFidDB(true), and
// FidService.createNewLibraryFromPrograms(); it never fabricates a hash or DB
// row. The target's exported known function is seeded as an existing function
// because FID operates on function bodies, then the script extracts the actual
// FID label, plate comment, and bookmark from the saved program.

extern "C" volatile unsigned long long function_id_sink = 0;

// This body is intentionally larger than FidService's four-code-unit minimum
// and contains stable arithmetic that produces a useful full FID hash.
extern "C" __declspec(dllexport) __declspec(noinline) unsigned long long
known_library_function(unsigned long long value) {
    value ^= 0x13579bdf2468ace0ULL;
    value += 0x1020304050607080ULL;
    value = (value << 7) | (value >> 57);
    value ^= 0x0f0e0d0c0b0a0908ULL;
    value += 0x8877665544332211ULL;
    value = (value << 11) | (value >> 53);
    value ^= 0xa5a5a5a55a5a5a5aULL;
    value += 0x0101010101010101ULL;
    value = (value << 3) | (value >> 61);
    value ^= 0xffff0000ffff0000ULL;
    value += 0x2222222222222222ULL;
    value ^= 0x3333333333333333ULL;
    value = (value << 13) | (value >> 51);
    value += 0x4444444444444444ULL;
    value ^= 0x5555555555555555ULL;
    value = (value << 5) | (value >> 59);
    value += 0x6666666666666666ULL;
    value ^= 0x7777777777777777ULL;
    value = (value << 17) | (value >> 47);
    value += 0x8888888888888888ULL;
    value ^= 0x9999999999999999ULL;
    return value;
}

// A second exported function makes the target a real small program rather than
// a single isolated record while preserving the exact known function bytes.
extern "C" __declspec(dllexport) __declspec(noinline) void fixture_entry() {
    function_id_sink = known_library_function(0x1122334455667788ULL);
}
