# 01_bsearch_variant

- Model: `hf.co/jinaai/jina-code-embeddings-1.5b-GGUF:BF16`
- Query example: `01_bsearch_variant`
- Query instruction: `Find an equivalent code snippet given the following code snippet:`
- Expected function: `bsearch`
- Expected rank: `1`
- Top match: `bsearch` (94.00%)
- Top-1 vs top-2 gap: `5.44` percentage points
- Correct top-1 match: `Yes`

Cosine similarity is multiplied by 100 for presentation; it is not a probability.

| Rank | Database Function   | Similarity |
| ---: | ------------------- | ---------: |
|    1 | bsearch             |      94.00% |
|    2 | lower_bound         |      88.56% |
|    3 | upper_bound         |      88.54% |
|    4 | linear_search       |      79.56% |
|    5 | binary_search       |      79.22% |
|    6 | count_value         |      66.47% |
|    7 | first_difference    |      64.71% |
|    8 | partition           |      64.56% |
|    9 | rotate_array        |      56.99% |
|   10 | clamp               |      56.60% |
|   11 | strchr              |      56.57% |
|   12 | remove_value        |      56.05% |
|   13 | heap_sort           |      55.76% |
|   14 | qsort               |      55.71% |
|   15 | copy_if             |      54.75% |
|   16 | sqrt                |      53.14% |
|   17 | memmove             |      53.09% |
|   18 | merge_sorted        |      53.00% |
|   19 | fill                |      51.99% |
|   20 | min                 |      51.97% |
|   21 | memcmp              |      51.63% |
|   22 | iota                |      51.08% |
|   23 | pow                 |      49.45% |
|   24 | next_power_of_two   |      49.21% |
|   25 | strstr              |      49.19% |
|   26 | gcd                 |      49.07% |
|   27 | memcpy              |      48.89% |
|   28 | average             |      48.87% |
|   29 | linear_interpolate  |      48.45% |
|   30 | abs                 |      48.09% |
|   31 | swap_ranges         |      47.84% |
|   32 | max                 |      47.61% |
|   33 | strlen              |      47.41% |
|   34 | unique              |      46.74% |
|   35 | is_prime            |      46.11% |
|   36 | lcm                 |      46.00% |
|   37 | evaluate_polynomial |      45.24% |
|   38 | sum_array           |      44.21% |
|   39 | is_sorted           |      44.03% |
|   40 | strcmp              |      43.08% |
|   41 | atoi                |      42.73% |
|   42 | rotate_right        |      42.64% |
|   43 | cos                 |      41.65% |
|   44 | rotate_left         |      41.03% |
|   45 | manhattan_distance  |      40.65% |
|   46 | fibonacci           |      40.48% |
|   47 | product_array       |      39.06% |
|   48 | factorial           |      37.92% |
|   49 | dot_product         |      33.03% |
|   50 | distance_squared    |      31.81% |
