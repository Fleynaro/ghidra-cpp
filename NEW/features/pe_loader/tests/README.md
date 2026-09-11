# PE Loader Tests

The [`pe_loader_tests.cpp`](pe_loader_tests.cpp) target uses GoogleTest through
[`CMakeLists.txt`](CMakeLists.txt). The primary integration fixture is
[`../../../../TEST/test.exe`](../../../../TEST/test.exe), with expected values documented in
[`../../../../TEST/test.exe.md`](../../../../TEST/test.exe.md).

The suite asserts concrete headers, Rich records, all section layout values, directory addresses,
imports/exports, relocation and exception counts, CodeView identity, TLS callbacks, CFG load-config
fields, resource leaves, memory bytes, address translation, and malformed-input errors. Synthetic PE
byte builders cover PE32, ordinal imports, virtual-only section tails, and checked range failures so
coverage does not depend solely on one executable.
