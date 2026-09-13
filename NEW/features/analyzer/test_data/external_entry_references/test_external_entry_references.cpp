// MSVC x64 PE integration fixture for ExternalEntryFunctionAnalyzer.java.
//
// Authoritative implementation:
// Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/function/ExternalEntryFunctionAnalyzer.java
// (added() and isGoodFunctionStart()). The analyzer iterates the program's
// external-entry addresses, requires an instruction at each address, rejects a
// location reached by the preceding instruction's fall-through, removes entries
// already represented by function symbols, and asks AutoAnalysisManager to
// create the remaining functions.
//
// Artifact rationale and expected observable result:
// A PE export table is the real standalone-EXE source of external-entry
// references. The three exported functions below are retained code locations;
// the unexported function is a negative control. After the script's explicit
// disassembly, exported locations that pass the authoritative checks should be
// function entries after analysis. The script extracts export/external-entry
// addresses, function entries before/after, and bookmarks are not used because
// this analyzer has no bookmark option.

extern "C" volatile unsigned int external_entry_sink = 0;

// Exported code provides a valid external entry reference and a real instruction.
extern "C" __declspec(dllexport) __declspec(noinline) void external_entry_alpha() {
    external_entry_sink = 0x11;
}

// A second exported code location verifies that creation is performed for more
// than one external entry and is not limited to the first symbol.
extern "C" __declspec(dllexport) __declspec(noinline) void external_entry_beta() {
    external_entry_sink = 0x22;
}

// This export is deliberately distinct from the positive controls so the report
// can show the complete PE export-derived entry set.
extern "C" __declspec(dllexport) __declspec(noinline) void external_entry_gamma() {
    external_entry_sink = 0x33;
}

// The unexported control is retained by the entry point but cannot become an
// external entry reference merely from being present in the PE.
extern "C" __declspec(noinline) void internal_control() {
    external_entry_sink += 1;
}

// Keeps all positive and negative controls reachable in the executable.
extern "C" __declspec(noinline) void fixture_entry() {
    external_entry_alpha();
    external_entry_beta();
    external_entry_gamma();
    internal_control();
}
