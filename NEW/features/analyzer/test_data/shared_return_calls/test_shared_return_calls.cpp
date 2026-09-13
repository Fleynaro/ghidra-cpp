// MSVC x64 integration fixture for Ghidra's "Shared Return Calls" analyzer.
//
// Exact Java implementation:
//   Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/function/SharedReturnAnalyzer.java
//   Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/function/SharedReturnJumpAnalyzer.java
//   Ghidra/Features/Base/src/main/java/ghidra/app/cmd/analysis/SharedReturnAnalysisCmd.java
//
// Intended behavior and artifacts:
// * With optimization enabled, shared_return_wrapper is a tail-call-shaped C++
//   function and is expected to compile as an unconditional jump to
//   shared_return_target. The target is seeded as an existing function by the
//   harness because SharedReturnAnalysisCmd intentionally processes jumps to
//   already-defined destination functions.
// * SharedReturnJumpAnalyzer discovers jump references, then the superclass
//   checks the function boundary and applies FlowOverride.CALL_RETURN. The report
//   compares actual jump flow and flow-override state before and after analysis;
//   it does not invent a target address if this compiler version chooses a call.

extern "C" volatile unsigned int shared_return_sink = 0U;

// This no-inline function is the destination whose return is shared.
extern "C" __declspec(noinline) int shared_return_target(int value) {
    shared_return_sink += static_cast<unsigned int>(value);
    return value + 9;
}

// The optimizer may legally represent this forwarding return as a tail jump.
extern "C" __declspec(noinline) int shared_return_wrapper(int value) {
    return shared_return_target(value);
}

// Keep the wrapper reachable from the PE entry point and prevent dead stripping.
extern "C" __declspec(noinline) void shared_return_calls_entry() {
    shared_return_sink = static_cast<unsigned int>(shared_return_wrapper(4));
}
