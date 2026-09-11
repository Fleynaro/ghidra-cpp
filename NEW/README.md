# New C++23 Project

This directory contains the initial C++23 rewrite workspace developed alongside the Java Ghidra project.

## Navigation

- [`CMakeLists.txt`](CMakeLists.txt) defines the application, feature library, and tests.
- [`build.bat`](build.bat) configures, builds, and runs tests with CMake, Ninja, and vcpkg.
- [`format.bat`](format.bat) applies the repository `.clang-format` configuration to all C/C++ files under `NEW`, including the Ghidra runtime sources.
- [`tidy.bat`](tidy.bat) applies the repository `.clang-tidy` checks and fixes to C/C++ translation units under `NEW`, including the Ghidra runtime sources; headers are analyzed through their including translation units.
- [`src/main.cpp`](src/main.cpp) is the application entry point.
- [`features/README.md`](features/README.md) documents the feature library collection.
- [`features/hello/README.md`](features/hello/README.md) documents the sample feature module.
- [`features/decompiler/README.md`](features/decompiler/README.md) documents the standalone native decompiler engine and provider boundary.

## Requirements

- MSVC with C++23 support.
- CMake 3.28 or newer (the provided script uses CMake 4.4.2 from vcpkg).
- Ninja.
- vcpkg with `gtest:x64-windows` installed.

Run `build.bat` from this directory to create `build/new_ghidra_app.exe` and execute the tests.

Run `format.bat` from this directory to format all C/C++ sources, including `features/sleigh_runtime/src/ghidra`.

Run `tidy.bat` to apply clang-tidy fixes using `build/compile_commands.json`. If build artifacts are missing, the script runs `build.bat` first. Module consumers importing C++20 modules are reported as skipped because clang-tidy cannot consume MSVC `.ifc` files.
