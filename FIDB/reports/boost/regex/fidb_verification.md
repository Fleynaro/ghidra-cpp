# Boost.Regex FIDB Verification

> PASS: original Ghidra Function ID recognized real Boost.Regex code in an independently compiled executable.

## Environment

- Ghidra: `12.1.3`
- PyGhidra: `3.1.0`
- Language: `x86:LE:64:default`
- Compiler specification: `windows`
- Compiler: `MSVC` `19.34.31937 for x64`
- Build: `x86-64` `Release` `static`
- FIDB: `FIDB/libraries/boost/regex/1.86.0/boost-regex-1.86.0-msvc-x86_64-release.fidb`
- FIDB records: `520`

## Expected Matches

| Boost.Regex function | Direct FidService match | Resulting Ghidra name | Score |
| --- | --- | --- | ---: |
| `match_all_states` | `2` | `match_all_states` | `122.00` |
| `parse` | `4` | `parse, parse_all, parse_match_any, unwind_alts` | `637.09` |
| `perl_matcher` | `31` | `extend_stack, match_all_states, match_alt, match_commit, match_rep, match_then, match_toggle_case, match_within_word, push_alt, push_matched_paren, push_recursion, skip_until_paren, unwind, unwind_commit, unwind_extra_block, unwind_recursion_pop, unwind_then, ~perl_matcher<>` | `451.72` |

## Negative Control

- Function: `non_boost_control`
- Ghidra entry: `0x0000000140014000`
- Boost.Regex claims: `0`
- The control retained its default `FUN_...` name and has no Boost.Regex Function ID comment or bookmark.

## Source-Language Metadata

- Declared source language: `C++`.
- The installed Ghidra 12.1.3 legacy FunctionID schema has no source-language column; empty database metadata means all source languages.
