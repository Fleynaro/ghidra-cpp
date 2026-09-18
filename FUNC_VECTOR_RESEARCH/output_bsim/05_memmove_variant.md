# 05_memmove_variant

- Model: `Ghidra BSim x64`
- Query example: `05_memmove_variant`
- Query instruction: `(none; native Ghidra BSim signature)`
- Expected function: `memmove`
- Expected rank: `1`
- Top match: `memmove` (67.66%)
- Top-1 vs top-2 gap: `24.06` percentage points
- Correct top-1 match: `Yes`
- Query BSim feature count: `36`

Cosine similarity is multiplied by 100 for presentation; it is not a probability.

| Rank | Database Function   | Similarity |
| ---: | ------------------- | ---------: |
|    1 | memmove             |      67.66% |
|    2 | memcpy              |      43.60% |
|    3 | memcmp              |      32.89% |
|    4 | strchr              |      30.76% |
|    5 | strstr              |      29.71% |
|    6 | average             |      28.85% |
|    7 | strlen              |      27.71% |
|    8 | strcmp              |      25.23% |
|    9 | rotate_array        |      24.97% |
|   10 | first_difference    |      22.36% |
|   11 | fill                |      21.13% |
|   12 | iota                |      20.27% |
|   13 | swap_ranges         |      19.72% |
|   14 | dot_product         |      19.71% |
|   15 | distance_squared    |      18.89% |
|   16 | product_array       |      18.83% |
|   17 | sum_array           |      18.83% |
|   18 | linear_search       |      18.04% |
|   19 | unique              |      17.25% |
|   20 | manhattan_distance  |      17.09% |
|   21 | count_value         |      16.73% |
|   22 | evaluate_polynomial |      15.64% |
|   23 | is_sorted           |      15.47% |
|   24 | copy_if             |      15.46% |
|   25 | remove_value        |      15.46% |
|   26 | is_prime            |      13.18% |
|   27 | factorial           |      11.28% |
|   28 | fibonacci           |      11.24% |
|   29 | clamp               |      10.26% |
|   30 | merge_sorted        |      10.13% |
|   31 | lcm                 |       8.18% |
|   32 | cos                 |       7.53% |
|   33 | partition           |       7.44% |
|   34 | atoi                |       6.45% |
|   35 | sqrt                |       5.78% |
|   36 | qsort               |       5.72% |
|   37 | gcd                 |       4.86% |
|   38 | max                 |       3.78% |
|   39 | min                 |       3.78% |
|   40 | pow                 |       3.70% |
|   41 | bsearch             |       3.69% |
|   42 | abs                 |       3.57% |
|   43 | linear_interpolate  |       2.74% |
|   44 | rotate_left         |       2.60% |
|   45 | rotate_right        |       2.60% |
|   46 | lower_bound         |       2.31% |
|   47 | heap_sort           |       2.24% |
|   48 | next_power_of_two   |       2.17% |
|   49 | binary_search       |       1.84% |
|   50 | upper_bound         |       0.00% |
