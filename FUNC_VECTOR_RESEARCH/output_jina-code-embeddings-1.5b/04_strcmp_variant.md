# 04_strcmp_variant

- Model: `hf.co/jinaai/jina-code-embeddings-1.5b-GGUF:BF16`
- Query example: `04_strcmp_variant`
- Query instruction: `Find an equivalent code snippet given the following code snippet:`
- Expected function: `strcmp`
- Expected rank: `2`
- Top match: `memcmp` (88.48%)
- Top-1 vs top-2 gap: `2.29` percentage points
- Correct top-1 match: `No`

Cosine similarity is multiplied by 100 for presentation; it is not a probability.

| Rank | Database Function   | Similarity |
| ---: | ------------------- | ---------: |
|    1 | memcmp              |      88.48% |
|    2 | strcmp              |      86.19% |
|    3 | first_difference    |      72.06% |
|    4 | strstr              |      65.31% |
|    5 | min                 |      61.89% |
|    6 | strchr              |      61.43% |
|    7 | max                 |      57.83% |
|    8 | strlen              |      55.36% |
|    9 | abs                 |      54.77% |
|   10 | atoi                |      53.75% |
|   11 | linear_search       |      52.49% |
|   12 | gcd                 |      51.40% |
|   13 | lcm                 |      51.33% |
|   14 | manhattan_distance  |      50.76% |
|   15 | memmove             |      49.18% |
|   16 | clamp               |      49.04% |
|   17 | is_sorted           |      48.98% |
|   18 | bsearch             |      47.44% |
|   19 | binary_search       |      46.08% |
|   20 | merge_sorted        |      45.04% |
|   21 | is_prime            |      44.49% |
|   22 | memcpy              |      44.32% |
|   23 | unique              |      44.23% |
|   24 | pow                 |      43.15% |
|   25 | partition           |      42.00% |
|   26 | upper_bound         |      41.87% |
|   27 | lower_bound         |      41.41% |
|   28 | remove_value        |      41.04% |
|   29 | count_value         |      40.74% |
|   30 | sqrt                |      39.21% |
|   31 | linear_interpolate  |      38.99% |
|   32 | copy_if             |      38.82% |
|   33 | swap_ranges         |      38.40% |
|   34 | qsort               |      38.14% |
|   35 | iota                |      37.84% |
|   36 | fill                |      37.82% |
|   37 | heap_sort           |      36.99% |
|   38 | distance_squared    |      35.49% |
|   39 | factorial           |      34.78% |
|   40 | next_power_of_two   |      34.30% |
|   41 | average             |      33.99% |
|   42 | rotate_array        |      33.31% |
|   43 | rotate_right        |      33.30% |
|   44 | rotate_left         |      32.73% |
|   45 | sum_array           |      31.53% |
|   46 | cos                 |      31.46% |
|   47 | evaluate_polynomial |      31.21% |
|   48 | fibonacci           |      31.06% |
|   49 | product_array       |      30.13% |
|   50 | dot_product         |      28.50% |
