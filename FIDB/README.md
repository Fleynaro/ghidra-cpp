# zlib Function ID Database

This directory is a self-contained, reproducible pipeline for producing and validating a real Ghidra Function ID database for zlib. It does not use or modify `NEW/features/function_id`; the only FID implementation used at runtime is the installed Ghidra/PyGhidra implementation.

## Selected Build

The checked-in [`config.json`](config.json) fixes the build and database identity:

| Property | Value |
| --- | --- |
| zlib | 1.3.1, official release archive from `https://zlib.net/fossils/zlib-1.3.1.tar.gz` |
| Architecture | x86-64 (`x64` MSVC environment) |
| Compiler | Microsoft Visual C++, selected with `vcvarsall.bat x64` |
| Configuration | Release, `/O2`, `/Oy-`, `/MD`, upstream `/Zi` |
| Linkage | Static `zlib.lib` |
| Ghidra language | `x86:LE:64:default` |
| Ghidra compiler specification | `windows` |
| Source language | `C` (zlib source; the installed legacy FID schema stores no source-language column) |
| FID library | family `zlib`, version `1.3.1`, variant `msvc-x86_64-release` |

The build stage records the downloaded archive SHA-256, compiler version, exact upstream make target, object files, and paths in [`build/zlib/metadata.json`](build/zlib/metadata.json). `build/` is ignored because it is machine-generated.

## Confirmed Ghidra Implementation

The implementation was researched before writing this pipeline. The local source tree corresponds to the Ghidra sources used by the installed Ghidra 12.1.3 runtime:

| Behavior | Original source |
| --- | --- |
| FID service construction, FNV digest setup, hashing, ingestion entry point, and querying | [`Ghidra/Features/FunctionID/src/main/java/ghidra/feature/fid/service/FidService.java`](../Ghidra/Features/FunctionID/src/main/java/ghidra/feature/fid/service/FidService.java), especially `getHasher`, `createNewLibraryFromPrograms`, `processProgram` |
| Normal library ingestion, named symbols, function filters, child relations, duplicate suppression, and `FidPopulateResult` | [`Ghidra/Features/FunctionID/src/main/java/ghidra/feature/fid/service/FidServiceLibraryIngest.java`](../Ghidra/Features/FunctionID/src/main/java/ghidra/feature/fid/service/FidServiceLibraryIngest.java) |
| Hashing contract and `FidHashQuad` generation | [`Ghidra/Features/FunctionID/src/main/java/ghidra/feature/fid/hash/FidHasher.java`](../Ghidra/Features/FunctionID/src/main/java/ghidra/feature/fid/hash/FidHasher.java) and the neighboring `MessageDigestFidHasher.java` |
| Official library-import analysis setup | [`Ghidra/Features/FunctionID/ghidra_scripts/FunctionIDHeadlessPrescript.java`](../Ghidra/Features/FunctionID/ghidra_scripts/FunctionIDHeadlessPrescript.java) |
| Official multi-library creation workflow and call shape | [`Ghidra/Features/FunctionID/ghidra_scripts/CreateMultipleLibraries.java`](../Ghidra/Features/FunctionID/ghidra_scripts/CreateMultipleLibraries.java), especially `populateLibrary` and `FidService.createNewLibraryFromPrograms` |
| Packed database schema, metadata, and function records | [`FidDB.java`](../Ghidra/Features/FunctionID/src/main/java/ghidra/feature/fid/db/FidDB.java), [`LibrariesTable.java`](../Ghidra/Features/FunctionID/src/main/java/ghidra/feature/fid/db/LibrariesTable.java), [`FunctionsTable.java`](../Ghidra/Features/FunctionID/src/main/java/ghidra/feature/fid/db/FunctionsTable.java) |
| Normal analyzer path | [`FidAnalyzer.java`](../Ghidra/Features/FunctionID/src/main/java/ghidra/feature/fid/analyzer/FidAnalyzer.java) and [`ApplyFidEntriesCommand.java`](../Ghidra/Features/FunctionID/src/main/java/ghidra/feature/fid/cmd/ApplyFidEntriesCommand.java) |
| Function ID behavior, filters, thresholds, library metadata, and analysis rules | [`fid.xml`](../Ghidra/Features/FunctionID/src/main/doc/fid.xml) |

Important implementation facts used here:

1. `FidService.createNewLibraryFromPrograms(...)` constructs `FidServiceLibraryIngest`, which hashes each non-external, non-thunk function with Ghidra's `FidHasher`, preserves defined symbols, records terminators, and creates intra-library call relations. The Python scripts pass `DomainFile` objects from a Ghidra project, exactly as the official Java script does.
2. A packed database is created by `FidFileManager.createNewFidDatabase`, populated through `FidFile.getFidDB(true)`, committed with `FidDB.saveDatabase`, and reopened through `FidFileManager`. No `.fidb` bytes are constructed or edited by this project.
3. `FunctionIDHeadlessPrescript` disables `Function ID`, `Library Identification`, and language demanglers while enabling `Scalar Operand References`. [`scripts/analyze_library.py`](scripts/analyze_library.py) executes that original Java script through `pyghidra.ghidra_script`; it does not duplicate the option logic.
4. The consumer stage attaches the generated file with `FidFileManager`, runs normal analysis, then invokes the original `FidAnalyzer.added(...)` path. It also calls `FidService.processProgram(...)` independently and records the actual `FidMatch` library/name/score evidence.
5. The canonical Function ID documentation states that full hashes are relocation-robust but compilation settings affect hashes. This is why the consumer is built with the same MSVC x64 Release/static configuration as the source objects.

