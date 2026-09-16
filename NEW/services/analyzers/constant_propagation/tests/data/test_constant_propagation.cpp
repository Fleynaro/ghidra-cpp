// MSVC x64 behavioral fixture for the C++23 Constant Propagation port.
//
// Original behavior studied:
// * Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/analysis/ConstantPropagationAnalyzer.java
// * Ghidra/Features/Base/src/main/java/ghidra/program/util/SymbolicPropogator.java
//
// The source intentionally combines a multi-block loop, a switch, a global
// memory store, and a branch-dependent overwrite. The native unit tests still
// hardcode their expected facts; this executable and its report are only the
// human-readable parity oracle for the real program shape.

extern "C" volatile unsigned long long constant_sink = 0;
extern "C" volatile unsigned long long constant_table[4] = {0x11ULL, 0x22ULL, 0x44ULL, 0x88ULL};

// Case: propagate a seed through a loop and switch before storing it globally.
// Purpose: exercise several basic blocks and memory-visible arithmetic.
// Expected Ghidra behavior: the function remains a normal returning function;
// no unresolved branch target is invented from the switch.
// This catches: single-block propagation and accidental fallthrough guesses.
extern "C" __declspec(dllexport) __declspec(noinline) unsigned long long constant_branch_loop(unsigned int selector) {
    unsigned long long value = 0x100ULL + selector;
    for (unsigned int index = 0; index != 3U; ++index) {
        switch ((selector + index) & 3U) {
            case 0U:
                value += constant_table[0];
                break;
            case 1U:
                value ^= constant_table[1];
                break;
            case 2U:
                value -= constant_table[2];
                break;
            default:
                value += constant_table[3];
                break;
        }
    }
    constant_sink = value;
    return value;
}

// Case: force a value overwrite after a conditional memory read.
// Purpose: provide a negative control for stale constants and alias-like data
// flow when the condition is not known at analysis time.
// Expected Ghidra behavior: only stable facts survive the join.
// This catches: ports that retain one branch's value unconditionally.
extern "C" __declspec(dllexport) __declspec(noinline) unsigned long long constant_join(unsigned int selector) {
    unsigned long long value = constant_table[selector & 3U];
    if ((selector & 1U) != 0U) {
        value = 0x777ULL;
    } else {
        value = 0x999ULL;
    }
    constant_sink ^= value;
    return value;
}

// Keeps both complex functions live in the CRT-free image.
extern "C" __declspec(noinline) void fixture_entry() {
    constant_sink = constant_branch_loop(static_cast<unsigned int>(constant_sink));
    constant_sink ^= constant_join(static_cast<unsigned int>(constant_sink));
}
