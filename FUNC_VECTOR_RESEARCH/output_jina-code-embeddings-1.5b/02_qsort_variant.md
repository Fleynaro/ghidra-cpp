# 02_qsort_variant

- Model: `hf.co/jinaai/jina-code-embeddings-1.5b-GGUF:BF16`
- Query example: `02_qsort_variant`
- Query instruction: `Find an equivalent code snippet given the following code snippet:`
- Expected function: `qsort`
- Expected rank: `1`
- Top match: `qsort` (91.68%)
- Top-1 vs top-2 gap: `3.65` percentage points
- Correct top-1 match: `Yes`

Cosine similarity is multiplied by 100 for presentation; it is not a probability.

| Rank | Database Function   | Similarity |
| ---: | ------------------- | ---------: |
|    1 | qsort               |      91.68% |
|    2 | heap_sort           |      88.04% |
|    3 | partition           |      81.55% |
|    4 | merge_sorted        |      80.72% |
|    5 | fill                |      80.52% |
|    6 | iota                |      78.66% |
|    7 | swap_ranges         |      77.91% |
|    8 | rotate_array        |      76.26% |
|    9 | remove_value        |      70.27% |
|   10 | copy_if             |      68.13% |
|   11 | memmove             |      67.75% |
|   12 | memcpy              |      66.55% |
|   13 | is_sorted           |      63.71% |
|   14 | lower_bound         |      63.25% |
|   15 | upper_bound         |      63.06% |
|   16 | unique              |      61.13% |
|   17 | min                 |      60.26% |
|   18 | binary_search       |      60.02% |
|   19 | max                 |      56.57% |
|   20 | count_value         |      56.19% |
|   21 | sum_array           |      55.58% |
|   22 | bsearch             |      54.94% |
|   23 | evaluate_polynomial |      54.69% |
|   24 | linear_search       |      54.67% |
|   25 | clamp               |      52.68% |
|   26 | product_array       |      51.92% |
|   27 | factorial           |      50.25% |
|   28 | pow                 |      49.26% |
|   29 | gcd                 |      48.18% |
|   30 | strlen              |      47.70% |
|   31 | abs                 |      47.65% |
|   32 | is_prime            |      46.96% |
|   33 | strchr              |      46.83% |
|   34 | fibonacci           |      46.58% |
|   35 | average             |      46.31% |
|   36 | next_power_of_two   |      45.37% |
|   37 | linear_interpolate  |      45.27% |
|   38 | first_difference    |      45.23% |
|   39 | strstr              |      44.02% |
|   40 | memcmp              |      43.97% |
|   41 | atoi                |      40.71% |
|   42 | lcm                 |      40.51% |
|   43 | sqrt                |      40.35% |
|   44 | dot_product         |      40.06% |
|   45 | rotate_right        |      38.90% |
|   46 | rotate_left         |      38.75% |
|   47 | cos                 |      37.57% |
|   48 | manhattan_distance  |      36.13% |
|   49 | strcmp              |      33.96% |
|   50 | distance_squared    |      32.34% |
