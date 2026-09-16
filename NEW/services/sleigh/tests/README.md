# Runtime Tests

[`sleigh_runtime_tests.cppm`](sleigh_runtime_tests.cppm) loads the module-level [`../specifications/x86-64.sla`](../specifications/x86-64.sla) by bare filename, decodes real x86-64 bytes, and checks lengths, mnemonics, operands, p-code opcode order, address spaces, offsets, and sizes. ARM cases use the same resolver with `ARM8_le.sla`.

The test target is declared in [`CMakeLists.txt`](CMakeLists.txt). The parent module is documented in [`../README.md`](../README.md).
