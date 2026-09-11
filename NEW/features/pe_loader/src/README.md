# PE Loader Sources

This directory contains the C++23 module implementation for the parent [`pe_loader`](../README.md)
feature.

## Contents

- [`pe_loader.cppm`](pe_loader.cppm) declares and exports the public PE model and loader API.
- [`pe_loader.cpp`](pe_loader.cpp) implements checked PE parsing, directory decoding, and image mapping.

Both source files import the C++ standard library with `import std;`. The parent
[`CMakeLists.txt`](../CMakeLists.txt) registers the module interface, implementation, and the MSVC standard
library module. GoogleTest coverage remains in [`../tests`](../tests/README.md).
