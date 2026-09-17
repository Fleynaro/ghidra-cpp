# BSim Differential Reference

> Generated automatically from the existing PE fixture through PyGhidra.
> Raw feature hashes and complete vectors are intentionally omitted.

## Input

- **File:** `bsim_fixture.exe`
- **File size:** `23040` bytes
- **Language:** `x86:LE:64:default`
- **Signature settings:** `0x49`
- **Weights:** `lshweights_64.xml`

## Selected Functions

| Function | Entry address | Body end | Feature count |
| --- | ---: | ---: | ---: |
| `sort_insertion_ascending` | `0x0000000140001000` | `0x00000001400010AE` | 47 |
| `sort_selection_ascending` | `0x00000001400010C0` | `0x000000014000119B` | 55 |
| `find_first_linear` | `0x00000001400011B0` | `0x0000000140001214` | 25 |
| `find_first_reverse` | `0x0000000140001220` | `0x000000014000128C` | 25 |
| `maximum_scan` | `0x00000001400012A0` | `0x000000014000131B` | 32 |
| `maximum_pairwise` | `0x0000000140001330` | `0x0000000140001418` | 64 |
| `absolute_branch` | `0x0000000140001420` | `0x0000000140001461` | 12 |
| `absolute_mask` | `0x0000000140001470` | `0x00000001400014BF` | 12 |
| `checksum_forward` | `0x00000001400014D0` | `0x0000000140001535` | 23 |
| `checksum_reverse` | `0x0000000140001540` | `0x00000001400015A4` | 23 |
| `hash_fnv_indexed` | `0x00000001400015B0` | `0x0000000140001623` | 24 |
| `hash_fnv_pointer` | `0x0000000140001630` | `0x00000001400016B7` | 24 |
| `population_count` | `0x00000001400019A0` | `0x00000001400019D9` | 12 |
| `rotate_left32` | `0x0000000140001950` | `0x0000000140001994` | 14 |
| `gcd_unsigned` | `0x0000000140001A70` | `0x0000000140001AAB` | 10 |
| `byte_swap32` | `0x0000000140001BE0` | `0x0000000140001C1D` | 11 |
| `dot_product` | `0x00000001400018B0` | `0x000000014000193A` | 33 |

## Pairwise Cosine

| Function A | Function B | Ghidra Cosine |
| --- | --- | ---: |
| `sort_insertion_ascending` | `sort_selection_ascending` | 0.468167012437 |
| `find_first_linear` | `find_first_reverse` | 0.368269612427 |
| `maximum_scan` | `maximum_pairwise` | 0.506475504474 |
| `absolute_branch` | `absolute_mask` | 0.278411266547 |
| `checksum_forward` | `checksum_reverse` | 0.534143648735 |
| `hash_fnv_indexed` | `hash_fnv_pointer` | 0.463203124201 |
| `sort_insertion_ascending` | `population_count` | 0.068833330995 |
| `find_first_linear` | `rotate_left32` | 0.032044763447 |
| `maximum_scan` | `byte_swap32` | 0.000000000000 |
| `absolute_branch` | `dot_product` | 0.117323068846 |
| `checksum_forward` | `gcd_unsigned` | 0.000000000000 |
| `hash_fnv_indexed` | `population_count` | 0.097416143659 |
