# PDB MSDIA Behavioral Fixture

> Generated from actual Ghidra program-state snapshots; no PDB-derived addresses are hard-coded.

## Input

- **Executable:** `test_pdb_msdia.exe`
- **PDB:** `test_pdb_msdia.pdb`
- **Executable bytes:** `3072`
- **PDB applied by MSDIA:** `false`

## Analysis Configuration

| Enabled boolean option |
| --- |
| `PDB MSDIA` |

## Before target analysis
### PDB-derived properties visible before target analysis

| Name | Value | Provenance |
| --- | --- | --- |
| `PDB Age` | `f` | PE loader/default artifact |
| `PDB File` | `test_pdb_msdia.pdb` | PE loader/default artifact |
| `PDB GUID` | `4fe01cc2-df2c-2f49-533a-a30a7f394035` | PE loader/default artifact |
| `PDB Signature` | `` | PE loader/default artifact |
| `PDB Version` | `RSDS` | PE loader/default artifact |
### PDB artifacts visible before target analysis

#### Symbols

| Name | Address | Source | Provenance |
| --- | --- | --- | --- |
| `entry` | `0x0000000140001040` | `IMPORTED` | PE loader/default artifact |

#### Functions

| Name | Entry | Provenance |
| --- | --- | --- |
| `entry` | `0x0000000140001040` | PE loader/default artifact |

#### Data types

| Path | Provenance |
| --- | --- |
| `/DOS/IMAGE_DOS_HEADER` | PE loader/default artifact |
| `/GUID` | PE loader/default artifact |
| `/IMAGE_RICH_HEADER` | PE loader/default artifact |
| `/ImageBaseOffset32` | PE loader/default artifact |
| `/PDB/DotNetPdbInfo` | PE loader/default artifact |
| `/PE/IMAGE_DATA_DIRECTORY` | PE loader/default artifact |
| `/PE/IMAGE_DATA_DIRECTORY[16]` | PE loader/default artifact |
| `/PE/IMAGE_DEBUG_DIRECTORY` | PE loader/default artifact |
| `/PE/IMAGE_FILE_HEADER` | PE loader/default artifact |
| `/PE/IMAGE_NT_HEADERS64` | PE loader/default artifact |
| `/PE/IMAGE_OPTIONAL_HEADER64` | PE loader/default artifact |
| `/PE/IMAGE_SECTION_HEADER` | PE loader/default artifact |
| `/PE/Misc` | PE loader/default artifact |
| `/PE/SectionFlags` | PE loader/default artifact |
| `/PE/_IMAGE_RUNTIME_FUNCTION_ENTRY` | PE loader/default artifact |
| `/PEx64_UnwindInfo` | PE loader/default artifact |
| `/byte` | PE loader/default artifact |
| `/byte[64]` | PE loader/default artifact |
| `/char` | PE loader/default artifact |
| `/char[2]` | PE loader/default artifact |
| `/char[4]` | PE loader/default artifact |
| `/char[8]` | PE loader/default artifact |
| `/dword` | PE loader/default artifact |
| `/pointer64` | PE loader/default artifact |
| `/qword` | PE loader/default artifact |
| `/string` | PE loader/default artifact |
| `/undefined *64` | PE loader/default artifact |
| `/word` | PE loader/default artifact |
| `/word[10]` | PE loader/default artifact |
| `/word[4]` | PE loader/default artifact |

## After target analysis
### PDB-derived properties visible after target analysis

| Name | Value | Provenance |
| --- | --- | --- |
| `PDB Age` | `f` | PE loader/default artifact |
| `PDB File` | `test_pdb_msdia.pdb` | PE loader/default artifact |
| `PDB GUID` | `4fe01cc2-df2c-2f49-533a-a30a7f394035` | PE loader/default artifact |
| `PDB Signature` | `` | PE loader/default artifact |
| `PDB Version` | `RSDS` | PE loader/default artifact |
### PDB artifacts visible after target analysis

#### Symbols

| Name | Address | Source | Provenance |
| --- | --- | --- | --- |
| `entry` | `0x0000000140001040` | `IMPORTED` | PE loader/default artifact |

#### Functions

| Name | Entry | Provenance |
| --- | --- | --- |
| `entry` | `0x0000000140001040` | PE loader/default artifact |

#### Data types

| Path | Provenance |
| --- | --- |
| `/DOS/IMAGE_DOS_HEADER` | PE loader/default artifact |
| `/GUID` | PE loader/default artifact |
| `/IMAGE_RICH_HEADER` | PE loader/default artifact |
| `/ImageBaseOffset32` | PE loader/default artifact |
| `/PDB/DotNetPdbInfo` | PE loader/default artifact |
| `/PE/IMAGE_DATA_DIRECTORY` | PE loader/default artifact |
| `/PE/IMAGE_DATA_DIRECTORY[16]` | PE loader/default artifact |
| `/PE/IMAGE_DEBUG_DIRECTORY` | PE loader/default artifact |
| `/PE/IMAGE_FILE_HEADER` | PE loader/default artifact |
| `/PE/IMAGE_NT_HEADERS64` | PE loader/default artifact |
| `/PE/IMAGE_OPTIONAL_HEADER64` | PE loader/default artifact |
| `/PE/IMAGE_SECTION_HEADER` | PE loader/default artifact |
| `/PE/Misc` | PE loader/default artifact |
| `/PE/SectionFlags` | PE loader/default artifact |
| `/PE/_IMAGE_RUNTIME_FUNCTION_ENTRY` | PE loader/default artifact |
| `/PEx64_UnwindInfo` | PE loader/default artifact |
| `/byte` | PE loader/default artifact |
| `/byte[64]` | PE loader/default artifact |
| `/char` | PE loader/default artifact |
| `/char[2]` | PE loader/default artifact |
| `/char[4]` | PE loader/default artifact |
| `/char[8]` | PE loader/default artifact |
| `/dword` | PE loader/default artifact |
| `/pointer64` | PE loader/default artifact |
| `/qword` | PE loader/default artifact |
| `/string` | PE loader/default artifact |
| `/undefined *64` | PE loader/default artifact |
| `/word` | PE loader/default artifact |
| `/word[10]` | PE loader/default artifact |
| `/word[4]` | PE loader/default artifact |

## Delta

PDB properties already present before the target are PE loader/CodeView artifacts. Only rows in this section are attributed to the target analyzer.

### Property delta

| Change | Name | Before | After |
| --- | --- | --- | --- |
| _(none)_ | | | |

### Symbol delta

| Change | Name | Address | Source |
| --- | --- | --- | --- |
| _(none)_ |  |  |  |

### Function delta

| Change | Name | Entry |
| --- | --- | --- |
| _(none)_ |  |  |

### Data type delta

| Change | Path |
| --- | --- |
| _(none)_ |  |

### Delta conclusion

- **Exact artifact delta rows:** `0`.
- **MSDIA applicability:** No PDB-derived symbol, function, type, or loaded-property changes were observed. MSDIA could not apply the raw PDB in this environment; the loader/default artifacts above are not PDB analyzer output.
