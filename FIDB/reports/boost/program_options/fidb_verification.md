# Boost.Program_options FIDB Verification

> PASS: original Ghidra Function ID recognized real Boost.Program_options code in an independently compiled executable.

## Environment

- Ghidra: `12.1.3`
- PyGhidra: `3.1.0`
- Language: `x86:LE:64:default`
- Compiler specification: `windows`
- Compiler: `MSVC` `19.34.31937 for x64`
- Build: `x86-64` `Release` `static`
- FIDB: `FIDB/libraries/boost/program_options/1.86.0/boost-program_options-1.86.0-msvc-x86_64-release.fidb`
- FIDB records: `731`

## Expected Matches

| Boost.Program_options function | Direct FidService match | Resulting Ghidra name | Score |
| --- | --- | --- | ---: |
| `options_description` | `9` | `FID_conflict:~action<boost::spirit::classic::chset<wchar_t>,boost::archive::xml::append_char<std::basic_string<char,std::char_traits<char>,std::allocator<char>_>_>_>, _Destroy_range<>, _Uninitialized_move<>, add, name_for_position, operator(), options_description` | `207.79` |
| `store` | `0` | `store` | `0.00` |
| `variables_map` | `7` | ``scalar_deleting_destructor', get, operator[], variables_map` | `116.39` |

## Negative Control

- Function: `non_boost_control`
- Ghidra entry: `0x0000000140007CE0`
- Boost.Program_options claims: `0`
- The control retained its default `FUN_...` name and has no Boost.Program_options Function ID comment or bookmark.

## Source-Language Metadata

- Declared source language: `C++`.
- The installed Ghidra 12.1.3 legacy FunctionID schema has no source-language column; empty database metadata means all source languages.
