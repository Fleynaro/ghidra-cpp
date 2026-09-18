# 06_gcd_variant

- Model: `hf.co/jinaai/jina-code-embeddings-1.5b-GGUF:BF16`
- Query example: `06_gcd_variant`
- Query instruction: `Find an equivalent code snippet given the following code snippet:`
- Expected function: `gcd`
- Expected rank: `1`
- Top match: `gcd` (89.82%)
- Top-1 vs top-2 gap: `6.65` percentage points
- Correct top-1 match: `Yes`

Cosine similarity is multiplied by 100 for presentation; it is not a probability.

| Rank | Database Function   | Similarity |
| ---: | ------------------- | ---------: |
|    1 | gcd                 |      89.82% |
|    2 | lcm                 |      83.18% |
|    3 | abs                 |      72.73% |
|    4 | max                 |      60.68% |
|    5 | min                 |      60.15% |
|    6 | manhattan_distance  |      57.31% |
|    7 | pow                 |      56.80% |
|    8 | strcmp              |      55.29% |
|    9 | clamp               |      55.27% |
|   10 | atoi                |      54.19% |
|   11 | sqrt                |      53.86% |
|   12 | memcmp              |      50.92% |
|   13 | first_difference    |      49.98% |
|   14 | is_prime            |      48.93% |
|   15 | next_power_of_two   |      48.42% |
|   16 | linear_interpolate  |      48.36% |
|   17 | strlen              |      48.33% |
|   18 | rotate_right        |      48.17% |
|   19 | rotate_array        |      48.13% |
|   20 | unique              |      47.24% |
|   21 | upper_bound         |      46.57% |
|   22 | rotate_left         |      46.53% |
|   23 | strstr              |      45.95% |
|   24 | average             |      45.19% |
|   25 | lower_bound         |      44.38% |
|   26 | memmove             |      44.02% |
|   27 | remove_value        |      43.97% |
|   28 | linear_search       |      43.90% |
|   29 | iota                |      43.47% |
|   30 | fill                |      43.08% |
|   31 | bsearch             |      42.65% |
|   32 | factorial           |      42.20% |
|   33 | strchr              |      42.14% |
|   34 | swap_ranges         |      41.95% |
|   35 | distance_squared    |      41.74% |
|   36 | heap_sort           |      40.45% |
|   37 | count_value         |      40.41% |
|   38 | memcpy              |      40.39% |
|   39 | merge_sorted        |      39.62% |
|   40 | binary_search       |      39.09% |
|   41 | cos                 |      39.04% |
|   42 | partition           |      38.98% |
|   43 | sum_array           |      38.81% |
|   44 | fibonacci           |      38.44% |
|   45 | qsort               |      37.63% |
|   46 | copy_if             |      36.22% |
|   47 | evaluate_polynomial |      35.60% |
|   48 | product_array       |      34.27% |
|   49 | is_sorted           |      31.39% |
|   50 | dot_product         |      27.97% |
