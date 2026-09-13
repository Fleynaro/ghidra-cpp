# Shared Return Calls Behavioral Fixture

> Generated from actual Ghidra function and flow-state snapshots. Destination functions were seeded only because the Java contract requires existing functions.

## Input

- **Executable:** `test_shared_return_calls.exe`
- **Executable bytes:** `2560`

## Analysis Configuration

| Enabled boolean option |
| --- |
| `Shared Return Calls` |

## Before target analysis
### Functions visible before target analysis

| Entry | Name | Provenance |
| --- | --- | --- |
| `0x0000000140001000` | `FUN_140001000` | Disassembler/fixture prerequisite |
| `0x0000000140001030` | `entry` | Disassembler/fixture prerequisite |
### Jump flow visible before target analysis

| Source | Target | Flow override | Provenance |
| --- | --- | --- | --- |
| `0x0000000140001020` | `0x0000000140001000` | `NONE` | Pre-existing flow state |

## After target analysis
### Functions visible after target analysis

| Entry | Name | Provenance |
| --- | --- | --- |
| `0x0000000140001000` | `FUN_140001000` | Post-target function state |
| `0x0000000140001030` | `entry` | Post-target function state |
### Jump flow visible after target analysis

| Source | Target | Flow override | Provenance |
| --- | --- | --- | --- |
| `0x0000000140001020` | `0x0000000140001000` | `CALL_RETURN` | Post-target flow state |

## Delta

Seeded destination functions are prerequisites and are classified as pre-existing. Only the exact function/flow changes below are attributed to Shared Return Calls.

### Function delta

| Change | Entry | Before name | After name |
| --- | --- | --- | --- |
| _(none)_ | | | |

### Flow delta

| Change | Source | Target | Before override | After override |
| --- | --- | --- | --- | --- |
| Changed | `0x0000000140001020` | `0x0000000140001000` | `NONE` | `CALL_RETURN` |

### Delta conclusion

- **Functions added:** `0`; removed: `0`; changed: `0`.
- **Flow rows added:** `0`; removed: `0`; changed: `1`.
- **Flow rows changed to `CALL_RETURN`:** `1`.
