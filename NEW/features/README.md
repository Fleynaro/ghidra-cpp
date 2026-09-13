# Features

This directory contains independently buildable feature modules for the C++ rewrite.

## Navigation

- [`CMakeLists.txt`](CMakeLists.txt) adds each feature subdirectory.
- [`hello/README.md`](hello/README.md) documents the sample `hello` module.
- [`sleigh_runtime/README.md`](sleigh_runtime/README.md) documents the independent compiled-SLA p-code runtime.
- [`pe_loader/README.md`](pe_loader/README.md) documents the standalone PE loader and loaded-image model.
- [`decompiler/README.md`](decompiler/README.md) documents the mechanically ported native engine and provider frontend.
- [`analyzers/README.md`](analyzers/README.md) documents the event-driven native analysis engine and its per-analyzer fixtures.

Each module also contains its own `build.bat` wrapper. Run the wrapper from that module directory to build and test only that module; tests are enabled by default. The wrappers call [`../../build.bat`](../../build.bat), so `--no-test` and `--clean` remain available.

```powershell
cd hello
.\build.bat
```
