# Core Domain

This directory contains immutable, serializable vocabulary shared by the C++23 services and runtime.

- [`domain.cppm`](domain.cppm) re-exports the individual value modules.
- Address identity and arithmetic are defined by [`address_space.cppm`](address_space.cppm), [`address.cppm`](address.cppm), [`address_range.cppm`](address_range.cppm), and [`address_factory.cppm`](address_factory.cppm).
- Decoded machine data is represented by [`instruction.cppm`](instruction.cppm), [`operand.cppm`](operand.cppm), [`pcode.cppm`](pcode.cppm), and [`flow.cppm`](flow.cppm).
- Project snapshots are represented by [`memory_region.cppm`](memory_region.cppm), [`function.cppm`](function.cppm), [`reference.cppm`](reference.cppm), [`symbol.cppm`](symbol.cppm), and the data-type modules.

The types are adapted from the Java SoftwareModeling address/listing/p-code contracts and the native Ghidra `address.hh`, `pcoderaw.hh`, and `translate.hh` values. Domain code does not own a project, service, file, native engine, or database.
