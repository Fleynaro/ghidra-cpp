# Subroutine References Behavioral Fixture

> Generated automatically from the MSVC x64 fixture using PyGhidra.
> All analysis options were disabled except `Subroutine References`.

## Input

- **File:** `test_subroutine_references.exe`
- **File size:** `2560` bytes

## Analysis Configuration

| Enabled boolean analysis options |
| --- |
| `Subroutine References` |

## Direct Call References

| Call site | Call target | Reference type | Fall-through | Function before analysis |
| --- | --- | --- | --- | --- |
| `0x0000000140001040` | `0x0000000140001000` | `UNCONDITIONAL_CALL` | `0x0000000140001045` | `false` |
| `0x0000000140001053` | `0x0000000140001014` | `UNCONDITIONAL_CALL` | `0x0000000140001058` | `false` |
| `0x0000000140001078` | `0x0000000140001000` | `UNCONDITIONAL_CALL` | `0x000000014000107D` | `false` |
| `0x000000014000108C` | `0x0000000140001028` | `UNCONDITIONAL_CALL` | `0x0000000140001091` | `false` |
| `0x00000001400010B0` | `0x0000000140001000` | `UNCONDITIONAL_CALL` | `0x00000001400010B5` | `false` |
| `0x00000001400010D4` | `0x000000014000103C` | `UNCONDITIONAL_CALL` | `0x00000001400010D9` | `true` |
| `0x00000001400010D9` | `0x0000000140001074` | `UNCONDITIONAL_CALL` | `0x00000001400010DE` | `true` |

## Function Entries Before Analysis

| Entry address |
| --- |
| `0x000000014000103C` |
| `0x0000000140001074` |
| `0x00000001400010AC` |
| `0x00000001400010D0` |

## Functions Created By Subroutine References

| Entry address | Body ranges |
| --- | --- |
| `0x0000000140001000` | `0x0000000140001000-0x000000014000100A` |
| `0x0000000140001014` | `0x0000000140001014-0x000000014000101E` |
| `0x0000000140001028` | `0x0000000140001028-0x0000000140001032` |

## Function Entries After Analysis

| Entry address | Body ranges |
| --- | --- |
| `0x0000000140001000` | `0x0000000140001000-0x000000014000100A` |
| `0x0000000140001014` | `0x0000000140001014-0x000000014000101E` |
| `0x0000000140001028` | `0x0000000140001028-0x0000000140001032` |
| `0x000000014000103C` | `0x000000014000103C-0x000000014000106A` |
| `0x0000000140001074` | `0x0000000140001074-0x00000001400010A4` |
| `0x00000001400010AC` | `0x00000001400010AC-0x00000001400010AC` |
| `0x00000001400010D0` | `0x00000001400010D0-0x00000001400010D0` |

## Fixture Assertions

- **Initial non-external functions:** `4`; **created entries:** `3`.
- **Maximum direct callers of one target:** `3`.
- **Known direct-call targets not newly created:** `2`.
