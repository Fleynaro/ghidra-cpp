# 05_memmove_variant

- Model: `qwen3-embedding:4b`
- Query example: `05_memmove_variant`
- Query instruction: `(none; raw source was embedded)`
- Expected function: `memmove`
- Expected rank: `1`
- Top match: `memmove` (97.02%)
- Top-1 vs top-2 gap: `4.92` percentage points
- Correct top-1 match: `Yes`

Cosine similarity is multiplied by 100 for presentation; it is not a probability.

| Rank | Database Function   | Similarity |
| ---: | ------------------- | ---------: |
|    1 | memmove             |      97.02% |
|    2 | memcpy              |      92.09% |
|    3 | memcmp              |      78.31% |
|    4 | strcmp              |      68.65% |
|    5 | swap_ranges         |      66.13% |
|    6 | merge_sorted        |      63.84% |
|    7 | first_difference    |      62.74% |
|    8 | fill                |      62.56% |
|    9 | strstr              |      60.81% |
|   10 | iota                |      60.71% |
|   11 | remove_value        |      58.38% |
|   12 | rotate_array        |      58.23% |
|   13 | copy_if             |      58.13% |
|   14 | unique              |      57.24% |
|   15 | clamp               |      57.11% |
|   16 | min                 |      57.05% |
|   17 | partition           |      55.98% |
|   18 | qsort               |      55.60% |
|   19 | lower_bound         |      54.78% |
|   20 | max                 |      54.70% |
|   21 | binary_search       |      54.32% |
|   22 | rotate_left         |      54.22% |
|   23 | upper_bound         |      54.01% |
|   24 | strchr              |      53.14% |
|   25 | rotate_right        |      52.90% |
|   26 | bsearch             |      51.65% |
|   27 | dot_product         |      51.31% |
|   28 | gcd                 |      51.11% |
|   29 | heap_sort           |      51.10% |
|   30 | evaluate_polynomial |      50.74% |
|   31 | count_value         |      50.56% |
|   32 | sum_array           |      50.45% |
|   33 | linear_search       |      50.11% |
|   34 | distance_squared    |      49.31% |
|   35 | strlen              |      49.06% |
|   36 | is_sorted           |      48.40% |
|   37 | lcm                 |      48.20% |
|   38 | manhattan_distance  |      46.88% |
|   39 | abs                 |      46.68% |
|   40 | next_power_of_two   |      46.53% |
|   41 | linear_interpolate  |      46.30% |
|   42 | average             |      45.11% |
|   43 | fibonacci           |      44.80% |
|   44 | product_array       |      44.40% |
|   45 | sqrt                |      44.35% |
|   46 | is_prime            |      44.15% |
|   47 | pow                 |      43.65% |
|   48 | atoi                |      43.23% |
|   49 | cos                 |      40.46% |
|   50 | factorial           |      38.36% |
