# Reference Behavioral Fixture

> Generated from actual Ghidra reference-manager snapshots before and after analysis.

## Input

- **Executable:** `test_reference.exe`
- **Executable bytes:** `3072`

## Analysis Configuration

| Enabled boolean option |
| --- |
| `Reference` |

## Reference Provenance

| Phase | Source | Target | Operand | Type | Ghidra source | Provenance |
| --- | --- | --- | --- | --- | --- | --- |
| Before | `0x0000000140001000` | `0x0000000140003000` | `1` | `READ` | `DEFAULT` | `Pre-existing disassembler reference` |
| Before | `0x0000000140001015` | `0x0000000140003000` | `0` | `WRITE` | `DEFAULT` | `Pre-existing disassembler reference` |
| Before | `0x000000014000101C` | `0x0000000140003000` | `1` | `READ` | `DEFAULT` | `Pre-existing disassembler reference` |
| Before | `0x0000000140001039` | `0x0000000140003000` | `0` | `WRITE` | `DEFAULT` | `Pre-existing disassembler reference` |
| After | `0x0000000140001000` | `0x0000000140003000` | `1` | `READ` | `DEFAULT` | `Pre-existing disassembler reference` |
| After | `0x0000000140001015` | `0x0000000140003000` | `0` | `WRITE` | `DEFAULT` | `Pre-existing disassembler reference` |
| After | `0x000000014000101C` | `0x0000000140003000` | `1` | `READ` | `DEFAULT` | `Pre-existing disassembler reference` |
| After | `0x0000000140001039` | `0x0000000140003000` | `0` | `WRITE` | `DEFAULT` | `Pre-existing disassembler reference` |

## Fixture Assertions

- **References before analysis:** `4`.
- **References after analysis:** `4`.
- **Analyzer-created references:** `0`.
- `DEFAULT` references present in both snapshots are disassembler output, not Reference-analyzer output.
