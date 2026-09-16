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

The provider contract's [`ConstantFormatDescription`](decompiler.cppm) supports an optional encoded width,
which is applied by [`decompiler_impl.cppm`](decompiler_impl.cppm) before native dynamic-symbol formatting.
The x86 long-double and forced integer-format coverage is implemented in
[`../tests/decompiler_datatests.cppm`](../tests/decompiler_datatests.cppm).

`DecompilationResult` also exposes the native Clang XML markup and value-owned
[`PcodeOpProvenance`](decompiler.cppm) / [`VarnodeProvenance`](decompiler.cppm)
snapshots. `decompiler_impl.cppm` captures these from the original
`EmitMarkup`/`Funcdata` graph, preserving `opref`/`varref` joins to originating
instruction addresses. The bidirectional contract is exercised by
[`../tests/provenance_mapping_tests.cppm`](../tests/provenance_mapping_tests.cppm),
which uses real x86-64 Sleigh decoding and checks nested fields, loops,
conditionals, switch cases, calls, locals, parameters, and arithmetic.
