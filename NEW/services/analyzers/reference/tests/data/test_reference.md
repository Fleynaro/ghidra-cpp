# Reference Behavioral Fixture

> Generated from actual Ghidra reference-manager snapshots around the target analyzer.

## Input

- **Executable:** `test_reference.exe`
- **Executable bytes:** `3072`

## Analysis Configuration

| Enabled boolean option |
| --- |
| `Reference` |

## Before target analysis
### References visible before target analysis

| Source | Target | Operand | Type | Ghidra source | Provenance |
| --- | --- | --- | --- | --- | --- |
| `0x0000000140001000` | `0x0000000140003000` | `1` | `READ` | `DEFAULT` | Pre-existing disassembler reference |
| `0x0000000140001015` | `0x0000000140003000` | `0` | `WRITE` | `DEFAULT` | Pre-existing disassembler reference |
| `0x000000014000101C` | `0x0000000140003000` | `1` | `READ` | `DEFAULT` | Pre-existing disassembler reference |
| `0x0000000140001069` | `0x0000000140003000` | `0` | `WRITE` | `DEFAULT` | Pre-existing disassembler reference |
| `0x0000000140001070` | `0x0000000140003000` | `1` | `READ` | `DEFAULT` | Pre-existing disassembler reference |
| `0x000000014000107C` | `0x0000000140003000` | `1` | `READ` | `DEFAULT` | Pre-existing disassembler reference |
| `0x0000000140001086` | `0x0000000140003000` | `0` | `WRITE` | `DEFAULT` | Pre-existing disassembler reference |

## After target analysis
### References visible after target analysis

| Source | Target | Operand | Type | Ghidra source | Provenance |
| --- | --- | --- | --- | --- | --- |
| `0x0000000140001000` | `0x0000000140003000` | `1` | `READ` | `DEFAULT` | Post-target reference state |
| `0x0000000140001015` | `0x0000000140003000` | `0` | `WRITE` | `DEFAULT` | Post-target reference state |
| `0x000000014000101C` | `0x0000000140003000` | `1` | `READ` | `DEFAULT` | Post-target reference state |
| `0x0000000140001069` | `0x0000000140003000` | `0` | `WRITE` | `DEFAULT` | Post-target reference state |
| `0x0000000140001070` | `0x0000000140003000` | `1` | `READ` | `DEFAULT` | Post-target reference state |
| `0x000000014000107C` | `0x0000000140003000` | `1` | `READ` | `DEFAULT` | Post-target reference state |
| `0x0000000140001086` | `0x0000000140003000` | `0` | `WRITE` | `DEFAULT` | Post-target reference state |

## Delta

The delta is keyed by source, target, operand, and reference type; a changed Ghidra source is reported as changed rather than silently merged.

| Change | Source | Target | Operand | Type | Before source | After source |
| --- | --- | --- | --- | --- | --- | --- |
| _(none)_ | | | | | | |

### Delta conclusion

- **References before target analysis:** `7`.
- **References after target analysis:** `7`.
- **References added:** `0`; removed: `0`; changed: `0`.
- `DEFAULT` rows present in both snapshots are loader/disassembler artifacts, not Reference-analyzer deltas.
