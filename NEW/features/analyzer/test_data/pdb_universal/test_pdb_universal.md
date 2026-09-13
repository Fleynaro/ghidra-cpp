# PDB Universal Behavioral Fixture

> Generated from actual Ghidra program-state snapshots before and after raw PDB Universal analysis.

## Input

- **Executable:** `test_pdb_universal.exe`
- **Raw PDB:** `test_pdb_universal.pdb`
- **Executable bytes:** `3072`

## Analysis Configuration

| Enabled boolean option |
| --- |
| `PDB Universal` |

## Before target analysis
### PDB artifacts visible before target analysis

#### Program properties

| Name | Value | Provenance |
| --- | --- | --- |
| `PDB Age` | `c` | PE loader/default artifact |
| `PDB File` | `test_pdb_universal.pdb` | PE loader/default artifact |
| `PDB GUID` | `4d1a0d5b-567a-67c8-83ba-d3f22c4b9fbf` | PE loader/default artifact |
| `PDB Signature` | `` | PE loader/default artifact |
| `PDB Version` | `RSDS` | PE loader/default artifact |

#### Symbols

| Name | Address | Source | Provenance |
| --- | --- | --- | --- |
| `entry` | `0x0000000140001050` | `IMPORTED` | PE loader/default artifact |

#### Functions

| Name | Entry | Provenance |
| --- | --- | --- |
| `entry` | `0x0000000140001050` | PE loader/default artifact |

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
### PDB artifacts visible after target analysis

#### Program properties

| Name | Value | Provenance |
| --- | --- | --- |
| `PDB Age` | `c` | PE loader/default artifact |
| `PDB File` | `test_pdb_universal.pdb` | PE loader/default artifact |
| `PDB GUID` | `4d1a0d5b-567a-67c8-83ba-d3f22c4b9fbf` | PE loader/default artifact |
| `PDB Loaded` | `true` | PDB analyzer delta |
| `PDB Signature` | `` | PE loader/default artifact |
| `PDB Version` | `RSDS` | PE loader/default artifact |

#### Symbols

| Name | Address | Source | Provenance |
| --- | --- | --- | --- |
| `.bss` | `0x0000000140003000` | `IMPORTED` | PDB analyzer delta |
| `.edata` | `0x000000014000222C` | `IMPORTED` | PDB analyzer delta |
| `.pdata` | `0x0000000140004000` | `IMPORTED` | PDB analyzer delta |
| `.rdata` | `0x0000000140002000` | `IMPORTED` | PDB analyzer delta |
| `.rdata$voltmd` | `0x0000000140002084` | `IMPORTED` | PDB analyzer delta |
| `.rdata$zzzdbg` | `0x00000001400020B4` | `IMPORTED` | PDB analyzer delta |
| `.text$mn` | `0x0000000140001000` | `IMPORTED` | PDB analyzer delta |
| `.xdata` | `0x0000000140002224` | `IMPORTED` | PDB analyzer delta |
| `entry` | `0x0000000140001050` | `IMPORTED` | PE loader/default artifact |
| `pdb_universal_compute` | `0x0000000140001000` | `IMPORTED` | PDB analyzer delta |
| `pdb_universal_entry` | `0x0000000140001050` | `IMPORTED` | PDB analyzer delta |
| `pdb_universal_global` | `0x0000000140003000` | `IMPORTED` | PDB analyzer delta |

#### Functions

