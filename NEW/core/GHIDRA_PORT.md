# Ghidra Port Evidence

The canonical values in [`domain`](domain/README.md) consolidate behavior studied in Ghidra Java `ghidra.program.model.address`, `ghidra.program.model.listing`, `ghidra.program.model.pcode`, and native decompiler `address.hh`, `pcoderaw.hh`, and `translate.hh`. The event payload helpers preserve the source/priority/provenance distinctions used by `Symbol`, `Function`, `Reference`, `MemoryBlock`, and analyzer result objects.

This is an independent C++23 value layer, not a runtime wrapper around Java objects or the old mutable `AnalysisContext`. All values are copyable snapshots or explicit non-owning views; persistence and native engine ownership remain outside core.
