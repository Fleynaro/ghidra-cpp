# 10_NONE_reverse

- Model: `qwen3-embedding:4b`
- Query example: `10_NONE_reverse`
- Query instruction: `(none; raw source was embedded)`
- Expected function: `NONE`
- Expected rank: `N/A`
- Top match: `swap_ranges` (85.27%)
- Top-1 vs top-2 gap: `6.70` percentage points
- Correct top-1 match: `No`

Cosine similarity is multiplied by 100 for presentation; it is not a probability.

| Rank | Database Function   | Similarity |
| ---: | ------------------- | ---------: |
|    1 | swap_ranges         |      85.27% |
|    2 | rotate_array        |      78.57% |
|    3 | qsort               |      75.01% |
|    4 | heap_sort           |      71.45% |
|    5 | iota                |      65.30% |
|    6 | fill                |      62.86% |
|    7 | partition           |      61.63% |
|    8 | rotate_right        |      61.35% |
|    9 | memmove             |      60.26% |
|   10 | is_sorted           |      60.11% |
|   11 | rotate_left         |      59.68% |
|   12 | memcpy              |      59.47% |
|   13 | sum_array           |      58.63% |
|   14 | product_array       |      57.91% |
|   15 | bsearch             |      57.39% |
|   16 | evaluate_polynomial |      56.69% |
|   17 | unique              |      56.55% |
|   18 | fibonacci           |      55.65% |
|   19 | gcd                 |      55.41% |
|   20 | lower_bound         |      54.98% |
|   21 | pow                 |      54.07% |
|   22 | next_power_of_two   |      54.03% |
|   23 | binary_search       |      53.88% |
|   24 | upper_bound         |      53.83% |
|   25 | min                 |      53.65% |
|   26 | max                 |      53.28% |
|   27 | is_prime            |      53.22% |
|   28 | linear_search       |      52.80% |
|   29 | factorial           |      52.76% |
|   30 | abs                 |      52.42% |
|   31 | remove_value        |      52.40% |
|   32 | memcmp              |      52.10% |
|   33 | cos                 |      52.01% |
|   34 | first_difference    |      51.60% |
|   35 | merge_sorted        |      51.57% |
|   36 | average             |      49.95% |
|   37 | dot_product         |      48.50% |
|   38 | count_value         |      47.82% |
|   39 | lcm                 |      47.65% |
|   40 | copy_if             |      47.11% |
|   41 | sqrt                |      46.68% |
|   42 | strcmp              |      45.22% |
|   43 | clamp               |      44.88% |
|   44 | strlen              |      44.74% |
|   45 | atoi                |      44.11% |
|   46 | distance_squared    |      42.77% |
|   47 | linear_interpolate  |      41.83% |
|   48 | strchr              |      38.61% |
|   49 | strstr              |      38.30% |
|   50 | manhattan_distance  |      37.38% |
