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

// Case: multiple memory reads selected by a branch and a variable offset.
// Purpose: make Reference inspect several operand positions and targets in one
// function instead of only one read and one write.
// Expected Ghidra behavior: each concrete memory access has its own exact
// source, target, operand, and access direction; no reference is invented for
// a computed address itself.
// This catches: ports that collapse references by target or only process the
// first memory operation in a basic block.
extern "C" __declspec(noinline) unsigned long long reference_mixed_reads(unsigned int selector) {
    const unsigned long long selected = (selector & 1U) != 0U ? reference_table[0] : reference_table[2];
    return selected ^ static_cast<unsigned long long>(selector);
}

// Retain an ordinary direct call that the generic reference analyzer can observe.
extern "C" __declspec(noinline) void reference_entry() {
    reference_value = reference_read();
    reference_value ^= reference_mixed_reads(static_cast<unsigned int>(reference_value));
}