| Name | Entry | Provenance |
| --- | --- | --- |
| `pdb_universal_compute` | `0x0000000140001000` | PDB analyzer delta |
| `pdb_universal_entry` | `0x0000000140001050` | PDB analyzer delta |

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
| `/bool` | PDB analyzer delta |
| `/byte` | PE loader/default artifact |
| `/byte[64]` | PE loader/default artifact |
| `/char` | PE loader/default artifact |
| `/char *` | PDB analyzer delta |
| `/char * *` | PDB analyzer delta |
| `/char[18]` | PDB analyzer delta |
| `/char[2]` | PE loader/default artifact |
| `/char[4]` | PE loader/default artifact |
| `/char[8]` | PE loader/default artifact |
| `/dword` | PE loader/default artifact |
| `/int` | PDB analyzer delta |
| `/long64` | PDB analyzer delta |
| `/long64 *` | PDB analyzer delta |
| `/longlong` | PDB analyzer delta |
| `/pointer64` | PE loader/default artifact |
| `/qword` | PE loader/default artifact |
| `/string` | PE loader/default artifact |
| `/test_pdb_universal.pdb/!_anon_funcs_/_func___cdecl_long64_PdbUniversalRecord_ptr_PdbUniversalMode` | PDB analyzer delta |
| `/test_pdb_universal.pdb/!_anon_funcs_/_func___cdecl_void` | PDB analyzer delta |
| `/test_pdb_universal.pdb/PdbUniversalMode` | PDB analyzer delta |
| `/test_pdb_universal.pdb/PdbUniversalRecord` | PDB analyzer delta |
| `/test_pdb_universal.pdb/PdbUniversalRecord *` | PDB analyzer delta |
| `/test_pdb_universal.pdb/__vc_attributes/aggregatableAttribute` | PDB analyzer delta |
| `/test_pdb_universal.pdb/__vc_attributes/aggregatableAttribute *` | PDB analyzer delta |
| `/test_pdb_universal.pdb/__vc_attributes/aggregatableAttribute/!_anon_funcs_/_func___thiscall_aggregatableAttribute_ptr` | PDB analyzer delta |
| `/test_pdb_universal.pdb/__vc_attributes/aggregatableAttribute/!_anon_funcs_/_func___thiscall_aggregatableAttribute_ptr_type_e` | PDB analyzer delta |
| `/test_pdb_universal.pdb/__vc_attributes/aggregatableAttribute/type_e` | PDB analyzer delta |
| `/test_pdb_universal.pdb/__vc_attributes/event_receiverAttribute` | PDB analyzer delta |
| `/test_pdb_universal.pdb/__vc_attributes/event_receiverAttribute *` | PDB analyzer delta |
| `/test_pdb_universal.pdb/__vc_attributes/event_receiverAttribute/!_anon_funcs_/_func___thiscall_event_receiverAttribute_ptr` | PDB analyzer delta |
| `/test_pdb_universal.pdb/__vc_attributes/event_receiverAttribute/!_anon_funcs_/_func___thiscall_event_receiverAttribute_ptr_type_e` | PDB analyzer delta |
| `/test_pdb_universal.pdb/__vc_attributes/event_receiverAttribute/!_anon_funcs_/_func___thiscall_event_receiverAttribute_ptr_type_e_bool` | PDB analyzer delta |
| `/test_pdb_universal.pdb/__vc_attributes/event_receiverAttribute/type_e` | PDB analyzer delta |
| `/test_pdb_universal.pdb/__vc_attributes/event_sourceAttribute` | PDB analyzer delta |
| `/test_pdb_universal.pdb/__vc_attributes/event_sourceAttribute *` | PDB analyzer delta |
| `/test_pdb_universal.pdb/__vc_attributes/event_sourceAttribute/!_anon_funcs_/_func___thiscall_event_sourceAttribute_ptr` | PDB analyzer delta |
| `/test_pdb_universal.pdb/__vc_attributes/event_sourceAttribute/!_anon_funcs_/_func___thiscall_event_sourceAttribute_ptr_type_e` | PDB analyzer delta |
| `/test_pdb_universal.pdb/__vc_attributes/event_sourceAttribute/optimize_e` | PDB analyzer delta |
| `/test_pdb_universal.pdb/__vc_attributes/event_sourceAttribute/type_e` | PDB analyzer delta |
| `/test_pdb_universal.pdb/__vc_attributes/helper_attributes/usageAttribute` | PDB analyzer delta |
| `/test_pdb_universal.pdb/__vc_attributes/helper_attributes/usageAttribute *` | PDB analyzer delta |
| `/test_pdb_universal.pdb/__vc_attributes/helper_attributes/usageAttribute/!_anon_funcs_/_func___thiscall_usageAttribute_ptr_uint` | PDB analyzer delta |
| `/test_pdb_universal.pdb/__vc_attributes/helper_attributes/usageAttribute/usage_e` | PDB analyzer delta |
| `/test_pdb_universal.pdb/__vc_attributes/helper_attributes/v1_alttypeAttribute` | PDB analyzer delta |
| `/test_pdb_universal.pdb/__vc_attributes/helper_attributes/v1_alttypeAttribute *` | PDB analyzer delta |
| `/test_pdb_universal.pdb/__vc_attributes/helper_attributes/v1_alttypeAttribute/!_anon_funcs_/_func___thiscall_v1_alttypeAttribute_ptr_type_e` | PDB analyzer delta |
| `/test_pdb_universal.pdb/__vc_attributes/helper_attributes/v1_alttypeAttribute/type_e` | PDB analyzer delta |
| `/test_pdb_universal.pdb/__vc_attributes/moduleAttribute` | PDB analyzer delta |
| `/test_pdb_universal.pdb/__vc_attributes/moduleAttribute *` | PDB analyzer delta |
| `/test_pdb_universal.pdb/__vc_attributes/moduleAttribute/!_anon_funcs_/_func___thiscall_moduleAttribute_ptr` | PDB analyzer delta |
| `/test_pdb_universal.pdb/__vc_attributes/moduleAttribute/!_anon_funcs_/_func___thiscall_moduleAttribute_ptr_type_e` | PDB analyzer delta |
| `/test_pdb_universal.pdb/__vc_attributes/moduleAttribute/!_anon_funcs_/_func___thiscall_moduleAttribute_ptr_type_e_char_ptr_char_ptr_char_ptr_int_bool_char_ptr_int_char_ptr_char_ptr_int_bool_bool_char_ptr_char_ptr` | PDB analyzer delta |
| `/test_pdb_universal.pdb/__vc_attributes/moduleAttribute/type_e` | PDB analyzer delta |
| `/test_pdb_universal.pdb/__vc_attributes/threadingAttribute` | PDB analyzer delta |
| `/test_pdb_universal.pdb/__vc_attributes/threadingAttribute *` | PDB analyzer delta |
| `/test_pdb_universal.pdb/__vc_attributes/threadingAttribute/!_anon_funcs_/_func___thiscall_threadingAttribute_ptr` | PDB analyzer delta |
| `/test_pdb_universal.pdb/__vc_attributes/threadingAttribute/!_anon_funcs_/_func___thiscall_threadingAttribute_ptr_threading_e` | PDB analyzer delta |
| `/test_pdb_universal.pdb/__vc_attributes/threadingAttribute/threading_e` | PDB analyzer delta |
| `/uint` | PDB analyzer delta |
| `/uint *` | PDB analyzer delta |
| `/ulong64` | PDB analyzer delta |
| `/ulonglong` | PDB analyzer delta |
| `/undefined *` | PDB analyzer delta |
| `/undefined *64` | PE loader/default artifact |
| `/void` | PDB analyzer delta |
| `/word` | PE loader/default artifact |
| `/word[10]` | PE loader/default artifact |
| `/word[4]` | PE loader/default artifact |

