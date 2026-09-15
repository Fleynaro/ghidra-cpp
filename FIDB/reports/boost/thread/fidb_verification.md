# Boost.Thread FIDB Verification

> PASS: original Ghidra Function ID recognized real Boost.Thread code in an independently compiled executable.

## Environment

- Ghidra: `12.1.3`
- PyGhidra: `3.1.0`
- Language: `x86:LE:64:default`
- Compiler specification: `windows`
- Compiler: `MSVC` `19.34.31937 for x64`
- Build: `x86-64` `Release` `static`
- FIDB: `FIDB/libraries/boost/thread/1.86.0/boost-thread-1.86.0-msvc-x86_64-release.fidb`
- FIDB records: `210`

## Expected Matches

| Boost.Thread function | Direct FidService match | Resulting Ghidra name | Score |
| --- | --- | --- | ---: |
| `interrupt` | `1` | `interruptible_wait` | `364.00` |
| `join` | `1` | `join_noexcept` | `356.07` |
| `start_thread` | `1` | `start_thread_noexcept` | `47.73` |

## Negative Control

- Function: `non_boost_control`
- Ghidra entry: `0x00000001400044F0`
- Boost.Thread claims: `0`
- The control retained its default `FUN_...` name and has no Boost.Thread Function ID comment or bookmark.

## Source-Language Metadata

- Declared source language: `C++`.
- The installed Ghidra 12.1.3 legacy FunctionID schema has no source-language column; empty database metadata means all source languages.
