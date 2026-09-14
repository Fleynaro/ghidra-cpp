# External Entry References

This module ports `ExternalEntryFunctionAnalyzer` in [`src/external_entry_references.cppm`](src/external_entry_references.cppm).
It consumes executable exports and decoded instructions from [`../../pe_loader/`](../../pe_loader/) and
[`../shared/`](../shared/), then creates named functions through `AnalysisContext::create_function`.

Focused structural tests are in [`tests/external_entry_references_tests.cppm`](tests/external_entry_references_tests.cppm);
the saved PE evidence is under [`tests/data/`](tests/data/). Build registration is local to
[`CMakeLists.txt`](CMakeLists.txt), and port boundaries are in [`GHIDRA_PORT.md`](GHIDRA_PORT.md).
