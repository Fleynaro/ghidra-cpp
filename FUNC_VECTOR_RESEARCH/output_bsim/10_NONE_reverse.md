# 10_NONE_reverse

- Model: `Ghidra BSim x64`
- Query example: `10_NONE_reverse`
- Query instruction: `(none; native Ghidra BSim signature)`
- Expected function: `NONE`
- Expected rank: `N/A`
- Top match: `swap_ranges` (68.18%)
- Top-1 vs top-2 gap: `13.19` percentage points
- Correct top-1 match: `No`
- Query BSim feature count: `27`

Cosine similarity is multiplied by 100 for presentation; it is not a probability.

| Rank | Database Function   | Similarity |
| ---: | ------------------- | ---------: |
|    1 | swap_ranges         |      68.18% |
|    2 | dot_product         |      54.99% |
|    3 | distance_squared    |      52.69% |
|    4 | rotate_array        |      50.55% |
|    5 | fill                |      50.01% |
|    6 | first_difference    |      49.39% |
|    7 | iota                |      47.98% |
|    8 | product_array       |      44.56% |
|    9 | sum_array           |      44.56% |
|   10 | copy_if             |      43.20% |
|   11 | remove_value        |      43.20% |
|   12 | manhattan_distance  |      42.18% |
|   13 | unique              |      39.03% |
|   14 | is_sorted           |      37.78% |
|   15 | linear_search       |      35.37% |
|   16 | memcpy              |      35.31% |
|   17 | merge_sorted        |      33.50% |
|   18 | evaluate_polynomial |      33.19% |
|   19 | count_value         |      32.81% |
|   20 | partition           |      29.63% |
|   21 | fibonacci           |      27.92% |
|   22 | memmove             |      27.83% |
|   23 | average             |      25.80% |
|   24 | strchr              |      25.67% |
|   25 | strlen              |      25.03% |
|   26 | strcmp              |      20.41% |
|   27 | memcmp              |      20.26% |
|   28 | heap_sort           |      19.58% |
|   29 | factorial           |      18.99% |
|   30 | qsort               |      18.91% |
|   31 | cos                 |      18.69% |
|   32 | bsearch             |      16.98% |
|   33 | binary_search       |      16.76% |
|   34 | strstr              |      12.19% |
|   35 | upper_bound         |      12.12% |
|   36 | lower_bound         |      12.12% |
|   37 | is_prime            |       7.92% |
|   38 | atoi                |       4.10% |
|   39 | gcd                 |       2.96% |
|   40 | lcm                 |       2.45% |
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
