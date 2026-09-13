# Analyzer Tests

[`analyzer_tests.cppm`](analyzer_tests.cppm) contains Google Tests for the
registry, event coalescing, priority scheduling, provider-backed disassembly,
function discovery, CFG construction, pattern search, references, stack,
constant propagation, data pointers, and known/discovered no-return behavior.

The tests load executable fixtures from [`../test_data/`](../test_data/) through
[`../../pe_loader/`](../../pe_loader/) and use the compiled x64 SLA from
[`../../sleigh_runtime/`](../../sleigh_runtime/). Checked-in Ghidra reports are
read as golden Delta evidence, while assertions compare structured addresses,
entities, references, blocks, and facts. The target is configured by
[`CMakeLists.txt`](CMakeLists.txt) and registered with CTest by the parent
analyzer build.
