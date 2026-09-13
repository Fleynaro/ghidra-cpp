// MSVC x64 fixture for WindowsResourceReferenceAnalyzer.java and WindowsResourceReference.java.
//
// Exact original source references:
// * Ghidra/Features/MicrosoftCodeAnalyzer/src/main/java/ghidra/app/plugin/prototype/MicrosoftCodeAnalyzerPlugin/WindowsResourceReferenceAnalyzer.java,
//   canAnalyze() lines
//   60-67 requires a PE and added() lines 69-73 runs WindowsResourceReference.java.
// * Ghidra/Features/Decompiler/ghidra_scripts/WindowsResourceReference.java, run() lines
//   208-214 associates LoadStringW argument 2 with Rsrc_StringTable.
// * The same script's analyzeFunction() lines 364-423 accepts a constant P-code argument,
//   and addResourceTableReferences() lines 508-537 creates an analysis DATA reference.
// * findResource() lines 547-610 walks loader-created Rsrc_StringTable symbols and their
//   defined data elements to resolve the numeric string ID.
//
// The real .rc file provides STRINGTABLE, DIALOGEX, and MENU resources. The code uses
// two constant string IDs so the analyzer must resolve resource table entries rather
// than merely recognizing that a PE has an .rsrc section. run_ghidra.py extracts the
// loader-created resource symbols and DATA references from the saved program.

#include "resource.h"

extern "C" __declspec(dllimport) int __stdcall LoadStringW(void* instance, unsigned int identifier, wchar_t* buffer,
                                                           int buffer_length);

extern "C" volatile unsigned int fixture_sink = 0U;

// Both calls use constant resource IDs that are compiled into the P-code call arguments.
extern "C" __declspec(noinline) void resource_lookup_calls() {
    wchar_t buffer[64] = {};
    fixture_sink = static_cast<unsigned int>(
        LoadStringW(nullptr, IDS_GREETING, buffer, static_cast<int>(sizeof(buffer) / sizeof(buffer[0]))));
    fixture_sink += static_cast<unsigned int>(
        LoadStringW(nullptr, IDS_SECOND, buffer, static_cast<int>(sizeof(buffer) / sizeof(buffer[0]))));
}

// A CRT-free entry point keeps the resource fixture focused on PE loading and analysis.
extern "C" void fixture_entry() {
    resource_lookup_calls();
}
