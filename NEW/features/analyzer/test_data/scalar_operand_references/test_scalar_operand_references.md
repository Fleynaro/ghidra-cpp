# Scalar Operand References Behavioral Fixture

> Generated from actual Ghidra instruction and operand-reference snapshots.

## Input

- **Executable:** `test_scalar_operand_references.exe`
- **Executable bytes:** `3072`

## Analysis Configuration

| Enabled boolean option |
| --- |
| `Scalar Operand References` |

## Scalar Operands

| Phase | Instruction | Operand index | Text | Unsigned scalar | Operand references | Outcome |
| --- | --- | --- | --- | ---: | --- | --- |
| `Before` | `0x0000000140001000` | `1` | `0x140003000` | `0x0000000140003000` | `` | `Rejected address-like control` |
| `Before` | `0x0000000140001014` | `1` | `0x140001000` | `0x0000000140001000` | `` | `Rejected address-like control` |
| `Before` | `0x0000000140001028` | `1` | `0x12345678` | `0x0000000012345678` | `` | `Rejected address-like control` |
| `Before` | `0x0000000140001034` | `1` | `0x11` | `0x0000000000000011` | `` | `Rejected small numeric control` |
| `Before` | `0x0000000140001040` | `1` | `0x28` | `0x0000000000000028` | `` | `Rejected small numeric control` |
| `Before` | `0x00000001400010A9` | `1` | `0x28` | `0x0000000000000028` | `` | `Rejected small numeric control` |
| `After` | `0x0000000140001000` | `1` | `0x140003000` | `0x0000000140003000` | `0x0000000140003000` | `Analyzer-created address reference` |
| `After` | `0x0000000140001014` | `1` | `0x140001000` | `0x0000000140001000` | `0x0000000140001000` | `Analyzer-created address reference` |
| `After` | `0x0000000140001028` | `1` | `0x12345678` | `0x0000000012345678` | `` | `Rejected address-like control` |
| `After` | `0x0000000140001034` | `1` | `0x11` | `0x0000000000000011` | `` | `Rejected small numeric control` |
| `After` | `0x0000000140001040` | `1` | `0x28` | `0x0000000000000028` | `` | `Rejected small numeric control` |
| `After` | `0x00000001400010A9` | `1` | `0x28` | `0x0000000000000028` | `` | `Rejected small numeric control` |

## Fixture Assertions

- **Positive analyzer references after analysis:** `2`.
- **Negative controls without references after analysis:** `4`.
- Positive values are fixed image addresses; negative rows retain the rejected address-like and small numeric controls.
