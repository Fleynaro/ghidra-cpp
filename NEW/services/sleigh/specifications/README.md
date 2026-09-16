# Processor Specifications

[`x86-64.sla`](x86-64.sla) and [`ARM8_le.sla`](ARM8_le.sla) are module-level binary compiled Sleigh specifications copied from the repository fixtures [`../specifications/x86-64.sla`](../specifications/x86-64.sla) and [`../specifications/ARM8_le.sla`](../specifications/ARM8_le.sla). The runtime resolver exposes this directory to library, decompiler, test, and CLI consumers without requiring test-only paths.

No `.slaspec` source or Sleigh compiler is included. The parent module is documented in [`../README.md`](../README.md).

## Local Inventory

The runtime and decompiler tests use only the compiled fixtures checked into this directory:

| Family | Local SLA | Representation |
| --- | --- | --- |
| ARM32 little-endian | [`ARM8_le.sla`](ARM8_le.sla) | Real local decode and provider smoke test |
| x86-64 little-endian | [`x86-64.sla`](x86-64.sla) | Real local decode and provider smoke test |

Other architecture families are covered by provider-only context tests until their compiled SLA
fixtures are added locally. Compiled SLAs are opaque, generated, version-sensitive artifacts;
keeping them in this module makes the runtime independent of any installed Ghidra distribution.
