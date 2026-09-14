# ASCII Strings

This directory ports `StringsAnalyzer` and its ASCII `StringSearcher` path. The independent
library is [`src/ascii_strings.cppm`](src/ascii_strings.cppm), and focused tests are in
[`tests/ascii_strings_tests.cppm`](tests/ascii_strings_tests.cppm). The executable fixture and
PyGhidra evidence are under [`tests/data/`](tests/data/).

[`CMakeLists.txt`](CMakeLists.txt) builds `analyzer_ascii_strings` and registers its GoogleTest
target. The module consumes only [`../shared/`](../shared/) and [`../../pe_loader/`](../../pe_loader/);
it does not duplicate PE parsing or instruction decoding.

Porting boundaries and source-to-source evidence are recorded in [`GHIDRA_PORT.md`](GHIDRA_PORT.md).
