# Call Convention ID

This analyzer invokes the existing native decompiler frontend and accepts only
an explicit calling-convention token emitted by its C printer. It does not
replace decompiler convention analysis with register or platform heuristics.

- [`src/call_convention_id.cppm`](src/call_convention_id.cppm) is the one declaration/implementation module.
- [`tests/call_convention_id_tests.cppm`](tests/call_convention_id_tests.cppm) contains hardcoded behavior tests.
- [`tests/data/`](tests/data/) contains the original fixture and report.
- [`CMakeLists.txt`](CMakeLists.txt) defines the library and CTest target.
- [`build.bat`](build.bat) selects the focused build from `build.bat`.
- [`../README.md`](../README.md) is the analyzer-family guide.
- [`../../decompiler/README.md`](../../decompiler/README.md) documents the frontend boundary.
- [`../../sleigh_runtime/README.md`](../../sleigh_runtime/README.md) documents the decoder dependency.

The current analyzer model has no separate signature-source field or custom
storage flag, so eligibility uses the available uncommitted/default signature
state and preserves the existing parameter vector.
