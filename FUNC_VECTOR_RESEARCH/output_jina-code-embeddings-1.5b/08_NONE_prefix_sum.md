# 08_NONE_prefix_sum

- Model: `hf.co/jinaai/jina-code-embeddings-1.5b-GGUF:BF16`
- Query example: `08_NONE_prefix_sum`
- Query instruction: `Find an equivalent code snippet given the following code snippet:`
- Expected function: `NONE`
- Expected rank: `N/A`
- Top match: `iota` (88.09%)
- Top-1 vs top-2 gap: `3.20` percentage points
- Correct top-1 match: `No`

Cosine similarity is multiplied by 100 for presentation; it is not a probability.

| Rank | Database Function   | Similarity |
| ---: | ------------------- | ---------: |
|    1 | iota                |      88.09% |
|    2 | fill                |      84.88% |
|    3 | sum_array           |      79.06% |
|    4 | swap_ranges         |      77.36% |
|    5 | memcpy              |      70.76% |
|    6 | merge_sorted        |      69.34% |
|    7 | rotate_array        |      69.32% |
|    8 | heap_sort           |      68.53% |
|    9 | evaluate_polynomial |      67.20% |
|   10 | qsort               |      66.26% |
|   11 | partition           |      65.95% |
|   12 | remove_value        |      65.82% |
|   13 | memmove             |      65.38% |
|   14 | count_value         |      63.40% |
|   15 | average             |      63.06% |
|   16 | unique              |      62.54% |
|   17 | copy_if             |      61.24% |
|   18 | strlen              |      59.31% |
|   19 | lower_bound         |      58.61% |
|   20 | fibonacci           |      58.59% |
|   21 | product_array       |      57.78% |
|   22 | upper_bound         |      57.67% |
|   23 | dot_product         |      56.42% |
|   24 | linear_search       |      56.20% |
|   25 | linear_interpolate  |      56.00% |
|   26 | next_power_of_two   |      55.13% |
|   27 | binary_search       |      54.28% |
|   28 | max                 |      53.48% |
|   29 | min                 |      53.31% |
|   30 | strchr              |      52.96% |
|   31 | pow                 |      52.31% |
|   32 | gcd                 |      51.51% |
|   33 | factorial           |      51.20% |
|   34 | abs                 |      50.04% |
|   35 | is_sorted           |      49.91% |
|   36 | bsearch             |      49.75% |
|   37 | manhattan_distance  |      49.37% |
|   38 | strstr              |      48.91% |
|   39 | lcm                 |      48.82% |
|   40 | atoi                |      48.06% |
|   41 | first_difference    |      47.03% |
|   42 | clamp               |      45.95% |
|   43 | is_prime            |      45.57% |
|   44 | sqrt                |      45.57% |
|   45 | cos                 |      45.33% |
|   46 | distance_squared    |      45.02% |
|   47 | rotate_left         |      42.00% |
|   48 | rotate_right        |      40.77% |
|   49 | memcmp              |      40.45% |
|   50 | strcmp              |      37.90% |
