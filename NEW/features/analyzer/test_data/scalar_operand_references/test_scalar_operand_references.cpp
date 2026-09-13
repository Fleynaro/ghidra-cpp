// MSVC x64 integration fixture for Ghidra's "Scalar Operand References" analyzer.
//
// Exact Java implementation:
//   Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/analysis/ScalarOperandAnalyzer.java
//
// Intended behavior and artifacts:
// * ScalarOperandAnalyzer.java inspects Scalar operand objects, first honoring a
//   matching relocation, then rejecting common numeric sentinels and values below
//   4096, and finally probing address spaces before adding analysis references.
//   It also rejects offcut function targets and preserves existing references.
// * scalar_operand_data and scalar_operand_function are address-bearing objects;
//   scalar_operand_number and scalar_operand_small are deliberate numeric controls.
//   The executable is disassembled before analysis, and the report extracts actual
//   scalar operand text and references rather than asserting a guessed encoding.

extern "C" volatile unsigned long long scalar_operand_data = 0xAABBCCDDEEFF0011ULL;
extern "C" volatile unsigned int scalar_operand_number = 0x12345678U;
extern "C" volatile unsigned int scalar_operand_small = 17U;

// Provide a code address and data address that the compiler must materialize.
extern "C" __declspec(noinline) unsigned long long scalar_operand_function() {
    return scalar_operand_data ^ scalar_operand_number;
}

// Keep all scalar cases observable in a single entry function.
extern "C" __declspec(noinline) void scalar_operand_references_entry() {
    scalar_operand_data = scalar_operand_function() + scalar_operand_small;
}
