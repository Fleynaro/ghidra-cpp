# Apply Data Archives Behavioral Fixture

> Generated automatically with PyGhidra.

## Input

- **File:** `test_apply_data_archives.exe`
- **File size:** `2560` bytes

## Analysis Configuration

- **Enabled boolean analyzers:** `Apply Data Archives`
- **Archive Chooser:** `generic_clib_64.gdt`
- **Create Analysis Bookmarks:** `false`

## Before target analysis

| Name | Signature | Calling convention | Source |
| --- | --- | --- | --- |
| `apply_data_archives_entry` | `undefined apply_data_archives_entry(void)` | `unknown` | `DEFAULT` |

## After target analysis

| Name | Signature | Calling convention | Source |
| --- | --- | --- | --- |
| `apply_data_archives_entry` | `undefined apply_data_archives_entry(void)` | `unknown` | `DEFAULT` |
| `memcpy` | `void * memcpy(void * __dest, void * __src, size_t __n)` | `unknown` | `IMPORTED` |

## Delta

**Added rows**

- `memcpy` | `void * memcpy(void * __dest, void * __src, size_t __n)` | `unknown` | `IMPORTED`

**Removed rows**

- None

**Changed rows**

- None


## Function Signatures Changed

| Name | Signature | Calling convention | Source |
| --- | --- | --- | --- |
| `memcpy` | `void * memcpy(void * __dest, void * __src, size_t __n)` | `unknown` | `IMPORTED` |

- **Changed signature count:** `1`.
- **Archive availability:** `Observed by analyzer run; see changed rows rather than assuming archive contents.`
