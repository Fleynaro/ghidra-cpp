# Function Start Search Behavioral Fixture

> Generated from the saved MSVC x64 PE program with PyGhidra.

## Input

- **File:** `test_function_start_search.exe`
- **File size:** `3584` bytes

## Analysis Configuration

| Enabled boolean analyzer |
| --- |
| `Function Start Search` |

## Candidate Export Offsets

| Symbol | Offset | Before function | After function | Created by target |
| --- | --- | --- | --- | --- |
| `function_start_candidate_a` | `0x0000000140001000` | `true` | `true` | `false` |
| `function_start_candidate_b` | `0x0000000140001080` | `true` | `true` | `false` |
| `function_start_positive_pattern` | `0x0000000140005003` | `false` | `true` | `true` |

## Functions Created By Pattern Search

| Offset |
| --- |
| `0x0000000140005003` |

## Function Start Search Bookmarks Before Analysis

| Offset | Category | Comment |
| --- | --- | --- |

## Function Start Search Bookmarks After Analysis

| Offset | Category | Comment |
| --- | --- | --- |
| `0x0000000140005003` | `Function Start Search` | `Match pattern 0` |

## Before target analysis

### Data

| Key | State |
| --- | --- |
| `function_start_candidate_a candidate` | `0x0000000140001000` |
| `function_start_candidate_b candidate` | `0x0000000140001080` |
| `function_start_positive_pattern candidate` | `0x0000000140005003` |

### Functions

| Key | State |
| --- | --- |
| `0x0000000140001000` | `function_start_candidate_a` |
| `0x0000000140001080` | `function_start_candidate_b` |
| `0x0000000140001100` | `entry` |

### Bookmarks

| Key | State |
| --- | --- |
| `(none)` | `No rows observed` |

### Options

| Key | State |
| --- | --- |
| `Bookmark Functions` | `true` |
| `Function Start Search` | `true` |
| `Function Start Search After Code` | `false` |
| `Function Start Search After Data` | `false` |


## After target analysis

### Data

| Key | State |
| --- | --- |
| `function_start_candidate_a candidate` | `0x0000000140001000` |
| `function_start_candidate_b candidate` | `0x0000000140001080` |
| `function_start_positive_pattern candidate` | `0x0000000140005003` |

### Functions

| Key | State |
| --- | --- |
| `0x0000000140001000` | `function_start_candidate_a` |
| `0x0000000140001080` | `function_start_candidate_b` |
| `0x0000000140001100` | `entry` |
| `0x0000000140005003` | `FUN_140005003` |

### Bookmarks

| Key | State |
| --- | --- |
| `0x0000000140005003 Function Start Search` | `Match pattern 0` |

### Options

| Key | State |
| --- | --- |
| `Bookmark Functions` | `true` |
| `Function Start Search` | `true` |
| `Function Start Search After Code` | `false` |
| `Function Start Search After Data` | `false` |


## Delta

### Data

No changes observed.

### Functions

| Change | Key | Before | After |
| --- | --- | --- | --- |
| `Added` | `0x0000000140005003` | `` | `FUN_140005003` |

### Bookmarks

| Change | Key | Before | After |
| --- | --- | --- | --- |
| `Added` | `0x0000000140005003 Function Start Search` | `` | `Match pattern 0` |

### Options

No changes observed.


## Fixture Assertions

- **Candidate exports:** `3`.
- **Functions created:** `1`.
- **Pattern bookmarks before/after:** `0` / `1`.
- **Positive candidate discovered:** `true`.
- The ordinary exported candidate remains a rejected negative control because it is already a function before the target analyzer runs.
