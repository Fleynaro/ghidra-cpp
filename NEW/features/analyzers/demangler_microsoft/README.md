# Microsoft Demangler

[`src/demangler_microsoft.cppm`](src/demangler_microsoft.cppm) contains the analyzer and a reusable
native parser for Microsoft decorated names. It handles nested scopes, constructors/destructors,
vftable names, template components, primitive and compound types, pointers/references, calling
conventions, static/member functions, and parameter back references.

Focused parser and PE tests are in [`tests/demangler_microsoft_tests.cppm`](tests/demangler_microsoft_tests.cppm);
the original PyGhidra evidence is under [`tests/data/`](tests/data/). Build configuration is local to
[`CMakeLists.txt`](CMakeLists.txt), and exact source mapping is in [`GHIDRA_PORT.md`](GHIDRA_PORT.md).
