# Boost.System FIDB Verification

> PASS: original Ghidra Function ID recognized real Boost.System code in an independently compiled executable.

## Environment

- Ghidra: `12.1.3`
- PyGhidra: `3.1.0`
- Language: `x86:LE:64:default`
- Compiler specification: `windows`
- Compiler: `MSVC` `19.34.31937 for x64`
- Build: `x86-64` `Release` `static`
- FIDB: `FIDB/libraries/boost/system/1.86.0/boost-system-1.86.0-msvc-x86_64-release.fidb`
- FIDB records: `0`

## Expected Matches

| Boost.System function | Direct FidService match | Resulting Ghidra name | Score |
| --- | --- | --- | ---: |

## Negative Control

- Function: `non_boost_control`
- Ghidra entry: `0x0000000140001BF0`
- Boost.System claims: `0`
- The control retained its default `FUN_...` name and has no Boost.System Function ID comment or bookmark.

## Source-Language Metadata

- Declared source language: `C++`.
- The installed Ghidra 12.1.3 legacy FunctionID schema has no source-language column; empty database metadata means all source languages.
