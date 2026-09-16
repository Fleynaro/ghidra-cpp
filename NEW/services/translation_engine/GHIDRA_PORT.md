# Ghidra Port Evidence

The shared translation boundary follows the original native contracts in `Ghidra/Features/Decompiler/src/decompile/cpp/address.hh`, `space.hh`, `translate.hh`, `loadimage.hh`, `globalcontext.hh`, `marshal.hh`, `opcodes.hh`, and `pcoderaw.hh`. Those sources define the common substrate consumed by both `Sleigh` and `Architecture`.

The current MVP adapter records the stable language/space metadata in [`native/native_translation.cppm`](native/native_translation.cppm) while the complete algorithmic copies remain compiled by [`../../features/sleigh_runtime`](../../features/sleigh_runtime/README.md) and [`../../features/decompiler`](../../features/decompiler/README.md). No native pointer crosses [`../../core/contracts`](../../core/contracts/README.md). Full physical source consolidation remains a targeted build cleanup after the canonical service adapters are validated.
