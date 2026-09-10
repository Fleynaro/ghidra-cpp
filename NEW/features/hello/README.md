# Hello Feature

The `hello` feature is a small C++23 module used to validate the project layout and toolchain.

## Navigation

- [`hello.cppm`](hello.cppm) exports `hello::build_message`.
- [`CMakeLists.txt`](CMakeLists.txt) declares the feature library and its tests.
- [`tests/README.md`](tests/README.md) documents the Google Test suite.
- [`../../CMakeLists.txt`](../../CMakeLists.txt) registers the parent project.
- [`../../build.bat`](../../build.bat) builds and tests the module through the parent project.

## Behavior

`build_message` validates input through a C++23 `consteval` helper and returns a diagnostic message for invalid input.
