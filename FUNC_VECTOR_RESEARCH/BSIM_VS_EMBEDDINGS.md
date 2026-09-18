# BSim Versus Source Embeddings

This comparison uses the existing research dataset: 50 database functions and
10 input variants. Qwen3 and Jina embed anonymized C++ source text, while BSim
opens the separately compiled `database.exe` and `input.exe` files and compares
Ghidra x64 signatures with `0x49` settings and `lshweights_64.xml`.

## Retrieval Metrics

| Metric | Qwen3 Embedding | Jina Code Embedding | Ghidra BSim |
| --- | ---: | ---: | ---: |
| Positive Top-1 | 6/7 | 6/7 | 3/7 |
| Positive Top-3 | 7/7 | 7/7 | 4/7 |
| Average positive expected rank | 1.14 | 1.14 | 7.00 |
| Average positive Top-1 similarity | 93.81% | 93.18% | 50.02% |
| Unknown Top-1 range | 74.31-85.27% | 80.25-89.32% | 52.04-76.62% |

## Per-Input Top Matches

| Input | Expected | Qwen3 | Jina | BSim |
| --- | --- | --- | --- | --- |
| `01_bsearch_variant` | `bsearch` | `bsearch` | `bsearch` | `binary_search` |
| `02_qsort_variant` | `qsort` | `heap_sort` | `qsort` | `swap_ranges` |
| `03_strlen_variant` | `strlen` | `strlen` | `strlen` | `factorial` |
| `04_strcmp_variant` | `strcmp` | `strcmp` | `memcmp` | `atoi` |
| `05_memmove_variant` | `memmove` | `memmove` | `memmove` | `memmove` |
| `06_gcd_variant` | `gcd` | `gcd` | `gcd` | `gcd` |
| `07_clamp_variant` | `clamp` | `clamp` | `clamp` | `clamp` |
| `08_NONE_prefix_sum` | `NONE` | `sum_array` | `iota` | `swap_ranges` |
| `09_NONE_count_bits` | `NONE` | `next_power_of_two` | `next_power_of_two` | `strlen` |
| `10_NONE_reverse` | `NONE` | `swap_ranges` | `swap_ranges` | `swap_ranges` |

## Interpretation

- On this experiment, source embeddings are materially better for identifying
  semantic or algorithmic variants. Both neural models recover six of seven
  known functions at Top-1 and all seven in Top-3.
- BSim is useful when the compiled implementation remains close: it correctly
  identifies `memmove`, `gcd`, and `clamp`. It also recognizes the binary-search
  family, but ranks `binary_search` above the expected `bsearch` implementation.
- BSim is not robust to source-level implementation changes in this dataset:
  `qsort`, `strlen`, and `strcmp` miss badly despite their source-level intent.
- BSim has fewer extreme unknown scores than either neural model, but the score
  distributions overlap heavily: the lowest known positive is `28.61%`, while
  the strongest unknown is `76.62%`. A single rejection threshold therefore
  cannot separate known and unknown cases reliably here.
- The database pair report also exposes strong low-level collisions, including
  `max` versus `min` at `1.000000` and `copy_if` versus `remove_value` at
  `0.921733`. These are machine-code/control-flow similarities, not semantic
  identity guarantees.

The result is dataset-specific. BSim and source embeddings measure different
objects, so this is a retrieval comparison rather than a claim that one vector
space universally dominates the other.
