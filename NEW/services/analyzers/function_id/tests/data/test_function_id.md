# Function ID Behavioral Fixture

> Generated from a real packed FID database and the saved target program with PyGhidra.

## Input

- **Target:** `test_function_id.exe`
- **Target file size:** `2560` bytes
- **FID artifact version:** `Ghidra 12.1.3`
- **FID language ID:** `x86:LE:64:default`
- **Direct FidService query results:** `2` functions, `2` matches

## Analysis Configuration

| Enabled boolean option |
| --- |
| `Function ID` |
| `Function ID.Always Apply FID Labels` |
| `Function ID.Create Analysis Bookmarks` |

## Target Functions After Function ID

| Entry offset | Name | Plate comment |
| --- | --- | --- |
| `0x0000000140001000` | `known_library_function` | `Library Function - Single Match<br> known_library_function<br><br>Library: Analyzer Fixture Library 1.0 x64` |
| `0x00000001400010E4` | `fixture_entry` | `Library Function - Single Match<br> fixture_entry<br><br>Library: Analyzer Fixture Library 1.0 x64` |

## Function ID Bookmarks

| Entry offset | Comment |
| --- | --- |
| `0x0000000140001000` | `Library Function - Single Match,  known_library_function` |
| `0x00000001400010E4` | `Library Function - Single Match,  fixture_entry` |

## Before target analysis

### Data

| Key | State |
| --- | --- |
| `(none)` | `No rows observed` |

### Functions

| Key | State |
| --- | --- |
| `0x0000000140001000` | `known_library_function \| plate comment: ` |
| `0x00000001400010E4` | `fixture_entry \| plate comment: ` |

### Bookmarks

| Key | State |
| --- | --- |
| `(none)` | `No rows observed` |

### Options

| Key | State |
| --- | --- |
| `Always Apply FID Labels` | `true` |
| `Create Analysis Bookmarks` | `true` |
| `Function ID` | `true` |


## After target analysis

### Data

| Key | State |
| --- | --- |
| `(none)` | `No rows observed` |

### Functions

| Key | State |
| --- | --- |
| `0x0000000140001000` | `known_library_function \| plate comment: Library Function - Single Match<br> known_library_function<br><br>Library: Analyzer Fixture Library 1.0 x64` |
| `0x00000001400010E4` | `fixture_entry \| plate comment: Library Function - Single Match<br> fixture_entry<br><br>Library: Analyzer Fixture Library 1.0 x64` |

### Bookmarks

| Key | State |
| --- | --- |
| `0x0000000140001000 Function ID Analyzer` | `Library Function - Single Match,  known_library_function` |
| `0x00000001400010E4 Function ID Analyzer` | `Library Function - Single Match,  fixture_entry` |

### Options

| Key | State |
| --- | --- |
| `Always Apply FID Labels` | `true` |
| `Create Analysis Bookmarks` | `true` |
| `Function ID` | `true` |


## Delta

### Data

No changes observed.

### Functions

| Change | Key | Before | After |
| --- | --- | --- | --- |
| `Changed` | `0x0000000140001000` | `known_library_function \| plate comment: ` | `known_library_function \| plate comment: Library Function - Single Match<br> known_library_function<br><br>Library: Analyzer Fixture Library 1.0 x64` |
| `Changed` | `0x00000001400010E4` | `fixture_entry \| plate comment: ` | `fixture_entry \| plate comment: Library Function - Single Match<br> fixture_entry<br><br>Library: Analyzer Fixture Library 1.0 x64` |

### Bookmarks

| Change | Key | Before | After |
| --- | --- | --- | --- |
| `Added` | `0x0000000140001000 Function ID Analyzer` | `` | `Library Function - Single Match,  known_library_function` |
| `Added` | `0x00000001400010E4 Function ID Analyzer` | `` | `Library Function - Single Match,  fixture_entry` |

### Options

No changes observed.

## Fixture Assertions

- **Functions added to the generated FID library:** `2`.
- **Target functions observed:** `2`.
- **Function ID bookmarks:** `2`.
