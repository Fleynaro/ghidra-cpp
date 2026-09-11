# Ghidra Runtime Core

This directory contains the private C++23 implementation of the standalone Sleigh runtime. The files implement packed decoding, SLA format decompression, address spaces, patterns, constructors, parser state, handles, templates, and p-code generation.

The runtime source set is listed by the CMake file in [`../../CMakeLists.txt`](../../CMakeLists.txt). It is deliberately limited to the Sleigh decoder and its low-level dependencies; it does not include the `.slaspec` parser/compiler, ProgramDB, Listing, GUI, or decompiler analysis code.

The public adapter is [`../../sleigh_runtime_adapter.cppm`](../../sleigh_runtime_adapter.cppm), and the module overview is [`../../README.md`](../../README.md). The private primary module is [`detail/internal.cppm`](detail/internal.cppm); its partitions provide declarations through imports, while the parent `.cppm` files provide implementation units attached to the same module. The runtime-specific raw storage value is [`detail/varnode.cppm`](detail/varnode.cppm).
