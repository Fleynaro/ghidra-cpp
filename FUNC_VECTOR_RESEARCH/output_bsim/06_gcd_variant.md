# 06_gcd_variant

- Model: `Ghidra BSim x64`
- Query example: `06_gcd_variant`
- Query instruction: `(none; native Ghidra BSim signature)`
- Expected function: `gcd`
- Expected rank: `1`
- Top match: `gcd` (44.40%)
- Top-1 vs top-2 gap: `3.07` percentage points
- Correct top-1 match: `Yes`
- Query BSim feature count: `27`

Cosine similarity is multiplied by 100 for presentation; it is not a probability.

| Rank | Database Function   | Similarity |
| ---: | ------------------- | ---------: |
|    1 | gcd                 |      44.40% |
|    2 | lcm                 |      41.33% |
|    3 | abs                 |      38.58% |
|    4 | pow                 |      26.63% |
|    5 | is_prime            |      11.90% |
|    6 | max                 |      10.09% |
|    7 | min                 |      10.09% |
|    8 | strchr              |       8.30% |
|    9 | average             |       8.11% |
|   10 | clamp               |       7.64% |
|   11 | memmove             |       6.78% |
|   12 | unique              |       6.33% |
|   13 | next_power_of_two   |       5.80% |
|   14 | strstr              |       5.38% |
|   15 | qsort               |       5.17% |
|   16 | memcmp              |       4.78% |
|   17 | merge_sorted        |       4.52% |
|   18 | is_sorted           |       4.47% |
|   19 | sqrt                |       4.38% |
|   20 | rotate_array        |       4.32% |
|   21 | bsearch             |       4.27% |
|   22 | fibonacci           |       3.94% |
|   23 | fill                |       3.77% |
|   24 | atoi                |       3.74% |
|   25 | iota                |       3.62% |
|   26 | memcpy              |       3.48% |
|   27 | product_array       |       3.36% |
|   28 | sum_array           |       3.36% |
|   29 | linear_interpolate  |       3.14% |
|   30 | linear_search       |       2.98% |
|   31 | rotate_left         |       2.98% |
|   32 | rotate_right        |       2.98% |
|   33 | swap_ranges         |       2.83% |
|   34 | dot_product         |       2.83% |
|   35 | count_value         |       2.79% |
|   36 | distance_squared    |       2.71% |
|   37 | first_difference    |       2.66% |
|   38 | lower_bound         |       2.64% |
|   39 | upper_bound         |       2.64% |
|   40 | cos                 |       2.64% |
|   41 | strcmp              |       2.41% |
|   42 | copy_if             |       2.34% |
|   43 | remove_value        |       2.34% |
|   44 | manhattan_distance  |       2.29% |
|   45 | partition           |       2.11% |
|   46 | binary_search       |       2.11% |
|   47 | heap_sort           |       1.20% |
|   48 | evaluate_polynomial |       0.00% |
|   49 | factorial           |       0.00% |
|   50 | strlen              |       0.00% |
