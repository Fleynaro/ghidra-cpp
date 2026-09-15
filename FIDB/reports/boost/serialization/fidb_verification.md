# Boost.Serialization FIDB Verification

> PASS: original Ghidra Function ID recognized real Boost.Serialization code in an independently compiled executable.

## Environment

- Ghidra: `12.1.3`
- PyGhidra: `3.1.0`
- Language: `x86:LE:64:default`
- Compiler specification: `windows`
- Compiler: `MSVC` `19.34.31937 for x64`
- Build: `x86-64` `Release` `static`
- FIDB: `FIDB/libraries/boost/serialization/1.86.0/boost-serialization-1.86.0-msvc-x86_64-release.fidb`
- FIDB records: `667`

## Expected Matches

| Boost.Serialization function | Direct FidService match | Resulting Ghidra name | Score |
| --- | --- | --- | ---: |
| `load` | `14` | `load, load_object, load_override, load_preamble, vload` | `180.86` |
| `save` | `3` | `save, ~basic_istream_locale_saver<char,std::char_traits<char>_>, ~basic_ostream_locale_saver<char,std::char_traits<char>_>` | `246.08` |
| `text_oarchive` | `11` | `FID_conflict:`scalar_deleting_destructor', `scalar_deleting_destructor', get_instance, newtoken, operator<<<unsigned___int64>, save, text_oarchive_impl<boost::archive::text_oarchive>, ~text_oarchive_impl<boost::archive::text_oarchive>` | `246.08` |

## Negative Control

- Function: `non_boost_control`
- Ghidra entry: `0x0000000140002400`
- Boost.Serialization claims: `0`
- The control retained its default `FUN_...` name and has no Boost.Serialization Function ID comment or bookmark.

## Source-Language Metadata

- Declared source language: `C++`.
- The installed Ghidra 12.1.3 legacy FunctionID schema has no source-language column; empty database metadata means all source languages.
