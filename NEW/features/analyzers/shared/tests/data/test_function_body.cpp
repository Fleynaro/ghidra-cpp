// Dedicated x64 PE fixture for shared AnalysisContext function-body tests.
//
// Original behavior studied:
// * Ghidra/Features/Base/src/main/java/ghidra/app/cmd/function/CreateFunctionCmd.java
// * Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/model/block/FollowFlow.java
// * Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/model/block/BasicBlockModel.java

extern "C" volatile unsigned long long function_body_sink = 0;

// Case: a straight-line exported function.
// Purpose: retain an ordinary executable entry in the shared fixture.
// Expected Ghidra behavior: the entry has one contiguous body.
// This catches: broken PE/Sleigh setup in infrastructure tests.
extern "C" __declspec(dllexport) __declspec(noinline) unsigned long long
function_body_straight(unsigned long long value) {
    function_body_sink ^= value + 0x11ULL;
    return function_body_sink;
}

// Case: nested branches, a loop, and a switch in one exported function.
// Purpose: provide realistic code-flow shapes for manual Ghidra inspection.
// Expected Ghidra behavior: branch destinations and loop headers are distinct
// flow leaders, while calls remain in the caller's basic block semantics.
// This catches: accidentally replacing the shared fixture with a trivial blob.
extern "C" __declspec(dllexport) __declspec(noinline) unsigned long long
function_body_branch_loop(unsigned int selector) {
    unsigned long long value = selector;
    for (unsigned int index = 0; index != 3U; ++index) {
        switch ((selector + index) & 3U) {
            case 0U:
                value += 3ULL;
                break;
            case 1U:
                value ^= 5ULL;
                break;
            case 2U:
                value -= 7ULL;
                break;
            default:
                value *= 2ULL;
                break;
        }
    }
    function_body_sink = value;
    return value;
}

// Case: a data read and a call from the fixture entry point.
// Purpose: ensure the PE contains both code and data without making data a
// control-flow edge in the shared body tests.
// Expected Ghidra behavior: the data object is not a function entry.
// This catches: fixture setup that only exercises executable bytes.
extern "C" __declspec(dllexport) __declspec(noinline) unsigned long long function_body_data_read() {
    return function_body_sink;
}

// Keeps all exported cases reachable in the deterministic CRT-free image.
extern "C" __declspec(noinline) void fixture_entry() {
    function_body_sink = function_body_straight(function_body_sink);
    function_body_sink ^= function_body_branch_loop(static_cast<unsigned int>(function_body_sink));
    function_body_sink ^= function_body_data_read();
}
