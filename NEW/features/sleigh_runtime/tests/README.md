# Runtime Tests

[`sleigh_runtime_tests.cppm`](sleigh_runtime_tests.cppm) loads the local [`../test_data/x86-64.sla`](../test_data/x86-64.sla), decodes real x86-64 bytes, and checks lengths, mnemonics, operands, p-code opcode order, address spaces, offsets, and sizes.

The test target is declared in [`CMakeLists.txt`](CMakeLists.txt). The parent module is documented in [`../README.md`](../README.md).
