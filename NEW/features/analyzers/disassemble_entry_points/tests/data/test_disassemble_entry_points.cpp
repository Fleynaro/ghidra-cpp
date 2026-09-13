// MSVC x64 fixture for Ghidra's Disassemble Entry Points analyzer.
//
// Exact Java implementation and dependency references:
// * Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/disassembler/EntryPointAnalyzer.java
//   (added(), execute-set filtering, code-symbol/external-entry collection, immediate and delayed
//   disassembly, and dummy-function handling).
// * Ghidra/Features/Base/src/main/java/ghidra/app/cmd/function/CreateFunctionCmd.java
//   (function/body creation used by EntryPointAnalyzer when entry-point code is valid).
// * Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/disassemble/Disassembler.java
//   (instruction creation performed by the analyzer's disassembly commands).
//
// Tested behavior:
// * entry_point_alpha and entry_point_beta are exported executable symbols in the PE. They are
//   deliberately separate entry points and are called from fixture_entry only to retain them.
//   EntryPointAnalyzer must disassemble code at those newly added code-symbol addresses while
//   respecting the executable section.
// * data_only_marker is a non-code exported data symbol and is a negative case: the analyzer
//   must not disassemble its bytes as an instruction entry point.
//
// Why these constructs are chosen:
// * dllexport supplies genuine PE code/data symbols, matching the Java analyzer's symbol-driven
//   input instead of inventing bookmarks or labels in the script.
// * noinline, /OPT:NOREF, and explicit volatile reads retain all entry points and keep their
//   x64 instruction sequences stable without CRT startup code.
//
// Expected MSVC artifacts and Ghidra discoveries:
// * build.bat emits a deterministic PE32+ executable with export-table code symbols, a read-only
//   data marker, executable .text, and no CRT imports.
// * The script intentionally does not pre-disassemble all executable bytes: instruction presence
//   before and after project.analyze(program) is the analyzer-specific observation.
// * run_ghidra.py extracts only selected symbol addresses, instruction presence, and function
//   presence. It does not claim that the analyzer discovers arbitrary unnamed code.

extern "C" volatile unsigned int fixture_sink = 0;

// Provide the first exported executable entry-point symbol.
extern "C" __declspec(dllexport) __declspec(noinline) void entry_point_alpha() {
    fixture_sink = 0xA1;
}

// Provide a second exported executable entry-point symbol.
extern "C" __declspec(dllexport) __declspec(noinline) void entry_point_beta() {
    fixture_sink = 0xB2;
}

// Keep C++ linkage while giving the PE export a real external symbol that the
// harness can resolve by its decorated or demangled name.
extern __declspec(dllexport) const unsigned int data_only_marker = 0xDADA1234U;

// Retain both exported code symbols and the data-only negative control.
extern "C" __declspec(noinline) void fixture_entry() {
    entry_point_alpha();
    entry_point_beta();
    fixture_sink ^= data_only_marker;
}
