# PE Loader Tests

The [`pe_loader_tests.cpp`](pe_loader_tests.cpp) target uses GoogleTest through
[`CMakeLists.txt`](CMakeLists.txt). The primary integration fixture is
[`data/test.exe`](data/test.exe), with expected values documented in

The suite asserts concrete headers, Rich records, Ghidra-compatible section block sizes, and aligned
virtual-range validation, directory addresses,
imports/exports, relocation and exception counts, CodeView identity, TLS callbacks, CFG load-config
fields, resource leaves and payload bytes, memory bytes, address translation, and malformed-input errors.
Synthetic PE byte builders cover PE32/PE32+, SectionAlignment and FileAlignment, 64-bit thunk overflow,
CLR `cb` bounds, strict versus partial parsing, forwarded exports, RVA/VA delay imports, certificates,
ARM-family exception records, architecture/global-pointer directories, and resource payload extraction.
