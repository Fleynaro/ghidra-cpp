# Windows Resource Reference

[`src/windows_resource_reference.cppm`](src/windows_resource_reference.cppm) ports the Windows
resource lookup analyzer and its `WindowsResourceReference.java` helper using the existing PE
resource tree, Sleigh references, and constant facts. It does not parse a second resource format.

Focused tests are in [`tests/windows_resource_reference_tests.cppm`](tests/windows_resource_reference_tests.cppm);
fixture inputs and the PyGhidra report are under [`tests/data/`](tests/data/). Build registration is
local to [`CMakeLists.txt`](CMakeLists.txt), with fidelity notes in [`GHIDRA_PORT.md`](GHIDRA_PORT.md).
