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
// * The fixed image-address immediates below target the first .data slot and the
//   entry point. /BASE:0x140000000 and PE section alignment make those positive
//   address-like scalars stable for this fixture rather than compiler-dependent.
// * scalar_operand_negative and scalar_operand_small are deliberate controls. The
//   executable is disassembled before analysis, and the report extracts actual
//   scalar values plus reference outcomes rather than asserting instruction text.

#pragma section(".data$scalar_operand", read, write)
__declspec(allocate(".data$scalar_operand")) extern "C" volatile unsigned long long scalar_operand_data =
    0xAABBCCDDEEFF0011ULL;
extern "C" volatile unsigned int scalar_operand_number = 0x12345678U;
extern "C" volatile unsigned int scalar_operand_small = 17U;

// Return the fixed address of the first aligned .data slot.
extern "C" __declspec(noinline) unsigned long long scalar_operand_positive_data() {
    return 0x0000000140003000ULL;
}

// Return the fixed address of the executable entry point.
extern "C" __declspec(noinline) unsigned long long scalar_operand_positive_code() {
    return 0x0000000140001000ULL;
}

// Return an address-shaped value outside the loaded image as a negative control.
extern "C" __declspec(noinline) unsigned long long scalar_operand_negative() {
    return 0x0000000012345678ULL;
}

// Return a value rejected by ScalarOperandAnalyzer's minimum-address filter.
extern "C" __declspec(noinline) unsigned long long scalar_operand_small_value() {
    return 17ULL;
}

// Case: an absolute scalar into a mapped non-code section, distinct from the
// first data slot and executable entry point.
// Purpose: exercise address-space probing for a valid image address that is
// neither a function entry nor the original positive data target.
// Expected Ghidra behavior: the scalar receives a memory reference when the
// target is mapped and no stronger operand reference already exists.
// This catches: implementations that only recognize one section or hard-code
// the first positive target.
extern "C" __declspec(noinline) unsigned long long scalar_operand_positive_rdata() {
    return 0x0000000140002000ULL;
}

// Keep all scalar cases observable in a single entry function.
extern "C" __declspec(noinline) void scalar_operand_references_entry() {
    scalar_operand_data = scalar_operand_positive_data();
    scalar_operand_data ^= scalar_operand_positive_code();
    scalar_operand_data ^= scalar_operand_negative();
    scalar_operand_data ^= scalar_operand_small_value();
    scalar_operand_data ^= scalar_operand_positive_rdata();
    scalar_operand_data ^= scalar_operand_number;
}
