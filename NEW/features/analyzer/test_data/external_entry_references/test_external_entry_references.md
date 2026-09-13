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

## Fixture Assertions

- **External entries:** `4`.
- **Functions before analysis:** `1`.
- **Functions created by the analyzer:** `3`.
