# 08_NONE_prefix_sum

- Model: `Ghidra BSim x64`
- Query example: `08_NONE_prefix_sum`
- Query instruction: `(none; native Ghidra BSim signature)`
- Expected function: `NONE`
- Expected rank: `N/A`
- Top match: `swap_ranges` (76.62%)
- Top-1 vs top-2 gap: `2.49` percentage points
- Correct top-1 match: `No`
- Query BSim feature count: `19`

Cosine similarity is multiplied by 100 for presentation; it is not a probability.

| Rank | Database Function   | Similarity |
| ---: | ------------------- | ---------: |
|    1 | swap_ranges         |      76.62% |
|    2 | fill                |      74.13% |
|    3 | dot_product         |      71.33% |
|    4 | iota                |      71.12% |
|    5 | distance_squared    |      68.34% |
|    6 | product_array       |      66.05% |
|    7 | sum_array           |      66.05% |
|    8 | first_difference    |      64.70% |
|    9 | memcpy              |      55.40% |
|   10 | linear_search       |      54.52% |
|   11 | copy_if             |      51.90% |
|   12 | remove_value        |      51.90% |
|   13 | manhattan_distance  |      50.67% |
|   14 | count_value         |      45.04% |
|   15 | is_sorted           |      40.87% |
|   16 | unique              |      38.65% |
|   17 | average             |      36.12% |
|   18 | merge_sorted        |      34.33% |
|   19 | memcmp              |      33.98% |
|   20 | fibonacci           |      33.53% |
|   21 | strchr              |      30.83% |
|   22 | strlen              |      30.07% |
|   23 | rotate_array        |      26.36% |
|   24 | strcmp              |      24.52% |
|   25 | factorial           |      22.82% |
|   26 | cos                 |      22.45% |
|   27 | memmove             |      21.56% |
|   28 | partition           |      19.55% |
|   29 | strstr              |      14.64% |
|   30 | binary_search       |      11.62% |
|   31 | evaluate_polynomial |      10.25% |
|   32 | qsort               |      10.07% |
|   33 | is_prime            |       9.52% |
|   34 | upper_bound         |       9.23% |
|   35 | lower_bound         |       9.23% |
|   36 | heap_sort           |       9.02% |
|   37 | bsearch             |       7.46% |
|   38 | atoi                |       4.92% |
|   39 | gcd                 |       3.56% |
|   40 | lcm                 |       2.94% |
|   41 | abs                 |       0.00% |
|   42 | clamp               |       0.00% |
|   43 | linear_interpolate  |       0.00% |
|   44 | max                 |       0.00% |
|   45 | min                 |       0.00% |
|   46 | next_power_of_two   |       0.00% |
|   47 | pow                 |       0.00% |
|   48 | rotate_left         |       0.00% |
|   49 | rotate_right        |       0.00% |
|   50 | sqrt                |       0.00% |
