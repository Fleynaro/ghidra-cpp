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

// Case: a code entry with a loop, nested branches, and a switch.
// Purpose: force entry-point disassembly to follow several real x64 flow edges
// instead of succeeding on a single straight-line store.
// Expected Ghidra behavior: the entry instruction is disassembled, but no
// function is created by Disassemble Entry Points itself.
// This catches: implementations that only handle the first exported code
// symbol or stop when conditional flow is present.
extern "C" __declspec(dllexport) __declspec(noinline) void entry_point_gamma(unsigned int selector) {
    unsigned int value = selector;
    for (unsigned int index = 0; index != 3U; ++index) {
        switch ((value + index) & 3U) {
            case 0U:
                value ^= 0x11U;
                break;
            case 1U:
                value += 0x23U;
                break;
            case 2U:
                value -= 0x07U;
                break;
            default:
                value = value * 3U + 1U;
                break;
        }
    }
    fixture_sink ^= value;
}

// Case: a code entry containing a direct call and a post-call conditional.
// Purpose: verify that disassembly records the entry and its fall-through
// without accidentally creating a function or scanning unrelated code.
// Expected Ghidra behavior: the exported entry is instructional and the data
// export below remains undefined.
// This catches: implementations that conflate entry disassembly with function
// discovery or treat call targets as additional entry seeds.
extern "C" __declspec(dllexport) __declspec(noinline) void entry_point_delta(unsigned int selector) {
    if ((selector & 1U) != 0U) {
        entry_point_alpha();
    } else {
        entry_point_beta();
    }
    fixture_sink += selector;
}

// Keep C++ linkage while giving the PE export a real external symbol that the
// harness can resolve by its decorated or demangled name.
extern __declspec(dllexport) const unsigned int data_only_marker = 0xDADA1234U;

// Retain both exported code symbols and the data-only negative control.
extern "C" __declspec(noinline) void fixture_entry() {
    entry_point_alpha();
    entry_point_beta();
    entry_point_gamma(fixture_sink);
    entry_point_delta(fixture_sink);
    fixture_sink ^= data_only_marker;
}
