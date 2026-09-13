# Aggressive Instruction Finder Behavioral Fixture

> Generated automatically with PyGhidra.

## Input

- **File:** `test_aggressive_instruction_finder.exe`
- **File size:** `4096` bytes

## Analysis Configuration

- **Enabled boolean analyzers:** `Aggressive Instruction Finder, Aggressive Instruction Finder.Create Analysis Bookmarks`
- **Create Analysis Bookmarks:** `true`
- **Minimum function count required by Java:** `20`

## Before target analysis

| Kind | Address | Value | Comment |
| --- | --- | --- | --- |
| `function` | `140001000` | `seed_00` | `` |
| `function` | `140001020` | `seed_01` | `` |
| `function` | `140001040` | `seed_02` | `` |
| `function` | `140001060` | `seed_03` | `` |
| `function` | `140001080` | `seed_04` | `` |
| `function` | `1400010a0` | `seed_05` | `` |
| `function` | `1400010c0` | `seed_06` | `` |
| `function` | `1400010e0` | `seed_07` | `` |
| `function` | `140001100` | `seed_08` | `` |
| `function` | `140001120` | `seed_09` | `` |
| `function` | `140001140` | `seed_10` | `` |
| `function` | `140001160` | `seed_11` | `` |
| `function` | `140001180` | `seed_12` | `` |
| `function` | `1400011a0` | `seed_13` | `` |
| `function` | `1400011c0` | `seed_14` | `` |
| `function` | `1400011e0` | `seed_15` | `` |
| `function` | `140001200` | `seed_16` | `` |
| `function` | `140001220` | `seed_17` | `` |
| `function` | `140001240` | `seed_18` | `` |
| `function` | `140001260` | `seed_19` | `` |
| `function` | `140001280` | `seed_20` | `` |
| `function` | `1400012a0` | `seed_21` | `` |
| `function` | `1400012c0` | `seed_22` | `` |
| `function` | `1400012e0` | `seed_23` | `` |
| `function` | `140001324` | `aggressive_instruction_finder_entry` | `` |

## After target analysis

| Kind | Address | Value | Comment |
| --- | --- | --- | --- |
| `bookmark` | `140001300` | `Aggressive Instruction Finder` | `Found code` |
| `function` | `140001000` | `seed_00` | `` |
| `function` | `140001020` | `seed_01` | `` |
| `function` | `140001040` | `seed_02` | `` |
| `function` | `140001060` | `seed_03` | `` |
| `function` | `140001080` | `seed_04` | `` |
| `function` | `1400010a0` | `seed_05` | `` |
| `function` | `1400010c0` | `seed_06` | `` |
| `function` | `1400010e0` | `seed_07` | `` |
| `function` | `140001100` | `seed_08` | `` |
| `function` | `140001120` | `seed_09` | `` |
| `function` | `140001140` | `seed_10` | `` |
| `function` | `140001160` | `seed_11` | `` |
| `function` | `140001180` | `seed_12` | `` |
| `function` | `1400011a0` | `seed_13` | `` |
| `function` | `1400011c0` | `seed_14` | `` |
| `function` | `1400011e0` | `seed_15` | `` |
| `function` | `140001200` | `seed_16` | `` |
| `function` | `140001220` | `seed_17` | `` |
| `function` | `140001240` | `seed_18` | `` |
| `function` | `140001260` | `seed_19` | `` |
| `function` | `140001280` | `seed_20` | `` |
| `function` | `1400012a0` | `seed_21` | `` |
| `function` | `1400012c0` | `seed_22` | `` |
| `function` | `1400012e0` | `seed_23` | `` |
| `function` | `140001324` | `aggressive_instruction_finder_entry` | `` |

## Delta

**Added rows**

- `bookmark` at `140001300`: `Aggressive Instruction Finder; Found code`

**Removed rows**

- None

**Changed rows**

- None

## Discovery Observation

- **Functions before analysis:** `25`
- **Functions after analysis:** `25`

| Bookmark address | Category | Comment |
| --- | --- | --- |
| `140001300` | `Aggressive Instruction Finder` | `Found code` |

- **Bookmarks before target analysis:** `0`.
- **Aggressive-discovery bookmarks:** `1`.
