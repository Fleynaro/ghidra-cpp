# FunctionID Feature

This directory contains an autonomous C++23 implementation of Ghidra FunctionID. It reads the
original packed `.fidb` files directly and does not depend on Ghidra Java, ProgramDB, the GUI, or
the analyzer framework.

## Navigation

- [`CMakeLists.txt`](CMakeLists.txt) builds the `NewGhidra::FunctionId` static library and optionally its tests
  when `BUILD_TESTING` is enabled.
- [`build.bat`](build.bat) builds and tests only this module through the parent script; tests are enabled by default.
- [`cli/`](cli/) contains `function_id_cli`, the command-line raw-byte identifier.
- [`function_id.cppm`](function_id.cppm) defines the public hashing, database, and identification API.
- [`src/function_id.cpp`](src/function_id.cpp) implements FNV hashing, the packed-item and Ghidra
  buffer-file reader, schema-aware B-tree traversal, and scoring. Instruction hashing consumes the
  abstract metadata emitted by [`../sleigh_runtime`](../sleigh_runtime), including prototype masks,
  operand masks, fixed handles, flow metadata, and decoded bytes.
- [`tests/CMakeLists.txt`](tests/CMakeLists.txt) registers the GoogleTest target.
- [`tests/function_id_tests.cpp`](tests/function_id_tests.cpp) exercises real `TEST/fid/*.fidb`
  fixtures, x86 and ARM-backed hashing, generic skip behavior, and the source-filter API contract.
- [`data/`](data/) contains the bundled original `.fidb` databases used by the CLI when a database
  name is supplied or no database filter is given.

Run `build.bat` from this directory for the focused build and test. Use `build.bat --no-test` for a compile-only check.

## Original implementation references

The implementation follows `Ghidra/Features/FunctionID/src/main/java/ghidra/feature/fid/hash`,
`.../db`, and `.../service`, plus the minimal storage behavior from
`Ghidra/Framework/DB/src/main/java/db` and
`Ghidra/Framework/FileSystem/src/main/java/ghidra/framework/store/local`.
