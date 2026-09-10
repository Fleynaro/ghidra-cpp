# Hello Feature Tests

This directory contains the Google Test coverage for the `hello` module.

## Navigation

- [`CMakeLists.txt`](CMakeLists.txt) defines `hello_feature_tests` and registers it with CTest.
- [`hello_tests.cpp`](hello_tests.cpp) checks successful message construction and validation errors.
- [`../hello.cppm`](../hello.cppm) contains the functionality under test.
- [`../../../build.bat`](../../../build.bat) builds and runs this suite.
