# PE Loader Sources

This directory contains the C++23 module implementation for the parent [`pe_loader`](../README.md)
feature.

## Contents

- [`pe_loader.cppm`](pe_loader.cppm) declares, exports, and implements the complete public PE model and loader API.

The module imports the C++ standard library with `import std;`. The parent
[`CMakeLists.txt`](../CMakeLists.txt) registers the module and the MSVC standard library module. GoogleTest
coverage remains in [`../tests`](../tests/README.md).
