# Ghidra Port Evidence

This file records the native port of **Embedded Media**. It complements
[`README.md`](README.md), [`src/embedded_media.cppm`](src/embedded_media.cppm),
[`tests/embedded_media_tests.cppm`](tests/embedded_media_tests.cppm), and
[`CMakeLists.txt`](CMakeLists.txt).

## Original Sources

- [`EmbeddedMediaAnalyzer.java`](../../../../Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/analysis/EmbeddedMediaAnalyzer.java)
  supplies the analyzer name, block-analysis scheduling, byte-pattern list,
  undefined-listing guard, CreateData behavior, and bookmark option.
- [`GifDataType.java`](../../../../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/model/data/GifDataType.java)
  and its `GIFResource` parser supply GIF dynamic lengths.
- [`PngDataType.java`](../../../../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/model/data/PngDataType.java)
  supplies PNG signature, chunk, and dynamic-length behavior.
- [`JPEGDataType.java`](../../../../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/model/data/JPEGDataType.java),
  [`WAVEDataType.java`](../../../../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/model/data/WAVEDataType.java),
  [`MIDIDataType.java`](../../../../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/model/data/MIDIDataType.java),
  [`AUDataType.java`](../../../../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/model/data/AUDataType.java),
  and [`AIFFDataType.java`](../../../../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/model/data/AIFFDataType.java)
  supply the remaining magic and length rules.

## Transferred Behavior

- Search is limited to initialized mapped memory and never overwrites an
  existing native instruction or data object.
- All media signatures from the Java analyzer are covered, including both GIF
  revisions and both AIFF form types.
- Dynamic lengths are bounded by the containing mapped region. PNG CRCs,
  JPEG segment/scan markers, GIF sub-blocks, MIDI tracks, and RIFF/FORM lengths
  are validated before mutation.
- Successful matches create the model record/data object and the exact
  `Embedded Media` bookmark wording when bookmarks are enabled.
- Repeated event scheduling is idempotent through `AnalysisContext` duplicate
  and overlap checks.

## Native Boundary and Differences

- [`AnalysisContext`](../shared/src/analyzer_context.cppm) owns mapped PE memory
  and listing artifacts; this module does not parse PE headers or decode Sleigh.
- Java Swing/audio value objects are intentionally not represented. The native
  observable contract is the validated type, length, data object, and bookmark.
- Java ImageIO performs full JPEG image decoding. The native implementation
  validates JFIF structure and complete marker/scan termination without adding
  an image library dependency; malformed or truncated streams are rejected.
- The Java searcher receives a caller `AddressSetView`; the event-driven native
  pass scans all initialized regions because memory events carry seed addresses,
  not a complete range object.

## Verification

[`tests/embedded_media_tests.cppm`](tests/embedded_media_tests.cppm) hardcodes
the seven fixture rows from [`tests/data/test_embedded_media.md`](tests/data/test_embedded_media.md)
and never reads Markdown at runtime. The executable fixture and its generator
are in [`tests/data/`](tests/data/), with PyGhidra execution required through
[`TEST/run_ghidra_python.bat`](../../../../TEST/run_ghidra_python.bat).
