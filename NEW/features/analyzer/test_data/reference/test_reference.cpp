// MSVC x64 integration fixture for Ghidra's "Reference" analyzer.
//
// Exact Java implementation:
//   Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/analysis/OperandReferenceAnalyzer.java
//   Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/analysis/DataOperandReferenceAnalyzer.java
//
// Intended behavior and artifacts:
// * The original analyzer consumes already-created instruction/data units and
//   creates only references that pass memory, symbol, relocation, offcut, and
//   existing-reference checks. The Python harness performs disassembly with code
//   analysis disabled before enabling only Reference, so discovery is attributable
//   to this analyzer and its explicit disassembly prerequisite.
// * reference_text, reference_table, reference_value, and the two no-inline
//   functions provide pointer, string, and call relationships. The report extracts
//   actual reference-manager rows and does not fake their addresses or count.

extern "C" const char reference_text[] = "reference-fixture";
extern "C" volatile unsigned long long reference_value = 0x1122334455667788ULL;
extern "C" const unsigned long long reference_table[] = {0x1111111111111111ULL, 0x2222222222222222ULL,
                                                         0x3333333333333333ULL};

// Read the string and table so both data objects remain live in the PE.
extern "C" __declspec(noinline) unsigned long long reference_read() {
    reference_value += static_cast<unsigned long long>(reference_text[0]);
    return reference_value ^ reference_table[1];
}

// Retain an ordinary direct call that the generic reference analyzer can observe.
extern "C" __declspec(noinline) void reference_entry() {
    reference_value = reference_read();
}
