# 04_strcmp_variant

- Model: `qwen3-embedding:4b`
- Query example: `04_strcmp_variant`
- Query instruction: `(none; raw source was embedded)`
- Expected function: `strcmp`
- Expected rank: `1`
- Top match: `strcmp` (93.29%)
- Top-1 vs top-2 gap: `5.47` percentage points
- Correct top-1 match: `Yes`

Cosine similarity is multiplied by 100 for presentation; it is not a probability.

| Rank | Database Function   | Similarity |
| ---: | ------------------- | ---------: |
|    1 | strcmp              |      93.29% |
|    2 | memcmp              |      87.82% |
|    3 | first_difference    |      68.07% |
|    4 | memmove             |      67.16% |
|    5 | strlen              |      65.18% |
|    6 | min                 |      64.55% |
|    7 | max                 |      63.32% |
|    8 | strstr              |      63.23% |
|    9 | memcpy              |      59.38% |
|   10 | strchr              |      59.22% |
|   11 | is_sorted           |      59.09% |
|   12 | atoi                |      57.65% |
|   13 | gcd                 |      57.03% |
|   14 | merge_sorted        |      55.05% |
|   15 | partition           |      54.42% |
|   16 | lower_bound         |      54.02% |
|   17 | binary_search       |      53.75% |
|   18 | qsort               |      53.42% |
|   19 | clamp               |      52.85% |
|   20 | unique              |      52.76% |
|   21 | upper_bound         |      52.59% |
|   22 | bsearch             |      52.25% |
|   23 | lcm                 |      52.18% |
|   24 | distance_squared    |      52.04% |
|   25 | abs                 |      50.80% |
|   26 | heap_sort           |      50.67% |
|   27 | linear_search       |      50.14% |
|   28 | remove_value        |      49.72% |
|   29 | is_prime            |      49.22% |
|   30 | count_value         |      49.12% |
|   31 | manhattan_distance  |      48.98% |
|   32 | copy_if             |      48.27% |
|   33 | swap_ranges         |      48.25% |
|   34 | dot_product         |      48.20% |
|   35 | sum_array           |      47.31% |
|   36 | rotate_left         |      46.02% |
|   37 | sqrt                |      45.17% |
|   38 | rotate_right        |      44.49% |
|   39 | fibonacci           |      44.40% |
|   40 | product_array       |      43.38% |
|   41 | next_power_of_two   |      43.31% |
|   42 | iota                |      42.95% |
|   43 | rotate_array        |      42.66% |
|   44 | evaluate_polynomial |      42.27% |
|   45 | linear_interpolate  |      41.68% |
|   46 | pow                 |      41.67% |
|   47 | factorial           |      41.20% |
|   48 | average             |      40.62% |
|   49 | fill                |      40.04% |
|   50 | cos                 |      39.58% |
