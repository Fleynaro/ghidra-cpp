# New C++23 Project

`NEW` is the autonomous C++23 implementation. Feature code is owned by `services`; there is no parallel `features` build graph.

## Navigation

- [`CMakeLists.txt`](CMakeLists.txt) defines core, runtime, services, bindings, application, and tests.
- [`core/README.md`](core/README.md) documents canonical domain values, contracts, and events.
- [`runtime/README.md`](runtime/README.md) documents workers, persistence, projections, scheduling, and lifecycle.
- [`services/README.md`](services/README.md) documents migrated feature services and their tests.
- [`bindings/cpp/README.md`](bindings/cpp/README.md) documents the native C++ facade. Python, JavaScript, and Go bindings are out of scope.
- [`tests/README.md`](tests/README.md) documents root integration and replay tests.

## Build

Requirements are MSVC with C++23, CMake 3.28+, Ninja, vcpkg, and the packages in [`vcpkg.json`](vcpkg.json): GTest, pugixml, SQLite3, and zlib.

```powershell
.\build.bat all
.\build.bat core
.\build.bat runtime
.\build.bat services
.\build.bat project
.\build.bat integration
.\build.bat replay
.\build.bat decompiler --no-test
```

The focused service modes preserve the existing names (`sleigh`, `pe`, `function_id`, `decompiler`, `hello`, and `analyzer`). Use `--no-test` only for compile-only checks and `--clean` only when the build graph changes. Inspect registered tests with `ctest --test-dir build -N`.

The executable pipeline loads a PE through [`services/pe_loader`](services/pe_loader/README.md), decodes it through [`services/sleigh`](services/sleigh/README.md), schedules analyzer services through [`runtime/analysis`](runtime/analysis/README.md), and exposes decompilation through the native facade.

The decompiler source list is explicit in [`services/decompiler/CMakeLists.txt`](services/decompiler/CMakeLists.txt), including `src/fspec.cppm`; its tests live in [`services/decompiler/tests`](services/decompiler/tests).

Run `format.bat` and `tidy.bat` from this directory to format and validate the migrated C++ modules. MSVC module consumers are reported as skipped by tidy because clang-tidy cannot consume `.ifc` files directly.
