# Ghidra Port Evidence

The service preserves behavior from `Ghidra/Features/Base/src/main/java/ghidra/app/util/opinion/PeLoader.java`, `PortableExecutable.java`, `NTHeader.java`, `OptionalHeader.java`, and the PE directory classes. The checked parser/value model remains in [`../../services/pe_loader/src/pe_loader.cppm`](../../services/pe_loader/src/pe_loader.cppm) and is not simplified.

This adapter intentionally separates parser facts from original `Program`/`MemoryBlock` side effects. Function and symbol creation is represented by runtime events and projection commands instead of hidden mutation during parsing.

Named imports/exports and base-relocation entries are promoted into canonical `core::Symbol` and `core::Relocation` values in the `PeLoadResult` contract. Parser-specific ordinal/hint and directory diagnostics remain in the native detail model because those fields have no direct domain equivalent yet.
