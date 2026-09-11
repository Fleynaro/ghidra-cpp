# Ghidra Runtime Core

This directory contains the standalone C++ Sleigh runtime sources copied from [`../../../../../Ghidra/Features/Decompiler/src/decompile/cpp`](../../../../../Ghidra/Features/Decompiler/src/decompile/cpp). The files implement packed decoding, SLA format decompression, address spaces, patterns, constructors, parser state, handles, templates, and p-code generation.

The runtime source set is listed by the CMake file in [`../../CMakeLists.txt`](../../CMakeLists.txt). It is deliberately limited to the Sleigh decoder and its low-level dependencies; it does not include the `.slaspec` parser/compiler, ProgramDB, Listing, GUI, or decompiler analysis code.

The public adapter is [`../../sleigh_runtime_adapter.cppm`](../../sleigh_runtime_adapter.cppm), and the module overview is [`../../README.md`](../../README.md). Internal declaration units are grouped under [`detail/README.md`](detail/README.md); the runtime-specific raw storage value is [`detail/varnode.cppm`](detail/varnode.cppm).
