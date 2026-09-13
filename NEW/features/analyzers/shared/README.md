# Shared Analyzer Runtime

This module owns the common analysis model and event-driven scheduler used by every analyzer submodule. The C++23 module interface and implementation are in [`src/analyzer.cppm`](src/analyzer.cppm) and [`src/analyzer.cpp`](src/analyzer.cpp); [`CMakeLists.txt`](CMakeLists.txt) contributes them to `NewGhidra::Analyzer`. Feature-specific implementations are sibling directories under [`../`](../), and their tests and fixture data remain isolated from this shared runtime.
