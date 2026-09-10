# Sleigh Runtime Decoder

This module decodes real machine bytes with a binary, compiled Ghidra `.sla` specification and returns assembly, operands, flow information, and materialized machine-independent p-code.

## Navigation

- [`CMakeLists.txt`](CMakeLists.txt) builds the runtime, imported reference sources, and tests.
- [`sleigh_runtime.cppm`](sleigh_runtime.cppm) exports the public C++23 module.
- [`include/sleigh_runtime.hpp`](include/sleigh_runtime.hpp) defines the owning public result model.
- [`src/sleigh_runtime.cpp`](src/sleigh_runtime.cpp) adapts the reference runtime to in-memory instruction bytes.
- [`src/README.md`](src/README.md) documents the implementation sources.
- [`src/ghidra/README.md`](src/ghidra/README.md) documents the imported Ghidra runtime sources.
- [`tests/sleigh_runtime_tests.cpp`](tests/sleigh_runtime_tests.cpp) contains structural end-to-end tests.
- [`test_data/x86-64.sla`](test_data/x86-64.sla) is the module-local processor specification used by tests.

The implementation intentionally does not parse `.slaspec`; all decoding tables and p-code templates come from the binary SLA file.

For x86-64, callers should provide the context fields from the processor specification, including `addrsize=2`, `opsize=1`, `rexprefix=0`, and `longMode=1`, as shown in [`tests/sleigh_runtime_tests.cpp`](tests/sleigh_runtime_tests.cpp).
