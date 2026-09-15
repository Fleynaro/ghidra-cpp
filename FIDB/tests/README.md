# FIDB Tests

This directory contains independent validation programs and orchestration for the real Ghidra Function ID database described in [`../README.md`](../README.md). It is not a test of `NEW/features/function_id`.

The [`zlib_detection`](zlib_detection/README.md) test compiles a real C++ consumer against the generated static zlib library and verifies the executable with original Ghidra/PyGhidra Function ID analysis.
