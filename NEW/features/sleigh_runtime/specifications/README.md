# Processor Specifications

[`x86-64.sla`](x86-64.sla) and [`ARM8_le.sla`](ARM8_le.sla) are module-level binary compiled Sleigh specifications copied from the repository fixtures [`../specifications/x86-64.sla`](../specifications/x86-64.sla) and [`../specifications/ARM8_le.sla`](../specifications/ARM8_le.sla). The runtime resolver exposes this directory to library, decompiler, test, and CLI consumers without requiring test-only paths.

No `.slaspec` source or Sleigh compiler is included. The parent module is documented in [`../README.md`](../README.md).

## Installed Architecture Inventory

The architecture-provider smoke tests intentionally resolve a small representative set from
`GHIDRA_INSTALL_DIR` instead of copying generated binary artifacts into this repository. The
paths are relative to the installation root and therefore work on any machine with the same
Ghidra distribution layout.

| Family | Relative SLA | Representation | Reason |
| --- | --- | --- | --- |
| MIPS32 big-endian | `Ghidra/Processors/MIPS/data/languages/mips32be.sla` | Installed at test time | Representative 32-bit ABI and endianness |
| ARM32 little-endian | `Ghidra/Processors/ARM/data/languages/ARM8_le.sla` | Installed at test time; module specification retained for runtime tests and CLI examples | Existing repository fixture and ARM smoke coverage |
| AArch64 little-endian | `Ghidra/Processors/AARCH64/data/languages/AARCH64.sla` | Installed at test time | Representative 64-bit ARM ABI |
| PPC32 big-endian | `Ghidra/Processors/PowerPC/data/languages/ppc_32_be.sla` | Installed at test time | Representative 32-bit PowerPC ABI |
| 68000 family | `Ghidra/Processors/68000/data/languages/68020.sla` | Installed at test time | Representative classic 68k core |
| 8051 | `Ghidra/Processors/8051/data/languages/8051.sla` | Installed at test time | Representative Harvard-style multi-space processor |
| x86-32 | `Ghidra/Processors/x86/data/languages/x86.sla` | Installed at test time | Required 32-bit x86 coverage |
| Toy | none installed | Provider-only `ArchitectureProviderContext` | The installed `Toy/data` contains only `build.xml`, not a compiled SLA |

Compiled SLAs are opaque, generated, version-sensitive artifacts. Keeping only the existing
module-local fixtures and resolving the selected installed files at runtime avoids stale copies,
machine-specific paths, and an unreviewable expansion to every processor variant.
