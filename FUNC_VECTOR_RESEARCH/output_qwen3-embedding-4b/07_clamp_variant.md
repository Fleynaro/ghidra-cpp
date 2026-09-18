# 07_clamp_variant

- Model: `qwen3-embedding:4b`
- Query example: `07_clamp_variant`
- Query instruction: `(none; raw source was embedded)`
- Expected function: `clamp`
- Expected rank: `1`
- Top match: `clamp` (97.12%)
- Top-1 vs top-2 gap: `18.45` percentage points
- Correct top-1 match: `Yes`

Cosine similarity is multiplied by 100 for presentation; it is not a probability.

| Rank | Database Function   | Similarity |
| ---: | ------------------- | ---------: |
|    1 | clamp               |      97.12% |
|    2 | min                 |      78.66% |
|    3 | max                 |      77.70% |
|    4 | upper_bound         |      64.53% |
|    5 | lower_bound         |      63.52% |
|    6 | memcmp              |      62.86% |
|    7 | binary_search       |      61.08% |
|    8 | partition           |      59.91% |
|    9 | abs                 |      59.40% |
|   10 | merge_sorted        |      59.28% |
|   11 | gcd                 |      58.92% |
|   12 | bsearch             |      58.32% |
|   13 | first_difference    |      58.20% |
|   14 | memmove             |      57.47% |
|   15 | qsort               |      55.73% |
|   16 | linear_search       |      55.44% |
|   17 | lcm                 |      55.26% |
|   18 | is_sorted           |      53.30% |
|   19 | copy_if             |      53.10% |
|   20 | memcpy              |      52.77% |
|   21 | linear_interpolate  |      52.21% |
|   22 | iota                |      52.04% |
|   23 | swap_ranges         |      51.96% |
|   24 | dot_product         |      51.93% |
|   25 | manhattan_distance  |      51.57% |
|   26 | evaluate_polynomial |      51.54% |
|   27 | heap_sort           |      51.08% |
|   28 | count_value         |      50.95% |
|   29 | sqrt                |      50.34% |
|   30 | is_prime            |      50.26% |
|   31 | rotate_array        |      50.24% |
|   32 | rotate_left         |      50.04% |
|   33 | rotate_right        |      49.60% |
|   34 | distance_squared    |      49.42% |
|   35 | fill                |      49.16% |
|   36 | sum_array           |      48.75% |
|   37 | strcmp              |      48.69% |
|   38 | product_array       |      48.62% |
|   39 | remove_value        |      48.26% |
|   40 | next_power_of_two   |      47.90% |
|   41 | pow                 |      47.53% |
|   42 | atoi                |      47.23% |
|   43 | fibonacci           |      46.37% |
|   44 | unique              |      45.26% |
|   45 | cos                 |      43.64% |
|   46 | factorial           |      41.69% |
|   47 | average             |      40.44% |
|   48 | strstr              |      39.46% |
|   49 | strlen              |      38.62% |
|   50 | strchr              |      36.74% |
