# Runtime Sources

[`../sleigh_runtime_adapter.cppm`](../sleigh_runtime_adapter.cppm) owns the in-memory `LoadImage`, captures assembly and p-code callbacks, and drives the compiled translator. [`ghidra/detail/varnode.cppm`](ghidra/detail/varnode.cppm) contains the raw storage triple shared by address, context, and emission code. The full internal implementation is documented in [`ghidra/README.md`](ghidra/README.md) and [`ghidra/detail/README.md`](ghidra/detail/README.md).

The parent module and build target are documented in [`../README.md`](../README.md) and [`../CMakeLists.txt`](../CMakeLists.txt).
