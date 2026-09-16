# Embedded Media

Ports `EmbeddedMediaAnalyzer.added()` and its bookmark option from
[`Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/analysis/EmbeddedMediaAnalyzer.java`](../../../../Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/analysis/EmbeddedMediaAnalyzer.java).

The analyzer scans initialized mapped PE regions and validates GIF87a/GIF89a,
PNG, JPEG/JFIF, WAVE, MIDI, AU, and AIFF/AIFC containers before adding an
`EmbeddedMediaRecord` and a corresponding `DataObject`. Media lengths preserve
the dynamic datatype contracts used by the original media datatypes. Analysis
bookmarks use the original category and comment strings.

The declaration and implementation are in [`src/embedded_media.cppm`](src/embedded_media.cppm),
the target and test registration are in [`CMakeLists.txt`](CMakeLists.txt), the
incremental wrapper is [`build.bat`](build.bat), and the fixture-backed
GoogleTests are in [`tests/embedded_media_tests.cppm`](tests/embedded_media_tests.cppm).
Human-readable fixture evidence is under [`tests/data/`](tests/data/), and shared
model dependencies are under [`../shared/`](../shared/).

Porting decisions and known differences are recorded in [`GHIDRA_PORT.md`](GHIDRA_PORT.md).
