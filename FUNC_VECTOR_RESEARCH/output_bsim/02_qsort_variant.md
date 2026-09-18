# 02_qsort_variant

- Model: `Ghidra BSim x64`
- Query example: `02_qsort_variant`
- Query instruction: `(none; native Ghidra BSim signature)`
- Expected function: `qsort`
- Expected rank: `29`
- Top match: `swap_ranges` (44.23%)
- Top-1 vs top-2 gap: `2.58` percentage points
- Correct top-1 match: `No`
- Query BSim feature count: `41`

Cosine similarity is multiplied by 100 for presentation; it is not a probability.

| Rank | Database Function   | Similarity |
| ---: | ------------------- | ---------: |
|    1 | swap_ranges         |      44.23% |
|    2 | fill                |      41.65% |
|    3 | count_value         |      40.92% |
|    4 | copy_if             |      40.84% |
|    5 | remove_value        |      40.84% |
|    6 | iota                |      39.96% |
|    7 | manhattan_distance  |      39.86% |
|    8 | dot_product         |      38.48% |
|    9 | product_array       |      37.11% |
|   10 | sum_array           |      37.11% |
|   11 | rotate_array        |      37.02% |
|   12 | distance_squared    |      36.87% |
|   13 | is_sorted           |      36.32% |
|   14 | first_difference    |      35.87% |
|   15 | merge_sorted        |      35.29% |
|   16 | unique              |      32.72% |
|   17 | linear_search       |      29.52% |
|   18 | partition           |      27.33% |
|   19 | average             |      27.14% |
|   20 | memcpy              |      25.10% |
|   21 | heap_sort           |      23.49% |
|   22 | strchr              |      18.44% |
|   23 | fibonacci           |      18.39% |
|   24 | strlen              |      15.98% |
|   25 | bsearch             |      15.73% |
|   26 | memcmp              |      15.65% |
|   27 | binary_search       |      15.53% |
|   28 | strcmp              |      14.39% |
|   29 | qsort               |      14.23% |
|   30 | memmove             |      13.86% |
|   31 | is_prime            |      12.36% |
|   32 | cos                 |      12.32% |
|   33 | strstr              |      12.17% |
|   34 | lower_bound         |      12.13% |
|   35 | evaluate_polynomial |      11.66% |
|   36 | factorial           |      10.99% |
|   37 | upper_bound         |      10.00% |
|   38 | atoi                |       6.29% |
|   39 | lcm                 |       3.89% |
|   40 | pow                 |       3.61% |
|   41 | clamp               |       3.23% |
|   42 | gcd                 |       2.45% |
|   43 | sqrt                |       1.85% |
|   44 | abs                 |       0.00% |
|   45 | linear_interpolate  |       0.00% |
|   46 | max                 |       0.00% |
|   47 | min                 |       0.00% |
|   48 | next_power_of_two   |       0.00% |
|   49 | rotate_left         |       0.00% |
|   50 | rotate_right        |       0.00% |
