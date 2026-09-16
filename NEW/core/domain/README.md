# Core Domain

This directory contains immutable, serializable vocabulary shared by the C++23 services and runtime.

- [`domain.cppm`](domain.cppm) re-exports the individual value modules.
- Address identity and arithmetic are defined by [`address_space.cppm`](address_space.cppm), [`address.cppm`](address.cppm), [`address_range.cppm`](address_range.cppm), and [`address_factory.cppm`](address_factory.cppm).
- Decoded machine data is represented by [`instruction.cppm`](instruction.cppm), [`operand.cppm`](operand.cppm), [`pcode.cppm`](pcode.cppm), and [`flow.cppm`](flow.cppm).
- Portable decoder snapshots shared by Sleigh and decompiler adapters are represented by [`decoded_instruction.cppm`](decoded_instruction.cppm). The native runtime and frontend expose aliases to these values rather than defining parallel DTOs.
- `DecodedInstruction` is the transient numeric-address boundary; [`instruction.cppm`](instruction.cppm) remains the space-aware listing/persistence snapshot. [`NEW/services/sleigh/sleigh_runtime.cppm`](../../services/sleigh/sleigh_runtime.cppm) and [`NEW/services/decompiler/src/decompiler.cppm`](../../services/decompiler/src/decompiler.cppm) must alias the transient values instead of declaring service DTOs.
- Project snapshots are represented by [`memory_region.cppm`](memory_region.cppm), [`function.cppm`](function.cppm), [`reference.cppm`](reference.cppm), [`symbol.cppm`](symbol.cppm), and the data-type modules.

The module is compiled by [`../CMakeLists.txt`](../CMakeLists.txt), re-exported by [`domain.cppm`](domain.cppm), and validated by [`../tests/domain_tests.cppm`](../tests/domain_tests.cppm). Runtime promotion into the listing snapshot is covered by [`../../services/sleigh/sleigh_service.cppm`](../../services/sleigh/sleigh_service.cppm); native frontend materialization is covered by [`../../services/decompiler/src/decompiler_impl.cppm`](../../services/decompiler/src/decompiler_impl.cppm) and its provider tests.

The types are adapted from the Java SoftwareModeling address/listing/p-code contracts and the native Ghidra `address.hh`, `pcoderaw.hh`, and `translate.hh` values. Domain code does not own a project, service, file, native engine, or database.
