# New C++23 Project

This directory contains the initial C++23 rewrite workspace developed alongside the Java Ghidra project.

## Navigation

- [`CMakeLists.txt`](CMakeLists.txt) defines the application, feature library, and tests.
- [`build.bat`](build.bat) configures, builds, and runs tests with CMake, Ninja, and vcpkg.
- [`format.bat`](format.bat) applies the repository `.clang-format` configuration to all C/C++ files under `NEW`, including the Ghidra runtime sources.
- [`tidy.bat`](tidy.bat) applies the repository `.clang-tidy` checks and fixes to C/C++ translation units under `NEW`, including the Ghidra runtime sources; headers are analyzed through their including translation units.
- [`src/main.cpp`](src/main.cpp) is the application entry point and runs the existing hello demonstration followed by a deterministic provider-backed decompiler smoke case.
- [`features/README.md`](features/README.md) documents the feature library collection.
- [`features/hello/README.md`](features/hello/README.md) documents the sample feature module.
- [`features/decompiler/README.md`](features/decompiler/README.md) documents the standalone native decompiler engine and provider boundary.

## Requirements

- MSVC with C++23 support.
- CMake 3.28 or newer (the provided script uses CMake 4.4.2 from vcpkg).
- Ninja.
- vcpkg with the dependencies declared in [`vcpkg.json`](vcpkg.json); manifest mode installs `gtest`, `pugixml`, and `zlib` for the selected triplet. The manifest pins the builtin baseline to the repository's verified local vcpkg checkout.

Run `build.bat` from this directory to create `build/new_ghidra_app.exe` and execute the tests, including `new_ghidra_app_smoke`.

The decompiler implementation source selection is being restored as an explicit, reviewed list in [`features/decompiler/CMakeLists.txt`](features/decompiler/CMakeLists.txt), rather than a `CONFIGURE_DEPENDS` glob. That file and the decompiler modules are owned by the concurrent restoration work and were not changed here. The explicit list must remain synchronized with every restored implementation unit before the full build is considered complete; the current link diagnostics show that `src/fspec.cppm` is one required entry. Update [`features/decompiler/src/README.md`](features/decompiler/src/README.md) with the final source-list change.

Run `format.bat` from this directory to format all C/C++ sources, including `features/sleigh_runtime/src/ghidra`.

Run `tidy.bat` to apply clang-tidy fixes using `build/compile_commands.json`. If build artifacts are missing, the script runs `build.bat` first. Module consumers importing C++20 modules are reported as skipped because clang-tidy cannot consume MSVC `.ifc` files.
