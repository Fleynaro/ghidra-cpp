# 07_clamp_variant

- Model: `Ghidra BSim x64`
- Query example: `07_clamp_variant`
- Query instruction: `(none; native Ghidra BSim signature)`
- Expected function: `clamp`
- Expected rank: `1`
- Top match: `clamp` (59.73%)
- Top-1 vs top-2 gap: `15.65` percentage points
- Correct top-1 match: `Yes`
- Query BSim feature count: `12`

Cosine similarity is multiplied by 100 for presentation; it is not a probability.

| Rank | Database Function   | Similarity |
| ---: | ------------------- | ---------: |
|    1 | clamp               |      59.73% |
|    2 | max                 |      44.08% |
|    3 | min                 |      44.08% |
|    4 | sqrt                |      11.47% |
|    5 | qsort               |      10.97% |
|    6 | count_value         |      10.10% |
|    7 | rotate_array        |       9.48% |
|    8 | manhattan_distance  |       8.27% |
|    9 | partition           |       7.64% |
|   10 | lcm                 |       6.69% |
|   11 | is_prime            |       6.62% |
|   12 | pow                 |       6.43% |
|   13 | memmove             |       6.23% |
|   14 | abs                 |       6.21% |
|   15 | strstr              |       5.21% |
|   16 | linear_interpolate  |       4.77% |
|   17 | rotate_left         |       4.51% |
|   18 | rotate_right        |       4.51% |
|   19 | strchr              |       4.09% |
|   20 | first_difference    |       4.04% |
|   21 | lower_bound         |       4.01% |
|   22 | gcd                 |       3.84% |
|   23 | next_power_of_two   |       3.77% |
|   24 | strcmp              |       3.65% |
|   25 | memcmp              |       3.63% |
|   26 | average             |       3.48% |
|   27 | is_sorted           |       3.39% |
|   28 | bsearch             |       3.24% |
|   29 | binary_search       |       3.20% |
|   30 | unique              |       2.79% |
|   31 | atoi                |       2.72% |
|   32 | merge_sorted        |       2.22% |
|   33 | heap_sort           |       1.82% |
|   34 | copy_if             |       0.00% |
|   35 | cos                 |       0.00% |
|   36 | distance_squared    |       0.00% |
|   37 | dot_product         |       0.00% |
|   38 | evaluate_polynomial |       0.00% |
|   39 | factorial           |       0.00% |
|   40 | fibonacci           |       0.00% |
|   41 | fill                |       0.00% |
|   42 | iota                |       0.00% |
|   43 | linear_search       |       0.00% |
|   44 | memcpy              |       0.00% |
|   45 | product_array       |       0.00% |
|   46 | remove_value        |       0.00% |
|   47 | strlen              |       0.00% |
|   48 | sum_array           |       0.00% |
|   49 | swap_ranges         |       0.00% |
|   50 | upper_bound         |       0.00% |
