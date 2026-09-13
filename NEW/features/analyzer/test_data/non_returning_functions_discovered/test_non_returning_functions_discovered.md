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

## Fixture Assertions

- **Discovered target rows:** `1`.
- **Discovered target marked no-return:** `1`.
- **Analyzer bookmarks:** `1`.
