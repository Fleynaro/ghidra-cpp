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
| `0x0000000140001028` | `entry` | `false` |

## Non-Returning Function Bookmarks

| Entry offset | Comment |
| --- | --- |
| `0x0000000140001000` | `Non-Returning Function Identified` |

## Fixture Assertions

- **Exact `abort` functions observed:** `1`.
- **Exact `abort` functions marked no-return:** `1`.
- **Known-name bookmarks:** `1`.
