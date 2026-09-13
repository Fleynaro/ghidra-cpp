# External Entry References Behavioral Fixture

> Generated from a saved MSVC x64 PE program with PyGhidra.

## Input

- **File:** `test_external_entry_references.exe`
- **File size:** `2560` bytes

## Analysis Configuration

- **Enabled analyzer:** `External Entry References`

## PE External Entry References

| Entry offset | Function after analysis |
| --- | --- |
| `0x0000000140001000` | `true` |
| `0x0000000140001014` | `true` |
| `0x0000000140001028` | `true` |
| `0x0000000140001054` | `true` |

## Function Entries Created

| Entry offset |
| --- |
| `0x0000000140001000` |
| `0x0000000140001014` |
| `0x0000000140001028` |

## Before target analysis

### Data

| Key | State |
| --- | --- |
| `0x0000000140001000` | `External entry point` |
| `0x0000000140001014` | `External entry point` |
| `0x0000000140001028` | `External entry point` |
| `0x0000000140001054` | `External entry point` |

### Functions

| Key | State |
| --- | --- |
| `0x0000000140001054` | `entry` |

### Bookmarks

| Key | State |
| --- | --- |
| `(none)` | `No rows observed` |

### Options

| Key | State |
| --- | --- |
| `External Entry References` | `true` |


## After target analysis

### Data

| Key | State |
| --- | --- |
| `0x0000000140001000` | `External entry point` |
| `0x0000000140001014` | `External entry point` |
| `0x0000000140001028` | `External entry point` |
| `0x0000000140001054` | `External entry point` |

### Functions

| Key | State |
| --- | --- |
| `0x0000000140001000` | `external_entry_alpha` |
| `0x0000000140001014` | `external_entry_beta` |
| `0x0000000140001028` | `external_entry_gamma` |
| `0x0000000140001054` | `entry` |

### Bookmarks

| Key | State |
| --- | --- |
| `(none)` | `No rows observed` |

### Options

| Key | State |
| --- | --- |
| `External Entry References` | `true` |


## Delta

### Data

No changes observed.

### Functions

| Change | Key | Before | After |
| --- | --- | --- | --- |
| `Added` | `0x0000000140001000` | `` | `external_entry_alpha` |
| `Added` | `0x0000000140001014` | `` | `external_entry_beta` |
| `Added` | `0x0000000140001028` | `` | `external_entry_gamma` |

### Bookmarks

No changes observed.

### Options

No changes observed.

## Fixture Assertions

- **External entries:** `4`.
- **Functions before analysis:** `1`.
- **Functions created by the analyzer:** `3`.
