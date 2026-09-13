# Stack Behavioral Fixture

> Generated from actual Ghidra stack-variable and reference state.

## Input

- **Executable:** `test_stack.exe`
- **Executable bytes:** `2560`

## Analysis Configuration

| Enabled boolean option |
| --- |
| `Stack` |
| `Stack.Create Local Variables` |
| `Subroutine References` |

## Functions

| Entry | Name |
| --- | --- |
| `0x0000000140001000` | `FUN_140001000` |
| `0x0000000140001038` | `entry` |

## Stack Variables

| Function | Name | Storage | Data type |
| --- | --- | --- | --- |
| `FUN_140001000` | `local_res10` | `Stack[0x10]:4` | `undefined4` |
| `FUN_140001000` | `local_res8` | `Stack[0x8]:4` | `undefined4` |

## Stack References

| Function | Source | Target |
| --- | --- | --- |
| `FUN_140001000` | `0x0000000140001003` | `Stack[0x10]` |
| `FUN_140001000` | `0x0000000140001007` | `Stack[0x10]` |
| `FUN_140001000` | `0x000000014000100E` | `Stack[0x8]` |
| `FUN_140001000` | `0x0000000140001018` | `Stack[0x8]` |
| `FUN_140001000` | `0x0000000140001024` | `Stack[0x8]` |
| `FUN_140001000` | `0x0000000140001028` | `Stack[0x10]` |
