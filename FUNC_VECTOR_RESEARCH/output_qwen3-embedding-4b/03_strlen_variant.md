# 03_strlen_variant

- Model: `qwen3-embedding:4b`
- Query example: `03_strlen_variant`
- Query instruction: `(none; raw source was embedded)`
- Expected function: `strlen`
- Expected rank: `1`
- Top match: `strlen` (95.07%)
- Top-1 vs top-2 gap: `21.71` percentage points
- Correct top-1 match: `Yes`

Cosine similarity is multiplied by 100 for presentation; it is not a probability.

| Rank | Database Function   | Similarity |
| ---: | ------------------- | ---------: |
|    1 | strlen              |      95.07% |
|    2 | strcmp              |      73.36% |
|    3 | atoi                |      64.85% |
|    4 | strchr              |      63.36% |
|    5 | unique              |      61.73% |
|    6 | memcmp              |      60.63% |
|    7 | strstr              |      57.87% |
|    8 | sum_array           |      56.97% |
|    9 | first_difference    |      55.98% |
|   10 | remove_value        |      54.73% |
|   11 | memcpy              |      54.44% |
|   12 | count_value         |      54.02% |
|   13 | fibonacci           |      52.43% |
|   14 | next_power_of_two   |      52.32% |
|   15 | iota                |      51.72% |
|   16 | memmove             |      50.84% |
|   17 | gcd                 |      50.46% |
|   18 | linear_search       |      50.39% |
|   19 | abs                 |      50.19% |
|   20 | lower_bound         |      49.18% |
|   21 | is_sorted           |      48.58% |
|   22 | evaluate_polynomial |      48.36% |
|   23 | factorial           |      48.03% |
|   24 | max                 |      47.78% |
|   25 | distance_squared    |      47.67% |
|   26 | fill                |      47.55% |
|   27 | product_array       |      47.33% |
|   28 | average             |      47.06% |
|   29 | is_prime            |      46.89% |
|   30 | min                 |      46.88% |
|   31 | upper_bound         |      46.76% |
|   32 | partition           |      46.74% |
|   33 | bsearch             |      45.78% |
|   34 | binary_search       |      45.72% |
|   35 | cos                 |      45.02% |
|   36 | dot_product         |      44.97% |
|   37 | copy_if             |      44.90% |
|   38 | manhattan_distance  |      44.60% |
|   39 | rotate_array        |      44.35% |
|   40 | sqrt                |      43.97% |
|   41 | lcm                 |      43.44% |
|   42 | swap_ranges         |      43.42% |
|   43 | pow                 |      41.99% |
|   44 | heap_sort           |      41.43% |
|   45 | rotate_left         |      41.43% |
|   46 | rotate_right        |      40.91% |
|   47 | qsort               |      40.21% |
|   48 | clamp               |      38.21% |
|   49 | merge_sorted        |      37.03% |
|   50 | linear_interpolate  |      34.83% |
