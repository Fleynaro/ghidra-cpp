# Ghidra Port Evidence

The adapter preserves the native `Architecture`, raw flow, SSA, action, and C-printing sequence from `Ghidra/Features/Decompiler/src/decompile/cpp/architecture.cc`, `funcdata.cc`, `flow.cc`, `action.cc`, and `printc.cc`. Provider-local behavior is supplied through the service-owned [`src/decompiler.cppm`](src/decompiler.cppm) interfaces.

The runtime-facing result is a canonical value and is revision-stamped. It does not expose `Funcdata`, `Architecture`, native varnodes, or C++ pointers to the project facade.