The installed `Ghidra/Features/FunctionID/lib/FunctionID.jar` was also inspected. Its 12.1.3 `FidService` exposes the legacy `LanguageID` ingestion overload and its `LibrariesTable` stores a compiler-spec column but no source-language column. The checked-in source tree has the newer `FidFilter`/source-language API, so this pipeline uses the installed legacy overload rather than pretending the newer filter exists. The zlib source language remains explicitly recorded as `C` in [`config.json`](config.json) and the generated report records the actual empty/all-source database metadata. This is an upstream schema/API limitation, not a database reimplementation or hand edit.

PyGhidra 3.1.0 is loaded from the Ghidra-managed virtual environment by [`TEST/run_ghidra_python.bat`](../TEST/run_ghidra_python.bat). Every PyGhidra stage in this subproject is launched through that wrapper. `GHIDRA_INSTALL_DIR` must point to the installed Ghidra 12.1.3 directory.

## Pipeline Layout

```text
FIDB/
├── README.md
├── config.json
├── scripts/
│   ├── common.py
│   ├── build_zlib.py
│   ├── analyze_library.py
│   ├── create_fidb.py
│   └── verify_fidb.py
├── libraries/zlib/1.3.1/
│   └── zlib-1.3.1-msvc-x86_64-release.fidb
├── tests/zlib_detection/
│   ├── README.md
│   ├── test.cpp
│   ├── build_consumer.py
│   └── run_test.py
└── reports/
    ├── library_analysis.json
    ├── fidb_generation.json
    ├── fidb_verification.json
    └── fidb_verification.md
```

## Reproduction

From the repository root, set the required environment variable and run:

```powershell
$env:GHIDRA_INSTALL_DIR = 'C:\path\to\ghidra_12.1.3_PUBLIC'
python FIDB\tests\zlib_detection\run_test.py --force
```

The command performs the explicit stages in order:

1. `scripts/build_zlib.py` downloads/extracts zlib 1.3.1 and runs the upstream `nmake -f win32/Makefile.msc zlib.lib` target under x64 MSVC.
2. `scripts/analyze_library.py` imports every produced `.obj` into a persistent Ghidra project, executes the original headless FID prescript, and analyzes the objects without FID/LID contamination.
3. `scripts/create_fidb.py` calls the original `FidService` ingestion API and emits the packed `.fidb`.
4. `tests/zlib_detection/build_consumer.py` compiles [`test.cpp`](tests/zlib_detection/test.cpp) against the generated static library without a PDB, so zlib code is not identified merely from consumer debug symbols.
5. `scripts/verify_fidb.py` opens the `.fidb`, checks metadata and records, analyzes the executable, runs original Ghidra Function ID, and verifies `adler32_z`, `compress`, `crc32_z`, and `uncompress` plus the non-zlib control. The short `adler32` and `crc32` entry points are intentionally not expected: original `FidServiceLibraryIngest` excludes their three-code-unit MSVC bodies at its documented four-code-unit minimum.

Individual stages are also available:

```powershell
python FIDB\scripts\build_zlib.py --force
TEST\run_ghidra_python.bat FIDB\scripts\analyze_library.py --force
TEST\run_ghidra_python.bat FIDB\scripts\create_fidb.py --force
python FIDB\tests\zlib_detection\build_consumer.py
```

The final verification command is normally run through `run_test.py`, because it supplies the generated executable, linker map, and `.fidb` paths.

## Validation Evidence

The machine-readable reports are generated only after the relevant Ghidra operation succeeds:

| Report | Evidence |
| --- | --- |
| [`reports/library_analysis.json`](reports/library_analysis.json) | Imported program language/compiler, prescript option state, symbol sources, and analyzed function counts |
| [`reports/fidb_generation.json`](reports/fidb_generation.json) | FidService population counts, database metadata, function records, and reopen count |
| [`reports/fidb_verification.json`](reports/fidb_verification.json) | Ghidra version/PyGhidra version, direct FidService matches, analyzer markup/bookmarks, expected matches, and negative-control evidence |
| [`reports/fidb_verification.md`](reports/fidb_verification.md) | Concise human-readable PASS summary with expected functions, scores, and negative-control result |

Failures are fatal and include the Ghidra installation, language/compiler choice, input paths, expected names, actual matches, and failed expectations where available. The generated `.fidb` is a normal packed Ghidra database and is intended to be attached through the normal Function ID infrastructure.
