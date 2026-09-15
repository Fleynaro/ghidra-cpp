# Boost.Iostreams FIDB Verification

> PASS: original Ghidra Function ID recognized real Boost.Iostreams code in an independently compiled executable.

## Environment

- Ghidra: `12.1.3`
- PyGhidra: `3.1.0`
- Language: `x86:LE:64:default`
- Compiler specification: `windows`
- Compiler: `MSVC` `19.34.31937 for x64`
- Build: `x86-64` `Release` `static`
- FIDB: `FIDB/libraries/boost/iostreams/1.86.0/boost-iostreams-1.86.0-msvc-x86_64-release.fidb`
- FIDB records: `171`

## Expected Matches

| Boost.Iostreams function | Direct FidService match | Resulting Ghidra name | Score |
| --- | --- | --- | ---: |
| `file_descriptor` | `4` | `FID_conflict:~action<boost::spirit::classic::chset<wchar_t>,boost::archive::xml::append_char<std::basic_string<char,std::char_traits<char>,std::allocator<char>_>_>_>` | `30.04` |
| `mapped_file_impl` | `13` | `checked_delete<boost::iostreams::detail::mapped_file_impl>, cleanup_and_throw, close, dispose, map_file, mapped_file_impl, open, open_file, shared_count<boost::iostreams::detail::mapped_file_impl>, try_map_file, ~mapped_file_impl` | `354.15` |
| `mapped_file_source` | `2` | `init, open_impl` | `137.79` |

## Negative Control

- Function: `non_boost_control`
- Ghidra entry: `0x0000000140004730`
- Boost.Iostreams claims: `0`
- The control retained its default `FUN_...` name and has no Boost.Iostreams Function ID comment or bookmark.

## Source-Language Metadata

- Declared source language: `C++`.
- The installed Ghidra 12.1.3 legacy FunctionID schema has no source-language column; empty database metadata means all source languages.
