# Boost.Filesystem FIDB Verification

> PASS: original Ghidra Function ID recognized real Boost.Filesystem code in an independently compiled executable.

## Environment

- Ghidra: `12.1.3`
- PyGhidra: `3.1.0`
- Language: `x86:LE:64:default`
- Compiler specification: `windows`
- Compiler: `MSVC` `19.34.31937 for x64`
- Build: `x86-64` `Release` `static`
- FIDB: `FIDB/libraries/boost/filesystem/1.86.0/boost-filesystem-1.86.0-msvc-x86_64-release.fidb`
- FIDB records: `290`

## Expected Matches

| Boost.Filesystem function | Direct FidService match | Resulting Ghidra name | Score |
| --- | --- | --- | ---: |
| `recursive_directory_iterator_increment` | `1` | `recursive_directory_iterator_increment` | `311.25` |
| `remove_all` | `2` | `remove_all, remove_all_nt5_impl` | `1412.09` |
| `status` | `9` | `dir_itr_create, dir_itr_increment, process_status_failure, set_file_statuses, status, status_by_handle, status_impl, symlink_status, symlink_status_impl` | `964.00` |

## Negative Control

- Function: `non_boost_control`
- Ghidra entry: `0x0000000140002C10`
- Boost.Filesystem claims: `0`
- The control retained its default `FUN_...` name and has no Boost.Filesystem Function ID comment or bookmark.

## Source-Language Metadata

- Declared source language: `C++`.
- The installed Ghidra 12.1.3 legacy FunctionID schema has no source-language column; empty database metadata means all source languages.
