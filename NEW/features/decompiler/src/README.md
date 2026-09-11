# Ported Engine Sources

This directory contains the native Ghidra decompiler engine copied mechanically from
[`Ghidra/Features/Decompiler/src/decompile/cpp`](../../../../Ghidra/Features/Decompiler/src/decompile/cpp).
The explicit source selection is maintained in [`../CMakeLists.txt`](../CMakeLists.txt), and every
ported translation unit retains a provenance comment naming its upstream file. The directory
contains address/space and p-code primitives, `Funcdata`, flow/heritage/merge/type recovery,
action rules, structured control-flow support, and the C/Java printers.

The standalone boundary is implemented by [`decompiler.cpp`](decompiler.cpp) and declared by
[`decompiler.cppm`](decompiler.cppm). Those files are adapters only: all analysis algorithms run
from the ported engine sources and p-code enters through the provider contract. Sleigh compiler,
parser, and decoder implementations are intentionally absent; the production decoder remains in
[`../../sleigh_runtime`](../../sleigh_runtime).
