# ASCII Strings Behavioral Fixture

> Generated automatically with PyGhidra.

## Input

- **File:** `test_ascii_strings.exe`
- **File size:** `2560` bytes

## Analysis Configuration

- **Enabled boolean analyzers:** `ASCII Strings`
- **Minimum string length:** `5` (authoritative default)
- **Require null termination:** `true` (authoritative default)

## Fixture-Owned Strings Created

| Symbol | Start | End | Data type | Value |
| --- | --- | --- | --- | --- |
| `ascii_welcome` | `0x0000000140002000` | `0x000000014000202B` | `string` | `The quick brown fox jumps over the lazy dog` |
| `ascii_protocol` | `0x0000000140002030` | `0x0000000140002064` | `string` | `Ghidra ASCII string analysis discovers readable data` |

## Negative Controls

| Symbol | Address | Defined data | Data type | Accepted as ASCII string |
| --- | --- | --- | --- | --- |
| `ascii_short` | `0x000000014000202C` | `false` | `undefined` | `false` |
| `ascii_unterminated` | `0x0000000140002068` | `false` | `undefined` | `false` |
| `ascii_non_ascii` | `0x000000014000206C` | `false` | `undefined` | `false` |

- **Fixture-owned strings created:** `2`.
- **Negative controls retained:** `3`.
- PE metadata, import names, and unrelated initialized text are excluded by symbol ownership rather than mistaken for fixture discoveries.
