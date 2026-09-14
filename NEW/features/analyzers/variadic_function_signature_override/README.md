# Variadic Function Signature Override

This module implements the supported printf/scanf format grammar and discovers
literal format arguments from production p-code. It records representable
call-site evidence as analysis bookmarks and deliberately does not mutate the
callee's persistent signature, because the native model has no call-site
override table.

- [`src/v.cppm`](src/v.cppm) is the one module.
- [`tests/t.cppm`](tests/t.cppm) contains hardcoded parser expectations.
- [`tests/data/`](tests/data/) contains the fixture and original analyzer report.
- [`CMakeLists.txt`](CMakeLists.txt) defines the target and CTest registration.
- [`build.bat`](build.bat) selects the focused build from `NEW/build.bat`.
- [`../../pe_loader/README.md`](../../pe_loader/README.md) documents the mapped-image dependency.
- [`../README.md`](../README.md) documents analyzer-family integration.
