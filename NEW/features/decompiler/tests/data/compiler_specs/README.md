# Compiler-Spec Fixtures

This directory contains the four compiler-spec XML fixtures required by
[`native_paramstore_tests.cppm`](../../native_paramstore_tests.cppm). They are copied into
`NEW` so the test reads only module-local data and never resolves a Ghidra installation path.

- [`x86-64-gcc.cspec`](x86-64-gcc.cspec) supplies the x64 System V models.
- [`ppc_64_be.cspec`](ppc_64_be.cspec) supplies the PowerPC64 big-endian models.
- [`mips32be.cspec`](mips32be.cspec) supplies the MIPS32 big-endian models.
- [`AARCH64.cspec`](AARCH64.cspec) supplies the AArch64 models.

The files preserve the original compiler-spec XML contract used by the native parameter-storage
tests; the test reduces each document to the elements needed by the native decompiler model.
