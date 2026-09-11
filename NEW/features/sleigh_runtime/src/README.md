# Runtime Sources

This directory contains the private C++23 implementation of the standalone Sleigh runtime. The files implement packed decoding, SLA format decompression, address spaces, patterns, constructors, parser state, handles, templates, and p-code generation. Each module partition is kept directly in this directory.

The runtime source set is listed by the CMake file in [`../CMakeLists.txt`](../CMakeLists.txt). It is deliberately limited to the Sleigh decoder and its low-level dependencies; it does not include the `.slaspec` parser/compiler, ProgramDB, Listing, GUI, or decompiler analysis code.

The public adapter is [`../sleigh_runtime_adapter.cppm`](../sleigh_runtime_adapter.cppm), and the module overview is [`../README.md`](../README.md). The private partition is [`internal.cppm`](internal.cppm), which re-exports the self-contained partitions in this directory. The runtime-specific raw storage value is [`varnode.cppm`](varnode.cppm).
