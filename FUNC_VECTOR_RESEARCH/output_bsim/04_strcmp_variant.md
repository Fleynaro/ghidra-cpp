# 04_strcmp_variant

- Model: `Ghidra BSim x64`
- Query example: `04_strcmp_variant`
- Query instruction: `(none; native Ghidra BSim signature)`
- Expected function: `strcmp`
- Expected rank: `11`
- Top match: `atoi` (29.71%)
- Top-1 vs top-2 gap: `11.03` percentage points
- Correct top-1 match: `No`
- Query BSim feature count: `31`

Cosine similarity is multiplied by 100 for presentation; it is not a probability.

| Rank | Database Function   | Similarity |
| ---: | ------------------- | ---------: |
|    1 | atoi                |      29.71% |
|    2 | strchr              |      18.68% |
|    3 | memcmp              |      18.08% |
|    4 | fibonacci           |      14.29% |
|    5 | fill                |      13.66% |
|    6 | is_prime            |      13.35% |
|    7 | iota                |      13.11% |
|    8 | memcpy              |      12.62% |
|    9 | product_array       |      12.17% |
|   10 | sum_array           |      12.17% |
|   11 | strcmp              |      11.69% |
|   12 | linear_search       |      11.45% |
|   13 | is_sorted           |      10.82% |
|   14 | factorial           |      10.44% |
|   15 | strlen              |      10.42% |
|   16 | swap_ranges         |      10.25% |
|   17 | dot_product         |      10.25% |
|   18 | first_difference    |      10.22% |
|   19 | distance_squared    |       9.82% |
|   20 | cos                 |       9.57% |
|   21 | evaluate_polynomial |       8.75% |
|   22 | lcm                 |       7.15% |
|   23 | bsearch             |       6.96% |
|   24 | binary_search       |       6.87% |
|   25 | pow                 |       6.71% |
|   26 | merge_sorted        |       6.36% |
|   27 | count_value         |       6.30% |
|   28 | lower_bound         |       5.97% |
|   29 | upper_bound         |       5.96% |
|   30 | strstr              |       5.58% |
|   31 | copy_if             |       5.28% |
|   32 | remove_value        |       5.28% |
|   33 | manhattan_distance  |       5.16% |
|   34 | partition           |       4.77% |
|   35 | clamp               |       4.02% |
|   36 | heap_sort           |       3.90% |
|   37 | gcd                 |       3.04% |
|   38 | rotate_array        |       3.02% |
|   39 | average             |       2.75% |
|   40 | memmove             |       2.30% |
|   41 | sqrt                |       2.30% |
|   42 | unique              |       2.04% |
|   43 | qsort               |       1.66% |
|   44 | abs                 |       0.00% |
|   45 | linear_interpolate  |       0.00% |
|   46 | max                 |       0.00% |
|   47 | min                 |       0.00% |
|   48 | next_power_of_two   |       0.00% |
|   49 | rotate_left         |       0.00% |
|   50 | rotate_right        |       0.00% |
