# Decompiler Parameter ID

This analyzer runs the existing native decompiler frontend over bounded
function bodies and records completion only after a non-empty decompilation
artifact is produced. Parameter storage and types are not guessed in the
analyzer.

- [`src/decompiler_parameter_id.cppm`](src/decompiler_parameter_id.cppm) is the single module implementation.
- [`tests/decompiler_parameter_id_tests.cppm`](tests/decompiler_parameter_id_tests.cppm) contains hardcoded tests.
- [`tests/data/`](tests/data/) contains the fixture and original report.
- [`CMakeLists.txt`](CMakeLists.txt) defines the target and CTest registration.
- [`build.bat`](build.bat) selects the focused build from `build.bat`.
- [`../../decompiler/README.md`](../../decompiler/README.md) documents the frontend contract.
- [`../../sleigh_runtime/README.md`](../../sleigh_runtime/README.md) documents the decoder dependency.
- [`../README.md`](../README.md) documents analyzer-family integration.
