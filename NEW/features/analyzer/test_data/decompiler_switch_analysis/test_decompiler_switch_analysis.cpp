// MSVC x64 fixture for Ghidra's Decompiler Switch Analysis analyzer.
//
// Exact Java implementation and dependency references:
// * Ghidra/Features/Decompiler/src/main/java/ghidra/app/plugin/core/analysis/DecompilerSwitchAnalyzer.java
//   (computed-jump location discovery, function selection, parallel decompilation, and the
//   one-time-analysis contract).
// * Ghidra/Features/Decompiler/src/main/java/ghidra/app/cmd/function/DecompilerSwitchAnalysisCmd.java
//   (JumpTable processing, switchD labeling, case references, case disassembly, and function
//   body repair).
// * Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/model/pcode/JumpTable.java
//   (switch address, cases, and label values extracted by the command).
//
// Tested behavior:
// * switch_fixture has eight dense cases and a default case. MSVC's optimized x64 lowering is
//   expected to produce a computed dispatch for this dense range; the analyzer must recover
//   the dynamic jump, add switch case references, label the dispatch with switchD, and include
//   the case instructions in the owning function body.
// * switch_fixture_negative uses sparse values and is retained as an ordinary branch case; it
//   prevents the report from treating every conditional chain as a recovered switch.
//
// Why these constructs are chosen:
// * dense integral cases are the compiler construct that reliably encourages a jump table,
//   while the separate sparse function documents the boundary without relying on hand-written
//   assembly or architecture-specific undefined behavior.
// * noinline and a volatile selector preserve the dynamic dispatch and ensure the switch is
//   reachable from fixture_entry.
//
// Expected MSVC artifacts and Ghidra discoveries:
// * build.bat uses /O2 with /Ob0, /GL-, /OPT:NOICF, and /OPT:NOREF so the dispatch and case
//   blocks remain deterministic and distinct in a CRT-free PE32+ image.
// * Ghidra should discover a switchD label/namespace and computed case references only for the
//   dense function. The report deliberately records labels and references rather than unstable
//   decompiler pseudocode.

extern "C" volatile int fixture_selector = 3;
extern "C" volatile int fixture_sink = 0;

// Dense cases encourage MSVC to lower this switch as a computed jump table.
extern "C" __declspec(dllexport) __declspec(noinline) int switch_fixture(int value) {
    switch (value) {
        case 0:
            return 10;
        case 1:
            return 20;
        case 2:
            return 30;
        case 3:
            return 40;
        case 4:
            return 50;
        case 5:
            return 60;
        case 6:
            return 70;
        case 7:
            return 80;
        default:
            return -1;
    }
}

// Sparse cases provide a conditional-branch negative control for switch recovery.
extern "C" __declspec(dllexport) __declspec(noinline) int switch_fixture_negative(int value) {
    switch (value) {
        case -100:
            return 1;
        case 7:
            return 2;
        case 1000:
            return 3;
        default:
            return 0;
    }
}

// Retain both switch functions and their selector in the executable image.
extern "C" __declspec(noinline) void fixture_entry() {
    fixture_sink = switch_fixture(fixture_selector);
    fixture_sink += switch_fixture_negative(fixture_selector);
}
