# 06_gcd_variant

- Model: `qwen3-embedding:4b`
- Query example: `06_gcd_variant`
- Query instruction: `(none; raw source was embedded)`
- Expected function: `gcd`
- Expected rank: `1`
- Top match: `gcd` (95.95%)
- Top-1 vs top-2 gap: `3.26` percentage points
- Correct top-1 match: `Yes`

Cosine similarity is multiplied by 100 for presentation; it is not a probability.

| Rank | Database Function   | Similarity |
| ---: | ------------------- | ---------: |
|    1 | gcd                 |      95.95% |
|    2 | lcm                 |      92.69% |
|    3 | abs                 |      68.46% |
|    4 | max                 |      67.43% |
|    5 | min                 |      65.22% |
|    6 | manhattan_distance  |      61.38% |
|    7 | pow                 |      56.01% |
|    8 | clamp               |      54.60% |
|    9 | sqrt                |      54.26% |
|   10 | rotate_array        |      54.17% |
|   11 | first_difference    |      53.29% |
|   12 | is_prime            |      52.82% |
|   13 | product_array       |      52.63% |
|   14 | fibonacci           |      52.44% |
|   15 | merge_sorted        |      52.05% |
|   16 | strcmp              |      51.87% |
|   17 | next_power_of_two   |      51.68% |
|   18 | distance_squared    |      51.51% |
|   19 | dot_product         |      50.83% |
|   20 | unique              |      50.65% |
|   21 | sum_array           |      50.60% |
|   22 | heap_sort           |      50.50% |
|   23 | memcmp              |      49.13% |
|   24 | qsort               |      48.45% |
|   25 | factorial           |      48.26% |
|   26 | rotate_right        |      48.07% |
|   27 | rotate_left         |      48.06% |
|   28 | lower_bound         |      47.95% |
|   29 | upper_bound         |      47.55% |
|   30 | atoi                |      47.51% |
|   31 | memmove             |      47.28% |
|   32 | swap_ranges         |      46.65% |
|   33 | is_sorted           |      46.46% |
|   34 | average             |      46.36% |
|   35 | cos                 |      45.80% |
|   36 | bsearch             |      45.79% |
|   37 | memcpy              |      45.03% |
|   38 | binary_search       |      44.99% |
|   39 | partition           |      44.92% |
|   40 | copy_if             |      44.91% |
|   41 | evaluate_polynomial |      44.56% |
|   42 | linear_search       |      43.53% |
|   43 | strlen              |      42.92% |
|   44 | linear_interpolate  |      41.70% |
|   45 | remove_value        |      41.24% |
|   46 | strstr              |      41.20% |
|   47 | iota                |      41.04% |
|   48 | count_value         |      39.93% |
|   49 | fill                |      38.51% |
|   50 | strchr              |      33.80% |
