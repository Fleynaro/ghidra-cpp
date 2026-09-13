# New C++23 Project

This directory contains the initial C++23 rewrite workspace developed alongside the Java Ghidra project.

## Navigation

- [`CMakeLists.txt`](CMakeLists.txt) defines the application, feature library, and tests.
- [`build.bat`](build.bat) configures, builds, and runs tests with CMake, Ninja, and vcpkg. With no arguments it builds every target and runs every test.
- [`format.bat`](format.bat) applies the repository `.clang-format` configuration to all C/C++ files under `NEW`, including the Ghidra runtime sources.
- [`tidy.bat`](tidy.bat) applies the repository `.clang-tidy` checks and fixes to C/C++ translation units under `NEW`, including the Ghidra runtime sources; headers are analyzed through their including translation units.
- [`src/main.cpp`](src/main.cpp) is the application entry point; with no arguments it runs the smoke case, and with `<pe.exe> <language.sla>` it runs the provider-backed analyzer pipeline.
- [`features/README.md`](features/README.md) documents the feature library collection.
- [`features/hello/README.md`](features/hello/README.md) documents the sample feature module.
- [`features/decompiler/README.md`](features/decompiler/README.md) documents the standalone native decompiler engine and provider boundary.

## Requirements

- MSVC with C++23 support.
- CMake 3.28 or newer (the provided script uses CMake 4.4.2 from vcpkg).
- Ninja.
- vcpkg with the dependencies declared in [`vcpkg.json`](vcpkg.json); manifest mode installs `gtest`, `pugixml`, and `zlib` for the selected triplet. The manifest pins the builtin baseline to the repository's verified local vcpkg checkout.

Run `build.bat` from this directory for the complete build and test workflow. The script preserves the build directory, so later invocations are incremental:

```powershell
# Build and test every module and the application.
.\build.bat
.\build.bat all

# Build and test one feature from the project root.
.\build.bat sleigh
.\build.bat decompiler
.\build.bat pe
.\build.bat function_id
.\build.bat hello

# The same focused commands are available in each module directory.
features\sleigh_runtime\build.bat
features\decompiler\build.bat
features\pe_loader\build.bat
features\function_id\build.bat
features\hello\build.bat

# Compile a feature without running tests.
.\build.bat decompiler --no-test

# Build every target and run every registered test.
.\build.bat all
```

Tests are enabled by default in every mode. Use `--no-test` only for a compile-only check, and `--clean` only when a clean rebuild is required. Both options can be passed to a module wrapper. The full generated test list can be inspected with `ctest --test-dir build -N`.

Run `build/new_ghidra_app.exe <pe.exe> <language.sla>` to load a PE through
`features/pe_loader`, decode it through `features/sleigh_runtime`, and run all
registered analyzers through `features/analyzer`.

The decompiler implementation source selection is being restored as an explicit, reviewed list in [`features/decompiler/CMakeLists.txt`](features/decompiler/CMakeLists.txt), rather than a `CONFIGURE_DEPENDS` glob. That file and the decompiler modules are owned by the concurrent restoration work and were not changed here. The explicit list must remain synchronized with every restored implementation unit before the full build is considered complete; the current link diagnostics show that `src/fspec.cppm` is one required entry. Update [`features/decompiler/src/README.md`](features/decompiler/src/README.md) with the final source-list change.

Run `format.bat` from this directory to format all C/C++ sources, including `features/sleigh_runtime/src/ghidra`.

Run `tidy.bat` to apply clang-tidy fixes using `build/compile_commands.json`. If build artifacts are missing, the script runs `build.bat` first. Module consumers importing C++20 modules are reported as skipped because clang-tidy cannot consume MSVC `.ifc` files.
