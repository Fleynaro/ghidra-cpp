# 03_strlen_variant

- Model: `Ghidra BSim x64`
- Query example: `03_strlen_variant`
- Query instruction: `(none; native Ghidra BSim signature)`
- Expected function: `strlen`
- Expected rank: `2`
- Top match: `factorial` (28.61%)
- Top-1 vs top-2 gap: `0.05` percentage points
- Correct top-1 match: `No`
- Query BSim feature count: `12`

Cosine similarity is multiplied by 100 for presentation; it is not a probability.

| Rank | Database Function   | Similarity |
| ---: | ------------------- | ---------: |
|    1 | factorial           |      28.61% |
|    2 | strlen              |      28.56% |
|    3 | evaluate_polynomial |      23.98% |
|    4 | atoi                |      23.47% |
|    5 | linear_search       |      17.58% |
|    6 | first_difference    |      15.69% |
|    7 | fibonacci           |      15.63% |
|    8 | fill                |      14.95% |
|    9 | iota                |      14.34% |
|   10 | memcmp              |      14.09% |
|   11 | memcpy              |      13.81% |
|   12 | product_array       |      13.32% |
|   13 | sum_array           |      13.32% |
|   14 | is_sorted           |      13.18% |
|   15 | strchr              |      11.56% |
|   16 | swap_ranges         |      11.21% |
|   17 | dot_product         |      11.21% |
|   18 | distance_squared    |      10.74% |
|   19 | cos                 |      10.46% |
|   20 | strcmp              |      10.33% |
|   21 | count_value         |       5.30% |
|   22 | upper_bound         |       5.02% |
|   23 | lower_bound         |       5.02% |
|   24 | copy_if             |       4.45% |
|   25 | remove_value        |       4.45% |
|   26 | manhattan_distance  |       4.34% |
|   27 | average             |       4.08% |
|   28 | bsearch             |       4.06% |
|   29 | partition           |       4.01% |
|   30 | binary_search       |       4.01% |
|   31 | lcm                 |       3.72% |
|   32 | memmove             |       3.41% |
|   33 | strstr              |       2.89% |
|   34 | merge_sorted        |       2.78% |
|   35 | rotate_array        |       2.32% |
|   36 | heap_sort           |       2.28% |
|   37 | abs                 |       0.00% |
|   38 | clamp               |       0.00% |
|   39 | gcd                 |       0.00% |
|   40 | is_prime            |       0.00% |
|   41 | linear_interpolate  |       0.00% |
|   42 | max                 |       0.00% |
|   43 | min                 |       0.00% |
|   44 | next_power_of_two   |       0.00% |
|   45 | pow                 |       0.00% |
|   46 | qsort               |       0.00% |
|   47 | rotate_left         |       0.00% |
|   48 | rotate_right        |       0.00% |
|   49 | sqrt                |       0.00% |
|   50 | unique              |       0.00% |
