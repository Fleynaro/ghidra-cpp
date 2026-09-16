# Ghidra Port Evidence

- Original: [`ApplyDataArchiveAnalyzer.java`](../../../../Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/analysis/ApplyDataArchiveAnalyzer.java).
- Native source: [`src/apply_data_archives.cppm`](src/apply_data_archives.cppm).
- Native tests: [`tests/apply_data_archives_tests.cppm`](tests/apply_data_archives_tests.cppm).
- Build target: [`CMakeLists.txt`](CMakeLists.txt); module wrapper: [`build.bat`](build.bat).
- Shared model dependency: [`../shared/README.md`](../shared/README.md).
- Fixture evidence: [`tests/data/test_apply_data_archives.md`](tests/data/test_apply_data_archives.md).

The port preserves analyzer name, Java priority `801`, explicit archive-path validation,
source-language propagation, duplicate suppression, cancellation, and diagnostic state. The original
constructor explicitly enables this analyzer by default; the native shared `AnalysisOptions` currently
defaults `apply_data_archives` to false. That host-level default mismatch cannot be corrected inside
this module without editing the protected shared analyzer options API, so the native analyzer honors
the current host value and the focused test verifies the explicit disabled path.

The original calls `DataTypeArchiveUtility`, `DataTypeManagerService`, `SourceLanguageService`,
`ApplyFunctionDataTypesCmd`, project archives, built-in archive discovery, and Go RTTI detection.
None of those services is a public native `AnalysisContext` API. The module therefore records a valid
`.gdt` selection as `applied=false` with a precise service-unavailable error and rejects invalid
paths; it does not falsely mark function signatures or datatypes as applied. Archive chooser and
automatic source-language discovery remain pending the prerequisite public API.
