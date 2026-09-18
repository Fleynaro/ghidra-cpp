# Pairwise Similarity: jina

## Experiment Metadata

- Model: `hf.co/jinaai/jina-code-embeddings-1.5b-GGUF:BF16`
- Embedding database: `embeddings_jina-code-embeddings-1.5b.json`
- Random seed: `20260915`
- Selected functions: `10`
- Embedding dimension: `1536`
- Pairwise comparisons: `45`

## Selected Functions

|   # | Function |
| --: | -------- |
|   1 | sqrt |
|   2 | qsort |
|   3 | strlen |
|   4 | copy_if |
|   5 | fibonacci |
|   6 | sum_array |
|   7 | lcm |
|   8 | atoi |
|   9 | manhattan_distance |
|  10 | iota |

## Pairwise Similarity Matrix

| Function | sqrt | qsort | strlen | copy_if | fibonacci | sum_array | lcm | atoi | manhattan_distance | iota |
| -------- | ------: | ------: | ------: | ------: | ------: | ------: | ------: | ------: | ------: | ------: |
| sqrt | 1.0000 | 0.4124 | 0.4844 | 0.4114 | 0.5352 | 0.4714 | 0.5717 | 0.4701 | 0.4473 | 0.4938 |
| qsort | 0.4124 | 1.0000 | 0.4507 | 0.6666 | 0.4475 | 0.5252 | 0.4176 | 0.3891 | 0.3760 | 0.7078 |
| strlen | 0.4844 | 0.4507 | 1.0000 | 0.6160 | 0.4980 | 0.6222 | 0.5368 | 0.6550 | 0.4750 | 0.6391 |
| copy_if | 0.4114 | 0.6666 | 0.6160 | 1.0000 | 0.4143 | 0.6294 | 0.4325 | 0.4934 | 0.4683 | 0.6819 |
| fibonacci | 0.5352 | 0.4475 | 0.4980 | 0.4143 | 1.0000 | 0.6498 | 0.4631 | 0.4268 | 0.4394 | 0.5395 |
| sum_array | 0.4714 | 0.5252 | 0.6222 | 0.6294 | 0.6498 | 1.0000 | 0.5091 | 0.5341 | 0.6467 | 0.7140 |
| lcm | 0.5717 | 0.4176 | 0.5368 | 0.4325 | 0.4631 | 0.5091 | 1.0000 | 0.6010 | 0.6261 | 0.4953 |
| atoi | 0.4701 | 0.3891 | 0.6550 | 0.4934 | 0.4268 | 0.5341 | 0.6010 | 1.0000 | 0.4884 | 0.5181 |
| manhattan_distance | 0.4473 | 0.3760 | 0.4750 | 0.4683 | 0.4394 | 0.6467 | 0.6261 | 0.4884 | 1.0000 | 0.4837 |
| iota | 0.4938 | 0.7078 | 0.6391 | 0.6819 | 0.5395 | 0.7140 | 0.4953 | 0.5181 | 0.4837 | 1.0000 |

Diagonal entries are exactly `1.0000`; each off-diagonal pair represents two distinct selected functions.

## Sorted Pair List

| Rank | Function A | Function B | Cosine Similarity |
| ---: | ---------- | ---------- | ----------------: |
|    1 | sum_array | iota |         0.713967 |
|    2 | qsort | iota |         0.707820 |
|    3 | copy_if | iota |         0.681937 |
|    4 | qsort | copy_if |         0.666580 |
|    5 | strlen | atoi |         0.655031 |
|    6 | fibonacci | sum_array |         0.649788 |
|    7 | sum_array | manhattan_distance |         0.646748 |
|    8 | strlen | iota |         0.639149 |
|    9 | copy_if | sum_array |         0.629384 |
|   10 | lcm | manhattan_distance |         0.626145 |
|   11 | strlen | sum_array |         0.622199 |
|   12 | strlen | copy_if |         0.615975 |
|   13 | lcm | atoi |         0.600968 |
|   14 | sqrt | lcm |         0.571741 |
|   15 | fibonacci | iota |         0.539533 |
|   16 | strlen | lcm |         0.536786 |
|   17 | sqrt | fibonacci |         0.535185 |
|   18 | sum_array | atoi |         0.534081 |
|   19 | qsort | sum_array |         0.525158 |
|   20 | atoi | iota |         0.518113 |
|   21 | sum_array | lcm |         0.509139 |
|   22 | strlen | fibonacci |         0.498037 |
|   23 | lcm | iota |         0.495334 |
|   24 | sqrt | iota |         0.493756 |
|   25 | copy_if | atoi |         0.493383 |
|   26 | atoi | manhattan_distance |         0.488403 |
|   27 | sqrt | strlen |         0.484417 |
|   28 | manhattan_distance | iota |         0.483691 |
|   29 | strlen | manhattan_distance |         0.474954 |
|   30 | sqrt | sum_array |         0.471408 |
|   31 | sqrt | atoi |         0.470059 |
|   32 | copy_if | manhattan_distance |         0.468335 |
|   33 | fibonacci | lcm |         0.463123 |
|   34 | qsort | strlen |         0.450675 |
|   35 | qsort | fibonacci |         0.447499 |
|   36 | sqrt | manhattan_distance |         0.447305 |
|   37 | fibonacci | manhattan_distance |         0.439443 |
|   38 | copy_if | lcm |         0.432496 |
|   39 | fibonacci | atoi |         0.426796 |
|   40 | qsort | lcm |         0.417613 |
|   41 | copy_if | fibonacci |         0.414254 |
|   42 | sqrt | qsort |         0.412436 |
|   43 | sqrt | copy_if |         0.411443 |
|   44 | qsort | atoi |         0.389079 |
|   45 | qsort | manhattan_distance |         0.376046 |

## Basic Statistics

- Maximum off-diagonal similarity: `0.713967`
- Minimum off-diagonal similarity: `0.376046`
- Mean pairwise similarity: `0.523898`
- Median pairwise similarity: `0.495334`
- Population standard deviation: `0.091718`

### Top 5 Most Similar Distinct-Function Pairs

| Rank | Function A | Function B | Cosine Similarity |
| ---: | ---------- | ---------- | ----------------: |
|    1 | sum_array | iota |         0.713967 |
|    2 | qsort | iota |         0.707820 |
|    3 | copy_if | iota |         0.681937 |
|    4 | qsort | copy_if |         0.666580 |
|    5 | strlen | atoi |         0.655031 |

No expected labels, input functions, thresholds, or handcrafted classifications are used in this analysis.
