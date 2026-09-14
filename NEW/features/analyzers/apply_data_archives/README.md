# Apply Data Archives

[`src/apply_data_archives.cppm`](src/apply_data_archives.cppm) ports archive selection, default
enablement, priority, path validation, cancellation, and state recording from
[`ApplyDataArchiveAnalyzer.java`](../../../../Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/analysis/ApplyDataArchiveAnalyzer.java).

The standalone library and GoogleTest target are registered in
[`CMakeLists.txt`](CMakeLists.txt), and the incremental module workflow is
[`build.bat`](build.bat). Focused tests are in
[`tests/apply_data_archives_tests.cppm`](tests/apply_data_archives_tests.cppm),
with the original behavioral fixture under [`tests/data/`](tests/data/).
The analyzer consumes the shared model in [`../shared/`](../shared/).
The unavailable datatype-manager behavior and explicit limitation are recorded
in [`GHIDRA_PORT.md`](GHIDRA_PORT.md).
