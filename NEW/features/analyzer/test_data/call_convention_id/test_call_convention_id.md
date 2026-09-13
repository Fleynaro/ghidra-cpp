# Call Convention ID Behavioral Fixture

> Generated automatically with PyGhidra.

## Input

- **File:** `test_call_convention_id.exe`
- **File size:** `2560` bytes

## Analysis Configuration

- **Enabled boolean analyzers:** `Call Convention ID`
- **Target eligibility:** `unknown convention, three defined DWord parameters, non-custom storage`

## Convention Observation

| Stage | Function | Calling convention | Parameter count | Signature source |
| --- | --- | --- | --- | --- |
| Before analysis | `convention_target` | `unknown` | `3` | `USER_DEFINED` |
| After analysis | `convention_target` | `__fastcall` | `3` | `USER_DEFINED` |

- **Convention changed:** `true`.
