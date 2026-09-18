# Pairwise Similarity Comparison

Random seed: `20260915`
Selected functions: `10`
Pairwise comparisons per model: `45`

The same fixed-seed function selection is used for both models. Values are raw cosine similarities; no threshold or model-specific transformation is applied.

## Summary Metrics

| Metric | Qwen3-Embedding:4B | Jina Code Embeddings 1.5B |
| ------------------ | -----------------: | ------------------------: |
| Mean similarity | 0.497343 | 0.523898 |
| Median similarity | 0.484846 | 0.495334 |
| Maximum similarity | 0.713823 | 0.713967 |
| Minimum similarity | 0.370627 | 0.376046 |

## Top 10 Pairs: Qwen3-Embedding:4B

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

## Top 10 Pairs: Jina Code Embeddings 1.5B

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
