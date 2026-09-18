# 01_bsearch_variant

- Model: `Ghidra BSim x64`
- Query example: `01_bsearch_variant`
- Query instruction: `(none; native Ghidra BSim signature)`
- Expected function: `bsearch`
- Expected rank: `4`
- Top match: `binary_search` (75.83%)
- Top-1 vs top-2 gap: `3.65` percentage points
- Correct top-1 match: `No`
- Query BSim feature count: `33`

Cosine similarity is multiplied by 100 for presentation; it is not a probability.

| Rank | Database Function   | Similarity |
| ---: | ------------------- | ---------: |
|    1 | binary_search       |      75.83% |
|    2 | lower_bound         |      72.17% |
|    3 | upper_bound         |      53.86% |
|    4 | bsearch             |      43.95% |
|    5 | count_value         |      18.20% |
|    6 | is_sorted           |      17.96% |
|    7 | first_difference    |      17.36% |
|    8 | partition           |      16.96% |
|    9 | linear_search       |      16.83% |
|   10 | copy_if             |      16.75% |
|   11 | remove_value        |      16.75% |
|   12 | manhattan_distance  |      14.91% |
|   13 | swap_ranges         |      14.74% |
|   14 | dot_product         |      14.74% |
|   15 | distance_squared    |      14.12% |
|   16 | product_array       |      12.81% |
|   17 | sum_array           |      12.81% |
|   18 | unique              |      12.34% |
|   19 | rotate_array        |      10.03% |
|   20 | heap_sort           |       9.63% |
|   21 | merge_sorted        |       9.55% |
|   22 | fill                |       9.11% |
|   23 | iota                |       8.74% |
|   24 | evaluate_polynomial |       8.06% |
|   25 | average             |       7.69% |
|   26 | qsort               |       7.55% |
|   27 | atoi                |       7.03% |
|   28 | memcmp              |       6.80% |
|   29 | sqrt                |       6.47% |
|   30 | strchr              |       5.34% |
|   31 | strcmp              |       4.78% |
|   32 | factorial           |       4.06% |
|   33 | strlen              |       4.05% |
|   34 | fibonacci           |       4.02% |
|   35 | memcpy              |       3.55% |
|   36 | clamp               |       3.53% |
|   37 | strstr              |       3.13% |
|   38 | cos                 |       2.69% |
|   39 | lcm                 |       2.04% |
|   40 | is_prime            |       2.02% |
|   41 | pow                 |       1.96% |
|   42 | abs                 |       0.00% |
|   43 | gcd                 |       0.00% |
|   44 | linear_interpolate  |       0.00% |
|   45 | max                 |       0.00% |
|   46 | memmove             |       0.00% |
|   47 | min                 |       0.00% |
|   48 | next_power_of_two   |       0.00% |
|   49 | rotate_left         |       0.00% |
|   50 | rotate_right        |       0.00% |
