# Scalar Operand References Behavioral Fixture

> Generated from actual Ghidra instruction and operand-reference snapshots around the target analyzer.

## Input

- **Executable:** `test_scalar_operand_references.exe`
- **Executable bytes:** `3072`

## Analysis Configuration

| Enabled boolean option |
| --- |
| `Scalar Operand References` |

## Before target analysis
### Scalar operands visible before target analysis

| Instruction | Operand index | Text | Unsigned scalar | Operand references | Outcome | Provenance |
| --- | --- | --- | ---: | --- | --- | --- |
| `0x0000000140001000` | `1` | `0x140003000` | `0x0000000140003000` | `` | `Rejected address-like control` | Pre-existing disassembly state |
| `0x0000000140001014` | `1` | `0x140001000` | `0x0000000140001000` | `` | `Rejected address-like control` | Pre-existing disassembly state |
| `0x0000000140001028` | `1` | `0x12345678` | `0x0000000012345678` | `` | `Rejected address-like control` | Pre-existing disassembly state |
| `0x0000000140001034` | `1` | `0x11` | `0x0000000000000011` | `` | `Rejected small numeric control` | Pre-existing disassembly state |
| `0x0000000140001040` | `1` | `0x28` | `0x0000000000000028` | `` | `Rejected small numeric control` | Pre-existing disassembly state |
| `0x00000001400010A9` | `1` | `0x28` | `0x0000000000000028` | `` | `Rejected small numeric control` | Pre-existing disassembly state |

## After target analysis
### Scalar operands visible after target analysis

| Instruction | Operand index | Text | Unsigned scalar | Operand references | Outcome | Provenance |
| --- | --- | --- | ---: | --- | --- | --- |
| `0x0000000140001000` | `1` | `0x140003000` | `0x0000000140003000` | `0x0000000140003000` | `Analyzer-created address reference` | Post-target operand state |
| `0x0000000140001014` | `1` | `0x140001000` | `0x0000000140001000` | `0x0000000140001000` | `Analyzer-created address reference` | Post-target operand state |
| `0x0000000140001028` | `1` | `0x12345678` | `0x0000000012345678` | `` | `Rejected address-like control` | Post-target operand state |
| `0x0000000140001034` | `1` | `0x11` | `0x0000000000000011` | `` | `Rejected small numeric control` | Post-target operand state |
| `0x0000000140001040` | `1` | `0x28` | `0x0000000000000028` | `` | `Rejected small numeric control` | Post-target operand state |
| `0x00000001400010A9` | `1` | `0x28` | `0x0000000000000028` | `` | `Rejected small numeric control` | Post-target operand state |

## Delta

The delta is keyed by instruction address and operand index, so added, removed, and changed operand-reference outcomes are explicit.

| Change | Instruction | Operand | Before references/outcome | After references/outcome |
| --- | --- | --- | --- | --- |
| Changed | `0x0000000140001000` | `1` | ` / Rejected address-like control` | `0x0000000140003000 / Analyzer-created address reference` |
| Changed | `0x0000000140001014` | `1` | ` / Rejected address-like control` | `0x0000000140001000 / Analyzer-created address reference` |

### Delta conclusion

- **Positive analyzer references after analysis:** `2`.
- **Negative controls without references after analysis:** `4`.
- **Rows added:** `0`; removed: `0`; changed: `2`.
- Positive values are fixed image addresses; negative rows retain the rejected address-like and small numeric controls.
