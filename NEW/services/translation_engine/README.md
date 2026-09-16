# Translation Engine

This target is the private shared boundary for native translation metadata used by [`../sleigh`](../sleigh/README.md) and [`../decompiler`](../decompiler/README.md).

- [`native/native_translation.cppm`](native/native_translation.cppm) re-exports `core::ArchitectureDescription` as the shared metadata contract used by adapters; it does not own a parallel architecture struct.
- The original low-level native ports remain in the current feature targets during the MVP compatibility migration; their source mappings are recorded in [`GHIDRA_PORT.md`](GHIDRA_PORT.md).
- Consumers expose only [`../../core`](../../core/README.md) values and never expose native `Address`, `AddrSpace`, `LoadImage`, or `VarnodeData` pointers.
