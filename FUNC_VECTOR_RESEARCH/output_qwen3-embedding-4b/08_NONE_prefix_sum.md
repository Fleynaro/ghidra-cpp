# 08_NONE_prefix_sum

- Model: `qwen3-embedding:4b`
- Query example: `08_NONE_prefix_sum`
- Query instruction: `(none; raw source was embedded)`
- Expected function: `NONE`
- Expected rank: `N/A`
- Top match: `sum_array` (84.05%)
- Top-1 vs top-2 gap: `2.81` percentage points
- Correct top-1 match: `No`

Cosine similarity is multiplied by 100 for presentation; it is not a probability.

| Rank | Database Function   | Similarity |
| ---: | ------------------- | ---------: |
|    1 | sum_array           |      84.05% |
|    2 | iota                |      81.24% |
|    3 | fill                |      69.89% |
|    4 | evaluate_polynomial |      68.22% |
|    5 | average             |      65.96% |
|    6 | unique              |      64.11% |
|    7 | swap_ranges         |      62.39% |
|    8 | product_array       |      62.12% |
|    9 | dot_product         |      60.78% |
|   10 | memcpy              |      60.58% |
|   11 | fibonacci           |      60.23% |
|   12 | rotate_array        |      59.01% |
|   13 | is_sorted           |      58.36% |
|   14 | partition           |      56.75% |
|   15 | qsort               |      55.76% |
|   16 | next_power_of_two   |      55.45% |
|   17 | heap_sort           |      55.25% |
|   18 | count_value         |      54.89% |
|   19 | memmove             |      54.68% |
|   20 | merge_sorted        |      54.48% |
|   21 | copy_if             |      53.80% |
|   22 | remove_value        |      53.74% |
|   23 | factorial           |      53.47% |
|   24 | linear_search       |      52.60% |
|   25 | lower_bound         |      52.57% |
|   26 | distance_squared    |      52.32% |
|   27 | cos                 |      51.95% |
|   28 | binary_search       |      51.29% |
|   29 | upper_bound         |      50.28% |
|   30 | strlen              |      49.89% |
|   31 | first_difference    |      49.73% |
|   32 | is_prime            |      49.58% |
|   33 | manhattan_distance  |      48.86% |
|   34 | pow                 |      48.74% |
|   35 | bsearch             |      48.66% |
|   36 | rotate_left         |      48.35% |
|   37 | abs                 |      47.60% |
|   38 | gcd                 |      47.49% |
|   39 | atoi                |      47.31% |
|   40 | min                 |      46.79% |
|   41 | sqrt                |      46.61% |
|   42 | max                 |      46.54% |
|   43 | rotate_right        |      46.43% |
|   44 | memcmp              |      46.26% |
|   45 | lcm                 |      43.89% |
|   46 | linear_interpolate  |      43.64% |
|   47 | strcmp              |      41.24% |
|   48 | strchr              |      40.91% |
|   49 | clamp               |      39.08% |
|   50 | strstr              |      38.41% |
