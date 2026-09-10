# New C++23 Project

This directory contains the initial C++23 rewrite workspace developed alongside the Java Ghidra project.

## Navigation

- [`CMakeLists.txt`](CMakeLists.txt) defines the application, feature library, and tests.
- [`build.bat`](build.bat) configures, builds, and runs tests with CMake, Ninja, and vcpkg.
- [`src/main.cpp`](src/main.cpp) is the application entry point.
- [`features/README.md`](features/README.md) documents the feature library collection.
- [`features/hello/README.md`](features/hello/README.md) documents the sample feature module.

## Requirements

- MSVC with C++23 support.
- CMake 3.28 or newer (the provided script uses CMake 4.4.2 from vcpkg).
- Ninja.
- vcpkg with `gtest:x64-windows` installed.

Run `build.bat` from this directory to create `build/new_ghidra_app.exe` and execute the tests.
