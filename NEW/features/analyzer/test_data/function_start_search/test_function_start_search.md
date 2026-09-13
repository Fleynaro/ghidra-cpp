# Function Start Search Behavioral Fixture

> Generated from the saved MSVC x64 PE program with PyGhidra.

## Input

- **File:** `test_function_start_search.exe`
- **File size:** `3072` bytes

## Analysis Configuration

| Enabled boolean analyzer |
| --- |
| `Function Start Search` |

## Candidate Export Offsets

| Symbol | Offset | Discovered function |
| --- | --- | --- |
| `function_start_candidate_a` | `0x0000000140001000` | `false` |
| `function_start_candidate_b` | `0x0000000140001080` | `false` |

## Functions Created By Pattern Search

| Offset |
| --- |

## Function Start Search Bookmarks

| Offset | Category | Comment |
| --- | --- | --- |

## Fixture Assertions

- **Candidate exports:** `2`.
- **Functions created:** `0`.
- **Pattern bookmarks:** `0`.

## Standalone Trigger Note

The real x64 prologues were present in the PE, but this verified standalone PyGhidra run scheduled no pattern-created function. The zero result is retained as the observed analyzer behavior; the pre-search phase has no matching x64 Windows pattern section.
