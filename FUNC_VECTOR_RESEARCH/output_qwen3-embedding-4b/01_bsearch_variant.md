# 01_bsearch_variant

- Model: `qwen3-embedding:4b`
- Query example: `01_bsearch_variant`
- Query instruction: `(none; raw source was embedded)`
- Expected function: `bsearch`
- Expected rank: `1`
- Top match: `bsearch` (97.40%)
- Top-1 vs top-2 gap: `1.65` percentage points
- Correct top-1 match: `Yes`

Cosine similarity is multiplied by 100 for presentation; it is not a probability.

| Rank | Database Function   | Similarity |
| ---: | ------------------- | ---------: |
|    1 | bsearch             |      97.40% |
|    2 | upper_bound         |      95.75% |
|    3 | lower_bound         |      95.38% |
|    4 | binary_search       |      92.43% |
|    5 | linear_search       |      91.07% |
|    6 | count_value         |      73.80% |
|    7 | first_difference    |      69.62% |
|    8 | partition           |      65.55% |
|    9 | remove_value        |      62.05% |
|   10 | copy_if             |      60.45% |
|   11 | clamp               |      59.16% |
|   12 | strchr              |      59.07% |
|   13 | memcmp              |      58.70% |
|   14 | is_sorted           |      58.50% |
|   15 | iota                |      56.77% |
|   16 | heap_sort           |      55.21% |
|   17 | qsort               |      55.13% |
|   18 | unique              |      54.99% |
|   19 | sum_array           |      54.93% |
|   20 | rotate_array        |      54.24% |
|   21 | fill                |      53.84% |
|   22 | min                 |      53.11% |
|   23 | linear_interpolate  |      52.98% |
|   24 | strcmp              |      52.35% |
|   25 | evaluate_polynomial |      52.28% |
|   26 | strstr              |      51.93% |
|   27 | sqrt                |      51.85% |
|   28 | memmove             |      51.52% |
|   29 | abs                 |      51.45% |
|   30 | max                 |      51.22% |
|   31 | gcd                 |      50.65% |
|   32 | memcpy              |      50.64% |
|   33 | is_prime            |      50.47% |
|   34 | fibonacci           |      49.93% |
|   35 | rotate_left         |      49.91% |
|   36 | swap_ranges         |      49.83% |
|   37 | pow                 |      49.56% |
|   38 | rotate_right        |      49.44% |
|   39 | next_power_of_two   |      48.85% |
|   40 | dot_product         |      48.62% |
|   41 | product_array       |      48.58% |
|   42 | average             |      48.47% |
|   43 | merge_sorted        |      47.33% |
|   44 | atoi                |      46.85% |
|   45 | lcm                 |      46.09% |
|   46 | distance_squared    |      45.91% |
|   47 | strlen              |      44.92% |
|   48 | cos                 |      43.97% |
|   49 | manhattan_distance  |      41.03% |
|   50 | factorial           |      40.15% |
