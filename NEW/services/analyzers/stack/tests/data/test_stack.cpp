// MSVC x64 integration fixture for Ghidra's "Stack" analyzer.
//
// Exact Java implementation:
//   Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/function/StackVariableAnalyzer.java
//   Ghidra/Features/Base/src/main/java/ghidra/app/cmd/function/NewFunctionStackAnalysisCmd.java
//
// Intended behavior and artifacts:
// * StackVariableAnalyzer finds defined function entries in the analysis set and
//   invokes NewFunctionStackAnalysisCmd to infer stack variables and references.
//   The harness explicitly disassembles first, enables the Subroutine References
//   prerequisite and Stack with its local-variable option, and extracts
//   actual stack references/local variables from the resulting listing.
// * stack_worker has volatile locals and takes one local's address, while
//   stack_entry creates a nested call boundary. /Oy- retains frame-pointer-friendly
//   prologues. These are inputs to discovery, not fake expected rows.

extern "C" volatile unsigned int stack_sink = 0U;

// Use volatile locals and an address-taking operation to make stack storage real.
extern "C" __declspec(noinline) unsigned int stack_worker(unsigned int seed) {
    volatile unsigned int local_value = seed + 3U;
    volatile unsigned int local_copy = local_value ^ 0x55U;
    volatile auto local_byte = static_cast<unsigned char>(seed);
    volatile unsigned long long local_wide = static_cast<unsigned long long>(local_value) << 32U;
    volatile unsigned int* local_pointer = &local_copy;
    stack_sink += *local_pointer;
    return local_value + local_copy + local_byte + static_cast<unsigned int>(local_wide >> 32U);
}

// Keep a separate caller frame and a live local across the call.
extern "C" __declspec(noinline) void stack_entry() {
    volatile unsigned int caller_local = 0x24U;
    stack_sink = stack_worker(caller_local);
}
