# Non-Returning Functions Known Behavioral Fixture

> Generated from the saved MSVC x64 PE program with PyGhidra.

## Input

- **File:** `test_non_returning_functions_known.exe`
- **File size:** `2560` bytes
- **Authoritative name:** `abort` from `PEFunctionsThatDoNotReturn`

## Analysis Configuration

- **Enabled analyzer:** `Non-Returning Functions - Known`
- **Create Analysis Bookmarks:** `true`

## Function No-Return State

| Entry offset | Name | No Return |
| --- | --- | --- |
| `0x0000000140001000` | `abort` | `true` |
| `0x0000000140001014` | `_abort` | `true` |
| `0x000000014000103C` | `entry` | `false` |

## Non-Returning Function Bookmarks

| Entry offset | Comment |
| --- | --- |
| `0x0000000140001000` | `Non-Returning Function Identified` |
| `0x0000000140001014` | `Non-Returning Function Identified` |

## Before target analysis

### Data

| Key | State |
| --- | --- |
| `(none)` | `No rows observed` |

### Functions

| Key | State |
| --- | --- |
| `0x000000014000103C` | `entry \| no-return: false` |

### Bookmarks

| Key | State |
| --- | --- |
| `(none)` | `No rows observed` |

### Options

| Key | State |
| --- | --- |
| `Create Analysis Bookmarks` | `true` |
| `Non-Returning Functions - Known` | `true` |


## After target analysis

### Data

| Key | State |
| --- | --- |
| `(none)` | `No rows observed` |

### Functions

| Key | State |
| --- | --- |
| `0x0000000140001000` | `abort \| no-return: true` |
| `0x0000000140001014` | `_abort \| no-return: true` |
| `0x000000014000103C` | `entry \| no-return: false` |

### Bookmarks

| Key | State |
| --- | --- |
| `0x0000000140001000 Non-Returning Function` | `Non-Returning Function Identified` |
| `0x0000000140001014 Non-Returning Function` | `Non-Returning Function Identified` |

### Options

| Key | State |
| --- | --- |
| `Create Analysis Bookmarks` | `true` |
| `Non-Returning Functions - Known` | `true` |


## Delta

### Data

No changes observed.

### Functions

| Change | Key | Before | After |
| --- | --- | --- | --- |
| `Added` | `0x0000000140001000` | `` | `abort \| no-return: true` |
| `Added` | `0x0000000140001014` | `` | `_abort \| no-return: true` |

### Bookmarks

| Change | Key | Before | After |
| --- | --- | --- | --- |
| `Added` | `0x0000000140001000 Non-Returning Function` | `` | `Non-Returning Function Identified` |
| `Added` | `0x0000000140001014 Non-Returning Function` | `` | `Non-Returning Function Identified` |

### Options

No changes observed.

## Fixture Assertions

- **Exact `abort` functions observed:** `1`.
- **Exact `abort` functions marked no-return:** `1`.
- **Known-name bookmarks:** `2`.
