#Analyzer Tests

[`analyzer_tests.cppm`](analyzer_tests.cppm) contains Google Tests for the
registry, event coalescing, priority scheduling, provider-backed disassembly,
function discovery, CFG construction, shared bodies, pattern search, references,
stack, constant propagation, data pointers, external imports, lifecycle events,
and known/discovered no-return behavior.

The tests load executable fixtures from [`../test_data/`](../test_data/) through
[`../../pe_loader/`](../../pe_loader/) and use the compiled x64 SLA from
[`../../sleigh_runtime/`](../../sleigh_runtime/). Golden values from the Ghidra
Delta reports are copied into typed C++ constants in the test module;
no report file is loaded at test runtime.The target is configured by[`CMakeLists.txt`](CMakeLists.txt) and
    registered with CTest by the parent analyzer build.
