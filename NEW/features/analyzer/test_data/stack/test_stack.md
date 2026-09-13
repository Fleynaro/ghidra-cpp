# Stack Behavioral Fixture

> Generated from actual Ghidra stack-variable and reference snapshots around the target analyzer.

## Input

- **Executable:** `test_stack.exe`
- **Executable bytes:** `2560`

## Analysis Configuration

| Enabled boolean option |
| --- |
| `Stack` |
| `Stack.Create Local Variables` |
| `Subroutine References` |

## Before target analysis
### Stack artifacts visible before target analysis

#### Functions

| Entry | Name | Provenance |
| --- | --- | --- |
| `0x0000000140001038` | `entry` | Disassembly/fixture prerequisite |

#### Stack variables

| Function entry | Function | Name | Storage | Data type | Provenance |
| --- | --- | --- | --- | --- | --- |
| _(none)_ | | | | | |

#### Stack references

| Function entry | Function | Source | Target | Provenance |
| --- | --- | --- | --- | --- |
| _(none)_ | | | | |

## After target analysis
### Stack artifacts visible after target analysis

#### Functions

| Entry | Name | Provenance |
| --- | --- | --- |
| `0x0000000140001000` | `FUN_140001000` | Post-target stack state |
| `0x0000000140001038` | `entry` | Post-target stack state |

#### Stack variables

| Function entry | Function | Name | Storage | Data type | Provenance |
| --- | --- | --- | --- | --- | --- |
| `0x0000000140001000` | `FUN_140001000` | `local_res10` | `Stack[0x10]:4` | `undefined4` | Post-target stack state |
| `0x0000000140001000` | `FUN_140001000` | `local_res8` | `Stack[0x8]:4` | `undefined4` | Post-target stack state |

#### Stack references

| Function entry | Function | Source | Target | Provenance |
| --- | --- | --- | --- | --- |
| `0x0000000140001000` | `FUN_140001000` | `0x0000000140001003` | `Stack[0x10]` | Post-target stack state |
| `0x0000000140001000` | `FUN_140001000` | `0x0000000140001007` | `Stack[0x10]` | Post-target stack state |
| `0x0000000140001000` | `FUN_140001000` | `0x000000014000100E` | `Stack[0x8]` | Post-target stack state |
| `0x0000000140001000` | `FUN_140001000` | `0x0000000140001018` | `Stack[0x8]` | Post-target stack state |
| `0x0000000140001000` | `FUN_140001000` | `0x0000000140001024` | `Stack[0x8]` | Post-target stack state |
| `0x0000000140001000` | `FUN_140001000` | `0x0000000140001028` | `Stack[0x10]` | Post-target stack state |

## Delta

The target boundary is `project.analyze(program)`. Existing functions, variables, and references are retained as before-state artifacts; only these exact keyed changes are attributed to Stack.

### Function delta

| Change | Entry | Name |
| --- | --- | --- |
| Added | `0x0000000140001000` | `FUN_140001000` |
### Stack variable delta

| Change | Function entry | Function | Name | Storage | Data type |
| --- | --- | --- | --- | --- | --- |
| Added | `0x0000000140001000` | `FUN_140001000` | `local_res8` | `Stack[0x8]:4` | `undefined4` |
### Stack reference delta

| Change | Function entry | Function | Source | Target |
| --- | --- | --- | --- | --- |
| Added | `0x0000000140001000` | `FUN_140001000` | `0x0000000140001003` | `Stack[0x10]` |
| Added | `0x0000000140001000` | `FUN_140001000` | `0x0000000140001007` | `Stack[0x10]` |
| Added | `0x0000000140001000` | `FUN_140001000` | `0x000000014000100E` | `Stack[0x8]` |
| Added | `0x0000000140001000` | `FUN_140001000` | `0x0000000140001018` | `Stack[0x8]` |
| Added | `0x0000000140001000` | `FUN_140001000` | `0x0000000140001024` | `Stack[0x8]` |
| Added | `0x0000000140001000` | `FUN_140001000` | `0x0000000140001028` | `Stack[0x10]` |

### Delta conclusion

- **Exact stack delta rows:** `8`.
- Stack references and variables are reported from Ghidra's listing and function model; source-level local names are not assumed.
