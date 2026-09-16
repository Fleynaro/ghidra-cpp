// MSVC x86 fixture for X86Analyzer.java's constant LEA reference behavior.
//
// Exact original source references:
// * Ghidra/Processors/x86/src/main/java/ghidra/app/plugin/core/analysis/X86Analyzer.java
//   canAnalyze() lines 40-44 restricts this analyzer to Processor "x86".
// * Its flowConstants() override lines 47-99 installs a context evaluator. The
//   evaluateContext() implementation lines 55-73 handles LEA, obtains the propagated
//   register value, constructs an address, and adds an analysis operand reference when
//   that address is present in memory.
// * The shared propagation setup and instruction traversal are in
//   Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/analysis/ConstantPropagationAnalyzer.java,
//   analyzeLocation() lines 457-491 and
//   flowConstants() lines 503-515.
//
// C++ inline assembly is intentional here. It prevents the compiler from replacing
// the pattern with a plain MOV and makes the LEA branch in the exact x86 analyzer
// observable in the imported PE. run_ghidra.py extracts the LEA operand references
// before and after analysis into the generated markdown report.

extern "C" volatile unsigned int target_value = 0x13579BDFU;

// The explicit LEA computes the absolute address of target_value into EAX before loading it.
extern "C" __declspec(noinline) unsigned int lea_constant_reference() {
    __asm {
        lea eax, target_value
        mov eax, [eax]
    }
}

// The entry function keeps the referenced code and data live without a CRT.
extern "C" __declspec(noreturn) void fixture_entry() {
    for (;;) {
        (void)lea_constant_reference();
    }
}
