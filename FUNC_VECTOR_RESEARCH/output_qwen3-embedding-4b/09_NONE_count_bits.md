# 09_NONE_count_bits

- Model: `qwen3-embedding:4b`
- Query example: `09_NONE_count_bits`
- Query instruction: `(none; raw source was embedded)`
- Expected function: `NONE`
- Expected rank: `N/A`
- Top match: `next_power_of_two` (74.31%)
- Top-1 vs top-2 gap: `14.15` percentage points
- Correct top-1 match: `No`

Cosine similarity is multiplied by 100 for presentation; it is not a probability.

| Rank | Database Function   | Similarity |
| ---: | ------------------- | ---------: |
|    1 | next_power_of_two   |      74.31% |
|    2 | unique              |      60.16% |
|    3 | strlen              |      59.33% |
|    4 | count_value         |      57.08% |
|    5 | is_prime            |      54.62% |
|    6 | rotate_left         |      54.61% |
|    7 | rotate_right        |      54.57% |
|    8 | fibonacci           |      53.64% |
|    9 | gcd                 |      53.42% |
|   10 | sum_array           |      51.88% |
|   11 | factorial           |      51.61% |
|   12 | lower_bound         |      51.59% |
|   13 | remove_value        |      51.52% |
|   14 | pow                 |      51.41% |
|   15 | upper_bound         |      50.43% |
|   16 | atoi                |      49.54% |
|   17 | binary_search       |      49.02% |
|   18 | product_array       |      48.42% |
|   19 | abs                 |      47.55% |
|   20 | cos                 |      47.47% |
|   21 | sqrt                |      47.12% |
|   22 | first_difference    |      46.82% |
|   23 | bsearch             |      46.79% |
|   24 | heap_sort           |      46.72% |
|   25 | rotate_array        |      46.71% |
|   26 | partition           |      46.68% |
|   27 | lcm                 |      46.47% |
|   28 | average             |      46.30% |
|   29 | linear_search       |      45.43% |
|   30 | strcmp              |      45.40% |
|   31 | min                 |      45.23% |
|   32 | evaluate_polynomial |      44.70% |
|   33 | qsort               |      44.63% |
|   34 | max                 |      44.21% |
|   35 | is_sorted           |      43.98% |
|   36 | copy_if             |      43.62% |
|   37 | iota                |      42.71% |
|   38 | fill                |      42.56% |
|   39 | memcmp              |      42.15% |
|   40 | dot_product         |      41.51% |
|   41 | swap_ranges         |      40.54% |
|   42 | memcpy              |      40.17% |
|   43 | distance_squared    |      38.88% |
|   44 | manhattan_distance  |      37.02% |
|   45 | strchr              |      36.94% |
|   46 | clamp               |      36.79% |
|   47 | memmove             |      35.41% |
|   48 | strstr              |      34.42% |
|   49 | merge_sorted        |      33.16% |
|   50 | linear_interpolate  |      32.75% |
