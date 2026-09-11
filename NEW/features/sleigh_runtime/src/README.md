# Runtime Sources

[`../sleigh_runtime_adapter.cppm`](../sleigh_runtime_adapter.cppm) owns the in-memory `LoadImage`, captures assembly and p-code callbacks, and drives the compiled translator. The self-contained Ghidra module partitions under [`ghidra/`](ghidra/) contain each declaration beside its implementation. The module layout is documented in [`ghidra/README.md`](ghidra/README.md).

The parent module and build target are documented in [`../README.md`](../README.md) and [`../CMakeLists.txt`](../CMakeLists.txt).