## Delta

Rows present in the before snapshot are loader/default artifacts. Only rows below are attributed to PDB Universal.

### Property delta

| Change | Name | Before | After |
| --- | --- | --- | --- |
| Added | `PDB Loaded` | | `true` |

### Symbol delta

| Change | Name | Address | Source |
| --- | --- | --- | --- |
| Added | `.bss` | `0x0000000140003000` | `IMPORTED` |
| Added | `.edata` | `0x000000014000222C` | `IMPORTED` |
| Added | `.pdata` | `0x0000000140004000` | `IMPORTED` |
| Added | `.rdata` | `0x0000000140002000` | `IMPORTED` |
| Added | `.rdata$voltmd` | `0x0000000140002084` | `IMPORTED` |
| Added | `.rdata$zzzdbg` | `0x00000001400020B4` | `IMPORTED` |
| Added | `.text$mn` | `0x0000000140001000` | `IMPORTED` |
| Added | `.xdata` | `0x0000000140002224` | `IMPORTED` |
| Added | `pdb_universal_compute` | `0x0000000140001000` | `IMPORTED` |
| Added | `pdb_universal_entry` | `0x0000000140001050` | `IMPORTED` |
| Added | `pdb_universal_global` | `0x0000000140003000` | `IMPORTED` |

### Function delta

| Change | Name | Entry |
| --- | --- | --- |
| Added | `pdb_universal_compute` | `0x0000000140001000` |
| Added | `pdb_universal_entry` | `0x0000000140001050` |
| Removed | `entry` | `0x0000000140001050` |

### Data type delta

