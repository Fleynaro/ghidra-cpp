# Ported Engine Sources

This directory contains the native Ghidra decompiler engine copied mechanically from
[`Ghidra/Features/Decompiler/src/decompile/cpp`](../../../../Ghidra/Features/Decompiler/src/decompile/cpp).
The module source selection is maintained in [`../CMakeLists.txt`](../CMakeLists.txt), and every
ported implementation unit retains a provenance comment naming its upstream file. The aggregate
interface [`ghidra_decompiler.cppm`](ghidra_decompiler.cppm) contains the mutually dependent
declarations, while the remaining `.cppm` files preserve each original implementation unit.
The directory contains address/space and p-code primitives, `Funcdata`, flow/heritage/merge/type
recovery, action rules, structured control-flow support, the C/Java printers, the C declaration
grammar, Renoir graph export, graph signatures, and parameter measurements.

The standalone boundary is implemented by [`decompiler_impl.cppm`](decompiler_impl.cppm) and declared by
[`decompiler.cppm`](decompiler.cppm). Those files are adapters only: all analysis algorithms run
from the ported engine sources and p-code enters through the provider contract. Sleigh compiler
and decoder implementations are intentionally absent; the production decoder remains in
[`../../sleigh_runtime`](../../sleigh_runtime).
