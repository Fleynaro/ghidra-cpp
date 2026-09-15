# zlib FIDB Verification

> PASS: original Ghidra Function ID recognized real zlib code in an independently compiled executable.

## Environment

- Ghidra: `12.1.3`
- PyGhidra: `3.1.0`
- Language: `x86:LE:64:default`
- Compiler specification: `windows`
- Compiler: `` ``
- Build: `` `` ``
- FIDB: `FIDB/libraries/zlib/1.3.1/zlib-1.3.1-msvc-x86_64-release.fidb`
- FIDB records: `123`

## Expected Matches

| zlib function | Direct FidService match | Resulting Ghidra name | Score |
| --- | --- | --- | ---: |
| `adler32_z` | `1` | `adler32_z` | `251.25` |
| `compress` | `3` | `compress, compress_block, uncompress` | `897.76` |
| `crc32_z` | `1` | `crc32_z` | `282.00` |
| `uncompress` | `1` | `uncompress` | `155.79` |

## Negative Control

- Function: `non_zlib_control`
- Ghidra entry: `0x00000001400011E0`
- zlib claims: `0`
- The control retained its default `FUN_...` name and has no zlib Function ID comment or bookmark.

## Source-Language Metadata

- Declared source language: `C`.
- The installed Ghidra 12.1.3 legacy FunctionID schema has no source-language column; empty database metadata means all source languages.
