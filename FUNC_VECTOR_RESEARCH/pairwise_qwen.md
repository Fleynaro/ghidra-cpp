# Pairwise Similarity: qwen

## Experiment Metadata

- Model: `qwen3-embedding:4b`
- Embedding database: `embeddings_qwen3-embedding-4b.json`
- Random seed: `20260915`
- Selected functions: `10`
- Embedding dimension: `2560`
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
| sqrt | 1.0000 | 0.4980 | 0.4508 | 0.4116 | 0.5491 | 0.4982 | 0.5463 | 0.4375 | 0.3837 | 0.4245 |
| qsort | 0.4980 | 1.0000 | 0.3706 | 0.5394 | 0.5755 | 0.5595 | 0.4664 | 0.4090 | 0.3866 | 0.6179 |
| strlen | 0.4508 | 0.3706 | 1.0000 | 0.4497 | 0.5113 | 0.5633 | 0.4181 | 0.6081 | 0.3893 | 0.4690 |
| copy_if | 0.4116 | 0.5394 | 0.4497 | 1.0000 | 0.4094 | 0.5570 | 0.4565 | 0.3973 | 0.4504 | 0.6103 |
| fibonacci | 0.5491 | 0.5755 | 0.5113 | 0.4094 | 1.0000 | 0.6882 | 0.5140 | 0.4789 | 0.4270 | 0.5793 |
| sum_array | 0.4982 | 0.5595 | 0.5633 | 0.5570 | 0.6882 | 1.0000 | 0.5320 | 0.5358 | 0.6070 | 0.7138 |
| lcm | 0.5463 | 0.4664 | 0.4181 | 0.4565 | 0.5140 | 0.5320 | 1.0000 | 0.4941 | 0.5961 | 0.4153 |
| atoi | 0.4375 | 0.4090 | 0.6081 | 0.3973 | 0.4789 | 0.5358 | 0.4941 | 1.0000 | 0.4336 | 0.4848 |
| manhattan_distance | 0.3837 | 0.3866 | 0.3893 | 0.4504 | 0.4270 | 0.6070 | 0.5961 | 0.4336 | 1.0000 | 0.4660 |
| iota | 0.4245 | 0.6179 | 0.4690 | 0.6103 | 0.5793 | 0.7138 | 0.4153 | 0.4848 | 0.4660 | 1.0000 |

Diagonal entries are exactly `1.0000`; each off-diagonal pair represents two distinct selected functions.

## Sorted Pair List

| Rank | Function A | Function B | Cosine Similarity |
| ---: | ---------- | ---------- | ----------------: |
|    1 | sum_array | iota |         0.713823 |
|    2 | fibonacci | sum_array |         0.688171 |
|    3 | qsort | iota |         0.617909 |
|    4 | copy_if | iota |         0.610269 |
|    5 | strlen | atoi |         0.608094 |
|    6 | sum_array | manhattan_distance |         0.607019 |
|    7 | lcm | manhattan_distance |         0.596094 |
|    8 | fibonacci | iota |         0.579263 |
|    9 | qsort | fibonacci |         0.575522 |
|   10 | strlen | sum_array |         0.563290 |
|   11 | qsort | sum_array |         0.559481 |
|   12 | copy_if | sum_array |         0.557001 |
|   13 | sqrt | fibonacci |         0.549051 |
|   14 | sqrt | lcm |         0.546348 |
|   15 | qsort | copy_if |         0.539449 |
|   16 | sum_array | atoi |         0.535846 |
|   17 | sum_array | lcm |         0.531998 |
|   18 | fibonacci | lcm |         0.514043 |
|   19 | strlen | fibonacci |         0.511325 |
|   20 | sqrt | sum_array |         0.498232 |
|   21 | sqrt | qsort |         0.497997 |
|   22 | lcm | atoi |         0.494063 |
|   23 | atoi | iota |         0.484846 |
|   24 | fibonacci | atoi |         0.478866 |
|   25 | strlen | iota |         0.469011 |
|   26 | qsort | lcm |         0.466434 |
|   27 | manhattan_distance | iota |         0.466010 |
|   28 | copy_if | lcm |         0.456550 |
|   29 | sqrt | strlen |         0.450840 |
|   30 | copy_if | manhattan_distance |         0.450440 |
|   31 | strlen | copy_if |         0.449720 |
|   32 | sqrt | atoi |         0.437464 |
|   33 | atoi | manhattan_distance |         0.433561 |
|   34 | fibonacci | manhattan_distance |         0.427042 |
|   35 | sqrt | iota |         0.424485 |
|   36 | strlen | lcm |         0.418078 |
|   37 | lcm | iota |         0.415335 |
|   38 | sqrt | copy_if |         0.411551 |
|   39 | copy_if | fibonacci |         0.409393 |
|   40 | qsort | atoi |         0.408963 |
|   41 | copy_if | atoi |         0.397301 |
|   42 | strlen | manhattan_distance |         0.389308 |
|   43 | qsort | manhattan_distance |         0.386613 |
|   44 | sqrt | manhattan_distance |         0.383726 |
|   45 | qsort | strlen |         0.370627 |

## Basic Statistics

- Maximum off-diagonal similarity: `0.713823`
- Minimum off-diagonal similarity: `0.370627`
- Mean pairwise similarity: `0.497343`
- Median pairwise similarity: `0.484846`
- Population standard deviation: `0.082661`

### Top 5 Most Similar Distinct-Function Pairs

| Rank | Function A | Function B | Cosine Similarity |
| ---: | ---------- | ---------- | ----------------: |
|    1 | sum_array | iota |         0.713823 |
|    2 | fibonacci | sum_array |         0.688171 |
|    3 | qsort | iota |         0.617909 |
|    4 | copy_if | iota |         0.610269 |
|    5 | strlen | atoi |         0.608094 |

No expected labels, input functions, thresholds, or handcrafted classifications are used in this analysis.
