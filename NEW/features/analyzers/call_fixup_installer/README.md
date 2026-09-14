# Call Fixup Installer

[`src/call_fixup_installer.cppm`](src/call_fixup_installer.cppm) ports target matching and
non-returning caller repair from `CallFixupAnalyzer`. Compiler-spec mappings are supplied through
the public `CallFixupRule` boundary, so the module does not duplicate XML parsing.

Focused tests are in [`tests/call_fixup_installer_tests.cppm`](tests/call_fixup_installer_tests.cppm),
fixture evidence is in [`tests/data/`](tests/data/), and local build registration is in
[`CMakeLists.txt`](CMakeLists.txt). See [`GHIDRA_PORT.md`](GHIDRA_PORT.md) for unsupported state.