| Change | Path |
| --- | --- |
| Added | `/bool` |
| Added | `/char *` |
| Added | `/char * *` |
| Added | `/char[18]` |
| Added | `/int` |
| Added | `/long64` |
| Added | `/long64 *` |
| Added | `/longlong` |
| Added | `/test_pdb_universal.pdb/!_anon_funcs_/_func___cdecl_long64_PdbUniversalRecord_ptr_PdbUniversalMode` |
| Added | `/test_pdb_universal.pdb/!_anon_funcs_/_func___cdecl_void` |
| Added | `/test_pdb_universal.pdb/PdbUniversalMode` |
| Added | `/test_pdb_universal.pdb/PdbUniversalRecord` |
| Added | `/test_pdb_universal.pdb/PdbUniversalRecord *` |
| Added | `/test_pdb_universal.pdb/__vc_attributes/aggregatableAttribute` |
| Added | `/test_pdb_universal.pdb/__vc_attributes/aggregatableAttribute *` |
| Added | `/test_pdb_universal.pdb/__vc_attributes/aggregatableAttribute/!_anon_funcs_/_func___thiscall_aggregatableAttribute_ptr` |
| Added | `/test_pdb_universal.pdb/__vc_attributes/aggregatableAttribute/!_anon_funcs_/_func___thiscall_aggregatableAttribute_ptr_type_e` |
| Added | `/test_pdb_universal.pdb/__vc_attributes/aggregatableAttribute/type_e` |
| Added | `/test_pdb_universal.pdb/__vc_attributes/event_receiverAttribute` |
| Added | `/test_pdb_universal.pdb/__vc_attributes/event_receiverAttribute *` |
| Added | `/test_pdb_universal.pdb/__vc_attributes/event_receiverAttribute/!_anon_funcs_/_func___thiscall_event_receiverAttribute_ptr` |
| Added | `/test_pdb_universal.pdb/__vc_attributes/event_receiverAttribute/!_anon_funcs_/_func___thiscall_event_receiverAttribute_ptr_type_e` |
| Added | `/test_pdb_universal.pdb/__vc_attributes/event_receiverAttribute/!_anon_funcs_/_func___thiscall_event_receiverAttribute_ptr_type_e_bool` |
| Added | `/test_pdb_universal.pdb/__vc_attributes/event_receiverAttribute/type_e` |
| Added | `/test_pdb_universal.pdb/__vc_attributes/event_sourceAttribute` |
| Added | `/test_pdb_universal.pdb/__vc_attributes/event_sourceAttribute *` |
| Added | `/test_pdb_universal.pdb/__vc_attributes/event_sourceAttribute/!_anon_funcs_/_func___thiscall_event_sourceAttribute_ptr` |
| Added | `/test_pdb_universal.pdb/__vc_attributes/event_sourceAttribute/!_anon_funcs_/_func___thiscall_event_sourceAttribute_ptr_type_e` |
| Added | `/test_pdb_universal.pdb/__vc_attributes/event_sourceAttribute/optimize_e` |
| Added | `/test_pdb_universal.pdb/__vc_attributes/event_sourceAttribute/type_e` |
| Added | `/test_pdb_universal.pdb/__vc_attributes/helper_attributes/usageAttribute` |
| Added | `/test_pdb_universal.pdb/__vc_attributes/helper_attributes/usageAttribute *` |
| Added | `/test_pdb_universal.pdb/__vc_attributes/helper_attributes/usageAttribute/!_anon_funcs_/_func___thiscall_usageAttribute_ptr_uint` |
| Added | `/test_pdb_universal.pdb/__vc_attributes/helper_attributes/usageAttribute/usage_e` |
| Added | `/test_pdb_universal.pdb/__vc_attributes/helper_attributes/v1_alttypeAttribute` |
| Added | `/test_pdb_universal.pdb/__vc_attributes/helper_attributes/v1_alttypeAttribute *` |
| Added | `/test_pdb_universal.pdb/__vc_attributes/helper_attributes/v1_alttypeAttribute/!_anon_funcs_/_func___thiscall_v1_alttypeAttribute_ptr_type_e` |
| Added | `/test_pdb_universal.pdb/__vc_attributes/helper_attributes/v1_alttypeAttribute/type_e` |
| Added | `/test_pdb_universal.pdb/__vc_attributes/moduleAttribute` |
| Added | `/test_pdb_universal.pdb/__vc_attributes/moduleAttribute *` |
| Added | `/test_pdb_universal.pdb/__vc_attributes/moduleAttribute/!_anon_funcs_/_func___thiscall_moduleAttribute_ptr` |
| Added | `/test_pdb_universal.pdb/__vc_attributes/moduleAttribute/!_anon_funcs_/_func___thiscall_moduleAttribute_ptr_type_e` |
| Added | `/test_pdb_universal.pdb/__vc_attributes/moduleAttribute/!_anon_funcs_/_func___thiscall_moduleAttribute_ptr_type_e_char_ptr_char_ptr_char_ptr_int_bool_char_ptr_int_char_ptr_char_ptr_int_bool_bool_char_ptr_char_ptr` |
| Added | `/test_pdb_universal.pdb/__vc_attributes/moduleAttribute/type_e` |
| Added | `/test_pdb_universal.pdb/__vc_attributes/threadingAttribute` |
| Added | `/test_pdb_universal.pdb/__vc_attributes/threadingAttribute *` |
| Added | `/test_pdb_universal.pdb/__vc_attributes/threadingAttribute/!_anon_funcs_/_func___thiscall_threadingAttribute_ptr` |
| Added | `/test_pdb_universal.pdb/__vc_attributes/threadingAttribute/!_anon_funcs_/_func___thiscall_threadingAttribute_ptr_threading_e` |
| Added | `/test_pdb_universal.pdb/__vc_attributes/threadingAttribute/threading_e` |
| Added | `/uint` |
| Added | `/uint *` |
| Added | `/ulong64` |
| Added | `/ulonglong` |
| Added | `/undefined *` |
| Added | `/void` |

### Delta conclusion

- **Exact PDB delta rows:** `70`.
- **PDB matching:** The runner validated PE CodeView identity and PDB age before target analysis.
