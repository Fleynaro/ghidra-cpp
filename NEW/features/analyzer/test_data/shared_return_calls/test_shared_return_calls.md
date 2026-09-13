# Shared Return Calls Behavioral Fixture

> Generated from actual Ghidra flow state. Destination functions were seeded only because the Java contract requires existing functions.

## Input

- **Executable:** `test_shared_return_calls.exe`
- **Executable bytes:** `2560`

## Analysis Configuration

| Enabled boolean option |
| --- |
| `Shared Return Calls` |

## Jump Flow Before and After

| Source | Target | Before | After |
| --- | --- | --- | --- |
| `0x0000000140001020` | `0x0000000140001000` | `NONE` | `CALL_RETURN` |

## Observed Analyzer Effect

- **Jump rows changed to CALL_RETURN:** `1`
