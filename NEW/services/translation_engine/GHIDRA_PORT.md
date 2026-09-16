# Ghidra Port Evidence

The shared translation boundary follows the original native contracts in `Ghidra/Features/Decompiler/src/decompile/cpp/address.hh`, `space.hh`, `translate.hh`, `loadimage.hh`, `globalcontext.hh`, `marshal.hh`, `opcodes.hh`, and `pcoderaw.hh`. Those sources define the common substrate consumed by both `Sleigh` and `Architecture`.

The shared service records stable language/space metadata in [`native/native_translation.cppm`](native/native_translation.cppm), while the migrated Sleigh and decompiler native implementations are compiled from [`../sleigh/src`](../sleigh/src) and [`../decompiler/src`](../decompiler/src). No native pointer crosses [`../../core/contracts`](../../core/contracts/README.md).
