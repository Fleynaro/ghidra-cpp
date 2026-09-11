# PE Loader

`pe_loader` is an autonomous C++23 module that parses Portable Executable bytes into an owned,
section-aware `LoadedPeImage`. It is the loader boundary for the future pipeline
`bytes -> PE image -> Sleigh decoder`; it does not disassemble, discover functions, build graphs, or
decompile.

## Navigation

- [`pe_loader.cppm`](pe_loader.cppm) exports the public value types, error model, address API, and loader.
- [`pe_loader.cpp`](pe_loader.cpp) implements checked PE32/PE32+ parsing and image mapping.
- [`CMakeLists.txt`](CMakeLists.txt) builds the module library and its test target.
- [`tests/README.md`](tests/README.md) describes fixture and malformed-input coverage.
- [`tests/pe_loader_tests.cpp`](tests/pe_loader_tests.cpp) contains GoogleTest integration and edge-case tests.
- [`../CMakeLists.txt`](../CMakeLists.txt) registers this feature with the feature collection.
- [`../../build.bat`](../../build.bat) configures, builds, and runs all CTest targets with MSVC, Ninja, and vcpkg.

## Contract

`PeLoader::load` copies its input span and returns `std::expected<LoadedPeImage, ParseError>`.
`PeLoader::load_file` provides the same contract for a filesystem path. The image exposes DOS, Rich,
COFF, optional-header, section, directory, import/export, relocation, debug, exception, TLS, load-config,
resource, certificate, bound/delay-import, architecture/global-pointer, CLR, and COFF-symbol metadata.
SectionAlignment-rounded virtual extents are retained for safe range and overlap validation, while
exposed memory regions preserve the actual Ghidra loader block sizes from the integration fixture. It
also owns a zero-filled preferred image view and provides checked RVA, VA, file-offset, memory-read, and
resource-payload operations.

Strict mode (the default) returns the first malformed directory as a `ParseError`. With
`LoadOptions::strict == false`, the loader retains the valid image and records every tolerated directory
failure in `parse_diagnostics()`; callers must inspect `parse_status()` or `is_partial()` before treating
directory metadata as complete.

The implementation is derived from the Ghidra PE loader and format classes listed in the source comments,
but has no dependency on Ghidra Java classes, Program/DB types, AddressFactory, Memory, SymbolTable, or
analysis infrastructure.
