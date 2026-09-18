# 09_NONE_count_bits

- Model: `Ghidra BSim x64`
- Query example: `09_NONE_count_bits`
- Query instruction: `(none; native Ghidra BSim signature)`
- Expected function: `NONE`
- Expected rank: `N/A`
- Top match: `strlen` (52.04%)
- Top-1 vs top-2 gap: `9.53` percentage points
- Correct top-1 match: `No`
- Query BSim feature count: `12`

Cosine similarity is multiplied by 100 for presentation; it is not a probability.

| Rank | Database Function   | Similarity |
| ---: | ------------------- | ---------: |
|    1 | strlen              |      52.04% |
|    2 | factorial           |      42.51% |
|    3 | fibonacci           |      29.32% |
|    4 | linear_search       |      28.83% |
|    5 | fill                |      28.04% |
|    6 | iota                |      26.90% |
|    7 | memcpy              |      25.91% |
|    8 | first_difference    |      25.73% |
|    9 | product_array       |      24.99% |
|   10 | sum_array           |      24.99% |
|   11 | evaluate_polynomial |      24.12% |
|   12 | memcmp              |      23.10% |
|   13 | strchr              |      21.68% |
|   14 | is_sorted           |      21.61% |
|   15 | swap_ranges         |      21.04% |
|   16 | dot_product         |      21.03% |
|   17 | distance_squared    |      20.15% |
|   18 | cos                 |      19.63% |
|   19 | strcmp              |      19.39% |
|   20 | count_value         |      15.75% |
|   21 | average             |      13.60% |
|   22 | copy_if             |      13.22% |
|   23 | remove_value        |      13.22% |
|   24 | manhattan_distance  |      12.91% |
|   25 | memmove             |      11.38% |
|   26 | strstr              |       9.64% |
|   27 | is_prime            |       8.57% |
|   28 | merge_sorted        |       8.27% |
|   29 | rotate_array        |       7.74% |
|   30 | unique              |       7.62% |
|   31 | atoi                |       6.32% |
|   32 | upper_bound         |       5.05% |
|   33 | lower_bound         |       5.05% |
|   34 | bsearch             |       4.08% |
|   35 | partition           |       4.03% |
|   36 | binary_search       |       4.03% |
|   37 | lcm                 |       3.74% |
|   38 | heap_sort           |       2.29% |
|   39 | abs                 |       0.00% |
|   40 | clamp               |       0.00% |
|   41 | gcd                 |       0.00% |
|   42 | linear_interpolate  |       0.00% |
|   43 | max                 |       0.00% |
|   44 | min                 |       0.00% |
|   45 | next_power_of_two   |       0.00% |
|   46 | pow                 |       0.00% |
|   47 | qsort               |       0.00% |
|   48 | rotate_left         |       0.00% |
|   49 | rotate_right        |       0.00% |
|   50 | sqrt                |       0.00% |
