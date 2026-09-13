# Non-Returning Functions Discovered Behavioral Fixture

> Generated from the saved MSVC x64 PE program with PyGhidra.

## Input

- **File:** `test_non_returning_functions_discovered.exe`
- **File size:** `3072` bytes
- **Evidence:** three real `INT3` instructions emitted by `__debugbreak` after calls to `discovered_target`

## Analysis Configuration

- **Enabled analyzer:** `Non-Returning Functions - Discovered`
- **Function Non-return Threshold:** `3`
- **Repair Flow Damage:** `false`
- **Create Analysis Bookmarks:** `true`

## Exported Function State

| Entry offset | Name | No Return |
| --- | --- | --- |
| `0x0000000140001000` | `discovered_target` | `true` |
| `0x0000000140001014` | `discovered_caller_one` | `false` |
| `0x0000000140001038` | `discovered_caller_two` | `false` |
| `0x000000014000105C` | `discovered_caller_three` | `false` |

## Call Evidence And Flow Overrides

| Call offset | Target offset | Flow override |
| --- | --- | --- |
| `0x0000000140001018` | `0x0000000140001000` | `CALL_RETURN` |
| `0x000000014000103C` | `0x0000000140001000` | `CALL_RETURN` |
| `0x0000000140001060` | `0x0000000140001000` | `CALL_RETURN` |
| `0x0000000140001084` | `0x0000000140001014` | `NONE` |
| `0x0000000140001089` | `0x0000000140001038` | `NONE` |

## Non-Returning Function Bookmarks

| Entry offset | Comment |
| --- | --- |
| `0x0000000140001000` | `Non-Returning Function Found` |

## Before target analysis

### Data

| Key | State |
| --- | --- |
| `Call 0x0000000140001018` | `target 0x0000000140001000 \| flow override: NONE` |
| `Call 0x000000014000103C` | `target 0x0000000140001000 \| flow override: NONE` |
| `Call 0x0000000140001060` | `target 0x0000000140001000 \| flow override: NONE` |
| `Call 0x0000000140001084` | `target 0x0000000140001014 \| flow override: NONE` |
| `Call 0x0000000140001089` | `target 0x0000000140001038 \| flow override: NONE` |

### Functions

| Key | State |
| --- | --- |
| `0x0000000140001000` | `discovered_target \| no-return: false` |
| `0x0000000140001014` | `discovered_caller_one \| no-return: false` |
| `0x0000000140001038` | `discovered_caller_two \| no-return: false` |
| `0x000000014000105C` | `discovered_caller_three \| no-return: false` |

### Bookmarks

| Key | State |
| --- | --- |
| `(none)` | `No rows observed` |

### Options

| Key | State |
| --- | --- |
| `Create Analysis Bookmarks` | `true` |
| `Function Non-return Threshold` | `3` |
| `Non-Returning Functions - Discovered` | `true` |
| `Repair Flow Damage` | `false` |


## After target analysis

### Data

| Key | State |
| --- | --- |
| `Call 0x0000000140001018` | `target 0x0000000140001000 \| flow override: CALL_RETURN` |
| `Call 0x000000014000103C` | `target 0x0000000140001000 \| flow override: CALL_RETURN` |
| `Call 0x0000000140001060` | `target 0x0000000140001000 \| flow override: CALL_RETURN` |
| `Call 0x0000000140001084` | `target 0x0000000140001014 \| flow override: NONE` |
| `Call 0x0000000140001089` | `target 0x0000000140001038 \| flow override: NONE` |

### Functions

| Key | State |
| --- | --- |
| `0x0000000140001000` | `discovered_target \| no-return: true` |
| `0x0000000140001014` | `discovered_caller_one \| no-return: false` |
| `0x0000000140001038` | `discovered_caller_two \| no-return: false` |
| `0x000000014000105C` | `discovered_caller_three \| no-return: false` |

### Bookmarks

| Key | State |
| --- | --- |
| `0x0000000140001000 Non-Returning Function` | `Non-Returning Function Found` |

### Options

| Key | State |
| --- | --- |
| `Create Analysis Bookmarks` | `true` |
| `Function Non-return Threshold` | `3` |
| `Non-Returning Functions - Discovered` | `true` |
| `Repair Flow Damage` | `false` |


## Delta

### Data

| Change | Key | Before | After |
| --- | --- | --- | --- |
| `Changed` | `Call 0x0000000140001018` | `target 0x0000000140001000 \| flow override: NONE` | `target 0x0000000140001000 \| flow override: CALL_RETURN` |
| `Changed` | `Call 0x000000014000103C` | `target 0x0000000140001000 \| flow override: NONE` | `target 0x0000000140001000 \| flow override: CALL_RETURN` |
| `Changed` | `Call 0x0000000140001060` | `target 0x0000000140001000 \| flow override: NONE` | `target 0x0000000140001000 \| flow override: CALL_RETURN` |

### Functions

| Change | Key | Before | After |
| --- | --- | --- | --- |
| `Changed` | `0x0000000140001000` | `discovered_target \| no-return: false` | `discovered_target \| no-return: true` |

### Bookmarks

| Change | Key | Before | After |
| --- | --- | --- | --- |
| `Added` | `0x0000000140001000 Non-Returning Function` | `` | `Non-Returning Function Found` |

### Options

No changes observed.

## Fixture Assertions

- **Discovered target rows:** `1`.
- **Discovered target marked no-return:** `1`.
- **Analyzer bookmarks:** `1`.
