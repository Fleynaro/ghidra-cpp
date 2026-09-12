# Standalone Native Decompiler Port

This module is a mechanical standalone source port of the portable native Ghidra decompiler engine. It preserves the original implementations and does not replace algorithms with adapters or placeholders. The engine is exported from the aggregate `ghidra.decompiler` C++23 module; `block_switch.cppm` is a separate named module for the mutually dependent switch hierarchy. `NewGhidra::DecompilerFrontend` adds the provider façade and links to `NewGhidra::SleighRuntime` only for the optional decoder-facing frontend.

## Source Mapping

- [`CMakeLists.txt`](CMakeLists.txt) explicitly separates the aggregate and restored named module interfaces from the per-source C++23 implementation units.
- [`src/xml.cppm`](src/xml.cppm) preserves the original `ghidra::Element`, `Document`, `DocumentStorage`, and `ContentHandler` API while loading through the vcpkg [`pugixml`](https://github.com/zeux/pugixml) dependency.
- Every active implementation unit under [`src`](src) carries a relative provenance comment to its authority file under [`Ghidra/Features/Decompiler/src/decompile/cpp`](../../../Ghidra/Features/Decompiler/src/decompile/cpp).
- The selected implementation set follows the original `CORE` and `DECCORE` partitions in the authority [`Makefile`](../../../Ghidra/Features/Decompiler/src/decompile/cpp/Makefile), including the restored grammar parser, Renoir graph exporter, signature generator, and parameter-report subsystem.
- [`src/grammar.cppm`](src/grammar.cppm), [`src/graph.cppm`](src/graph.cppm), [`src/signature.cppm`](src/signature.cppm), and [`src/paramid.cppm`](src/paramid.cppm) preserve the removed public APIs as named C++23 modules and compile the authoritative original implementations without activating legacy `.cc` or `.hh` files.
- The former `error.hh`, `partmap.hh`, `rangemap.hh`, and `types.h` declarations are included in the aggregate module interface; no legacy header unit remains active.
- [`src/decompiler.cppm`](src/decompiler.cppm) defines the clean provider contract for p-code, memory, architecture, functions, and bounded decompilation results. `LOAD` and `STORE` name their target through `memory_space`; their operands are the address/value operands, with matching legacy constant-space selectors accepted for compatibility.
- [`src/decompiler_impl.cppm`](src/decompiler_impl.cppm) implements `Translate`, `LoadImage`, provider-derived compiler-prototype bootstrap, namespace-aware symbols, exact custom prototype storage/conventions, provider type metadata, action, SSA, and C-printer integration without Java, IPC, or Sleigh internals. The Sleigh façade retries mapped windows down from 16 bytes and validates instruction lengths, addresses, storage ranges, and opcodes before entering the native engine.
- [`src/userop.cppm`](src/userop.cppm) lazily creates unspecialized descriptions for provider `CALLOTHER` operations that are absent from the minimal architecture model.

## Exclusions

The engine target intentionally excludes Sleigh compiler/parser/runtime sources, tests and demos, and loader-specific implementations. The frontend consumes only the public `sleigh_runtime` value API; it never imports `sleigh_runtime:internal` or duplicates an instruction decoder. The optional dynamic rule compiler remains disabled and reports the original unsupported-feature error.

## Build

Build the complete project through the repository entry point so MSVC modules, vcpkg, Sleigh Runtime, and Google Test are configured together:

```powershell
cmd /c NEW/build.bat
```

The outputs are `new_ghidra_decompiler.lib`, `new_ghidra_decompiler_frontend.lib`, and `decompiler_tests.exe`. The tests cover provider p-code materialization, generic provider architectures, exact prototype/type/symbol metadata, bounded arithmetic/opcode validation, native flow/SSA/printer execution, and explicit machine-code-to-C end-to-end cases based on `TEST/dec_code_examples/1.md` through `11.md`. The PE fixture tests require the repository's optional `TEST/test.exe` fixture; synthetic PE tests remain independent of it.

The imported Ghidra source is licensed under Apache 2.0. Each copied source retains the upstream banner and an explicit relative provenance comment. The pugixml library is consumed through the NEW project manifest and remains an implementation detail of the XML compatibility layer.
