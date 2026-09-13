# PDB MSDIA Behavioral Fixture

> Generated from actual Ghidra program state; no PDB-derived addresses are hard-coded.

## Input

- **Executable:** `test_pdb_msdia.exe`
- **PDB:** `test_pdb_msdia.pdb`
- **Executable bytes:** `3072`
- **PDB applied by MSDIA:** `false`

## Analysis Configuration

| Enabled boolean option |
| --- |
| `PDB MSDIA` |

## PDB Program Properties

| Name | Value |
| --- | --- |
| `PDB Age` | `c` |
| `PDB File` | `test_pdb_msdia.pdb` |
| `PDB GUID` | `7a2e3a25-772b-c44c-27fc-1f0596875ccb` |
| `PDB Version` | `RSDS` |

## Observed Symbols Without PDB Application

| Name | Address | Source |
| --- | --- | --- |
| `entry` | `0x0000000140001040` | `IMPORTED` |

## Observed Functions Without PDB Application

| Name | Entry |
| --- | --- |
| `entry` | `0x0000000140001040` |

## Observed Data Types Without PDB Application

| Path |
| --- |
| `/DOS/IMAGE_DOS_HEADER` |
| `/GUID` |
| `/IMAGE_RICH_HEADER` |
| `/ImageBaseOffset32` |
| `/PDB/DotNetPdbInfo` |
| `/PE/IMAGE_DATA_DIRECTORY` |
| `/PE/IMAGE_DATA_DIRECTORY[16]` |
| `/PE/IMAGE_DEBUG_DIRECTORY` |
| `/PE/IMAGE_FILE_HEADER` |
| `/PE/IMAGE_NT_HEADERS64` |
| `/PE/IMAGE_OPTIONAL_HEADER64` |
| `/PE/IMAGE_SECTION_HEADER` |
| `/PE/Misc` |
| `/PE/SectionFlags` |
| `/PE/_IMAGE_RUNTIME_FUNCTION_ENTRY` |
| `/PEx64_UnwindInfo` |
| `/byte` |
| `/byte[64]` |
| `/char` |
| `/char[2]` |
| `/char[4]` |
| `/char[8]` |
| `/dword` |
| `/pointer64` |
| `/qword` |
| `/string` |
| `/undefined *64` |
| `/word` |
| `/word[10]` |
| `/word[4]` |

## Environment Limitation

The legacy analyzer did not set `PDB Loaded` and no PDB-derived symbols or user types were observed. This run therefore does not claim raw-PDB extraction; install/configure the Windows DIA SDK or provide a matching preprocessed `.pdb.xml` for MSDIA processing.
