// MSVC x86 fixture for PropagateExternalParametersAnalyzer.java.
//
// Exact original source reference:
// * Ghidra/Features/MicrosoftCodeAnalyzer/src/main/java/ghidra/app/plugin/prototype/MicrosoftCodeAnalyzerPlugin/PropagateExternalParametersAnalyzer.java,
//   lines 240-265
//   enumerate external function symbols and their parameters.
// * The same file's processExternalFunction() lines 52-75 accepts a CALL after enough
//   PUSH instructions, while hasEnoughPushes() lines 157-180 counts PUSH mnemonics.
// * propogateParams() lines 182-226 assigns each pushed argument its external parameter
//   name and type in an EOL comment.
// * added() lines 274-305 then follows address operands, creates labels/comments, clears
//   undefined data, and creates typed data at referenced addresses.
//
// These behaviors are x86-specific because the original implementation compares the
// literal instruction mnemonic PUSH and the x86 OperandType address/data flags. The
// build.bat therefore uses the 32-bit MSVC environment rather than silently compiling
// an x64 binary that cannot reach this path. run_ghidra.py extracts the resulting
// parameter and address comments from the analyzed listing.

extern "C" __declspec(dllimport) int __stdcall MessageBoxA(void* window, const char* text, const char* caption,
                                                           unsigned int type);

// MSVC normally lowers a dllimport call directly through the IAT. The original
// PropagateExternalParametersAnalyzer.java also has a dedicated thunk path in
// processThunkReference() (lines 79-100), so this naked thunk deliberately emits
// the PE import JMP shape that that branch expects.
extern "C" __declspec(naked) int __stdcall fixture_message_box_thunk(void*, const char*, const char*, unsigned int) {
    __asm {
        jmp MessageBoxA
    }
}

// The analyzer must see real memory operands for the first two pushed parameters.
extern "C" const char parameter_text[] = "external parameter text";
extern "C" const char parameter_caption[] = "external parameter caption";

// This value is referenced by a non-pointer argument and supplies a negative control.
extern "C" const unsigned int parameter_flags = 0x40U;

// The four pushes are intentionally adjacent to the imported call so the analyzer's
// backward iterator observes them in the order required by its parameter loop.
extern "C" __declspec(noinline) int propagate_parameters() {
    return fixture_message_box_thunk(nullptr, parameter_text, parameter_caption, parameter_flags);
}

// Keep the PE entry self-contained and avoid CRT startup/termination code.
extern "C" __declspec(noreturn) void fixture_entry() {
    (void)propagate_parameters();
    for (;;) {
    }
}
