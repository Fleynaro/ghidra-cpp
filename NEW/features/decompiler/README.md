# Standalone Native Decompiler Port

This module is a mechanical standalone source port of the portable native Ghidra decompiler engine. It preserves the original `.cc` and header implementations and does not replace algorithms with adapters or placeholders. The `NewGhidra::Decompiler` target contains the C++11-compatible engine; `NewGhidra::DecompilerFrontend` adds the C++23 provider façade and links it to `NewGhidra::SleighRuntime`.

## Source Mapping

- [`CMakeLists.txt`](CMakeLists.txt) contains the explicit translation-unit mapping used by the initial static-library target.
- Every file under [`src`](src) has the same basename as its authority file under [`Ghidra/Features/Decompiler/src/decompile/cpp`](../../../Ghidra/Features/Decompiler/src/decompile/cpp) and carries a relative provenance comment.
- The selected implementation set follows the original `CORE` and `DECCORE` partitions in the authority [`Makefile`](../../../Ghidra/Features/Decompiler/src/decompile/cpp/Makefile).
- `error.hh`, `partmap.hh`, `rangemap.hh`, and `types.h` are supporting headers required by the selected engine sources.
- [`src/decompiler.cppm`](src/decompiler.cppm) defines the clean provider contract for p-code, memory, architecture, functions, and bounded decompilation results.
- [`src/decompiler.cpp`](src/decompiler.cpp) implements `Translate`, `LoadImage`, compiler-prototype bootstrap, `Architecture`, `Funcdata`, action, SSA, and C-printer integration without Java, IPC, or Sleigh internals.

## Exclusions

The engine target intentionally excludes Sleigh compiler/parser/runtime sources, `ghidra_*` and other Ghidra-host adapters, debug/interface sources, tests and demos, and loader-specific implementations. The frontend consumes only the public `sleigh_runtime` value API; it never imports `sleigh_runtime:internal` or duplicates an instruction decoder.

## Build

Build the complete project through the repository entry point so MSVC modules, vcpkg, Sleigh Runtime, and Google Test are configured together:

```powershell
cmd /c NEW/build.bat
```

The outputs are `new_ghidra_decompiler.lib`, `new_ghidra_decompiler_frontend.lib`, and `decompiler_tests.exe`. The tests cover provider p-code materialization, native flow/SSA/printer execution, and explicit machine-code-to-C end-to-end cases based on `TEST/dec_code_examples/1.md`, `2.md`, and `3.md`. The PE fixture tests require the repository's optional `TEST/test.exe` fixture; synthetic PE tests remain independent of it.

The imported Ghidra source is licensed under Apache 2.0. Each copied source retains the upstream banner and an explicit relative provenance comment.
