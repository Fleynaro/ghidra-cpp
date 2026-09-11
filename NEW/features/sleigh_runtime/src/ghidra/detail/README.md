# Sleigh Runtime Detail Units

This directory contains the private C++23 module interface and partitions used by the standalone Sleigh runtime. [`internal.cppm`](internal.cppm) is the primary private module; the other `.cppm` files are partitions imported with `import :partition`. Parent units in [`../`](../) are implementation units attached to the same `sleigh_runtime.ghidra` module. None is part of the public `sleigh_runtime` API.

The low-level groups are organized as follows:

- [`address.cppm`](address.cppm), [`space.cppm`](space.cppm), and [`translate.cppm`](translate.cppm) describe address spaces and translation contracts.
- [`varnode.cppm`](varnode.cppm), [`context.cppm`](context.cppm), and [`globalcontext.cppm`](globalcontext.cppm) hold raw storage and parser context state.
- [`marshal.cppm`](marshal.cppm), [`compression.cppm`](compression.cppm), [`slaformat.cppm`](slaformat.cppm), and [`xml.cppm`](xml.cppm) load compiled SLA data.
- [`slghpattern.cppm`](slghpattern.cppm), [`slghpatexpress.cppm`](slghpatexpress.cppm), [`semantics.cppm`](semantics.cppm), [`slghsymbol.cppm`](slghsymbol.cppm), [`sleighbase.cppm`](sleighbase.cppm), and [`sleigh.cppm`](sleigh.cppm) implement matching, constructor state, and p-code emission.
- [`types.cppm`](types.cppm), [`error.cppm`](error.cppm), [`opcodes.cppm`](opcodes.cppm), [`partmap.cppm`](partmap.cppm), and [`loadimage.cppm`](loadimage.cppm) provide shared primitives and the byte-loading contract.

The parent implementation units are listed by [`../README.md`](../README.md), and the feature-level build and public API are documented by [`../../README.md`](../../README.md).
