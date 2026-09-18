# 02_qsort_variant

- Model: `qwen3-embedding:4b`
- Query example: `02_qsort_variant`
- Query instruction: `(none; raw source was embedded)`
- Expected function: `qsort`
- Expected rank: `2`
- Top match: `heap_sort` (80.86%)
- Top-1 vs top-2 gap: `2.58` percentage points
- Correct top-1 match: `No`

Cosine similarity is multiplied by 100 for presentation; it is not a probability.

| Rank | Database Function   | Similarity |
| ---: | ------------------- | ---------: |
|    1 | heap_sort           |      80.86% |
|    2 | qsort               |      78.28% |
|    3 | partition           |      77.83% |
|    4 | is_sorted           |      76.00% |
|    5 | unique              |      71.71% |
|    6 | iota                |      67.87% |
|    7 | lower_bound         |      66.69% |
|    8 | binary_search       |      65.72% |
|    9 | upper_bound         |      65.56% |
|   10 | copy_if             |      64.62% |
|   11 | fill                |      63.60% |
|   12 | swap_ranges         |      63.50% |
|   13 | rotate_array        |      62.50% |
|   14 | remove_value        |      62.45% |
|   15 | merge_sorted        |      61.86% |
|   16 | bsearch             |      61.69% |
|   17 | sum_array           |      60.22% |
|   18 | memmove             |      59.75% |
|   19 | product_array       |      58.01% |
|   20 | min                 |      57.87% |
|   21 | memcpy              |      57.57% |
|   22 | linear_search       |      57.16% |
|   23 | max                 |      56.99% |
|   24 | next_power_of_two   |      56.41% |
|   25 | memcmp              |      56.16% |
|   26 | is_prime            |      54.92% |
|   27 | fibonacci           |      54.67% |
|   28 | evaluate_polynomial |      53.64% |
|   29 | first_difference    |      53.06% |
|   30 | gcd                 |      53.01% |
|   31 | count_value         |      52.59% |
|   32 | factorial           |      52.29% |
|   33 | clamp               |      52.24% |
|   34 | sqrt                |      49.84% |
|   35 | average             |      49.64% |
|   36 | rotate_left         |      49.64% |
|   37 | strcmp              |      48.57% |
|   38 | rotate_right        |      48.05% |
|   39 | pow                 |      47.96% |
|   40 | abs                 |      47.90% |
|   41 | dot_product         |      47.36% |
|   42 | lcm                 |      46.79% |
|   43 | cos                 |      46.10% |
|   44 | distance_squared    |      45.59% |
|   45 | linear_interpolate  |      44.46% |
|   46 | strlen              |      44.18% |
|   47 | atoi                |      44.01% |
|   48 | strstr              |      42.50% |
|   49 | manhattan_distance  |      41.22% |
|   50 | strchr              |      39.14% |
