# 10_NONE_reverse

- Model: `hf.co/jinaai/jina-code-embeddings-1.5b-GGUF:BF16`
- Query example: `10_NONE_reverse`
- Query instruction: `Find an equivalent code snippet given the following code snippet:`
- Expected function: `NONE`
- Expected rank: `N/A`
- Top match: `swap_ranges` (89.32%)
- Top-1 vs top-2 gap: `4.40` percentage points
- Correct top-1 match: `No`

Cosine similarity is multiplied by 100 for presentation; it is not a probability.

| Rank | Database Function   | Similarity |
| ---: | ------------------- | ---------: |
|    1 | swap_ranges         |      89.32% |
|    2 | heap_sort           |      84.92% |
|    3 | rotate_array        |      84.12% |
|    4 | qsort               |      75.14% |
|    5 | fill                |      72.62% |
|    6 | memmove             |      72.46% |
|    7 | partition           |      71.28% |
|    8 | iota                |      71.03% |
|    9 | merge_sorted        |      68.99% |
|   10 | memcpy              |      65.80% |
|   11 | remove_value        |      65.54% |
|   12 | upper_bound         |      55.91% |
|   13 | unique              |      55.89% |
|   14 | lower_bound         |      55.54% |
|   15 | min                 |      54.31% |
|   16 | binary_search       |      53.33% |
|   17 | copy_if             |      51.62% |
|   18 | max                 |      51.33% |
|   19 | bsearch             |      51.15% |
|   20 | linear_search       |      51.09% |
|   21 | clamp               |      51.08% |
|   22 | count_value         |      50.30% |
|   23 | gcd                 |      49.50% |
|   24 | first_difference    |      48.89% |
|   25 | sum_array           |      47.98% |
|   26 | is_sorted           |      47.25% |
|   27 | evaluate_polynomial |      47.00% |
|   28 | pow                 |      44.93% |
|   29 | strlen              |      44.87% |
|   30 | memcmp              |      44.60% |
|   31 | strchr              |      44.17% |
|   32 | abs                 |      44.14% |
|   33 | average             |      43.73% |
|   34 | linear_interpolate  |      43.56% |
|   35 | rotate_right        |      43.13% |
|   36 | rotate_left         |      42.11% |
|   37 | is_prime            |      42.00% |
|   38 | lcm                 |      41.82% |
|   39 | next_power_of_two   |      41.67% |
|   40 | product_array       |      41.66% |
|   41 | strstr              |      41.59% |
|   42 | factorial           |      41.33% |
|   43 | fibonacci           |      38.33% |
|   44 | sqrt                |      37.89% |
|   45 | dot_product         |      37.38% |
|   46 | atoi                |      37.16% |
|   47 | strcmp              |      36.71% |
|   48 | cos                 |      34.62% |
|   49 | manhattan_distance  |      33.89% |
|   50 | distance_squared    |      31.82% |
