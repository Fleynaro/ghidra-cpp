// MSVC x64 fixture for Ghidra's Microsoft Demangler analyzer.
//
// Exact Java implementation and dependency references:
// * Ghidra/Features/MicrosoftDemangler/src/main/java/ghidra/app/plugin/core/analysis/MicrosoftDemanglerAnalyzer.java
//   (the exact "Demangler Microsoft" option, canAnalyze() delegation, custom options, and
//   doDemangle() callback).
// * Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/analysis/AbstractDemanglerAnalyzer.java
//   (source-type filtering, global-symbol traversal, demangle/apply behavior, and signature
//   application rules).
// * Ghidra/Features/MicrosoftDemangler/src/main/java/ghidra/app/util/demangler/microsoft/MicrosoftDemangler.java
//   (Microsoft decorated-name parsing and conversion to DemangledObject).
// * Ghidra/Features/MicrosoftDemangler/src/test/java/ghidra/app/plugin/core/analysis/MicrosoftDemanglerAnalyzerTest.java
//   (signature application option behavior for a mangled function symbol).
//
// Tested behavior:
// * Calculator::add and Calculator::scale are exported C++ methods, so MSVC emits decorated
//   global symbols that the PE loader presents to the analyzer. Their names and signatures must
//   be converted to readable Microsoft C++ forms, with parameters applied when configured.
// * the C-linkage fixture_entry symbol is a negative case: it is not a Microsoft-decorated name
//   and must not be reported as a demangler discovery.
//
// Why these constructs are chosen:
// * a namespace, class, instance method, and static method exercise namespace qualification,
//   member calling convention, and ordinary parameter parsing without depending on PDB files.
// * dllexport makes the decorated names part of the deterministic PE export table; no hand-made
//   symbol injection or fabricated demangler output is necessary.
// * noinline and volatile state retain both methods in the executable under the CRT-free link.
//
// Expected MSVC artifacts and Ghidra discoveries:
// * build.bat emits a PE32+ image with genuine MSVC decorated exports such as ?add@Calculator@...
//   and ?scale@Calculator@..., plus no CRT imports.
// * Ghidra should retain each mangled symbol and add a demangled label/signature. Exact addresses
//   are intentionally extracted from the program, while exact names are taken from Ghidra's
//   post-analysis symbol table rather than guessed in this source.
// * run_ghidra.py extracts only decorated symbols and their resulting demangled names/signatures.

extern "C" volatile int fixture_sink = 0;

namespace fixture {

// Group exported methods under a namespace/class so MSVC emits nested decorated names.
class Calculator {
public:
    // Export an instance method whose decorated name includes a class and namespace.
    __declspec(dllexport) __declspec(noinline) int add(int left, int right);
    // Export a static method to exercise the alternate Microsoft calling convention encoding.
    __declspec(dllexport) __declspec(noinline) static int scale(int value);
};

// Add two values and retain a non-trivial member signature for demangling.
int Calculator::add(int left, int right) {
    fixture_sink = left + right;
    return fixture_sink;
}

// Scale one value and retain a non-trivial static signature for demangling.
int Calculator::scale(int value) {
    fixture_sink = value * 3;
    return fixture_sink;
}

} // namespace fixture

// Retain both decorated methods while keeping the entry symbol deliberately undecorated.
extern "C" __declspec(noinline) void fixture_entry() {
    fixture::Calculator calculator;
    fixture_sink = calculator.add(4, 5);
    fixture_sink += fixture::Calculator::scale(6);
}
