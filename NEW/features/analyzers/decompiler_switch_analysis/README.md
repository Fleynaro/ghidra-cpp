# Decompiler Switch Analysis

This analyzer runs the existing native decompiler flow/jump-table pipeline and
persists its recovered block destinations as computed-jump references. It does
not infer switch targets from ABI conventions or a hardcoded fixture table.

- [`src/decompiler_switch_analysis.cppm`](src/decompiler_switch_analysis.cppm) contains the declaration and implementation.
- [`tests/decompiler_switch_analysis_tests.cppm`](tests/decompiler_switch_analysis_tests.cppm) contains hardcoded parser and fixture expectations.
- [`tests/data/`](tests/data/) contains the original switch fixture and report.
- [`CMakeLists.txt`](CMakeLists.txt) defines the library and test target.
- [`build.bat`](build.bat) selects the focused build from `NEW/build.bat`.
- [`../../decompiler/README.md`](../../decompiler/README.md) documents the frontend.
- [`../../sleigh_runtime/README.md`](../../sleigh_runtime/README.md) documents the decoder dependency.
- [`../README.md`](../README.md) documents analyzer integration.
