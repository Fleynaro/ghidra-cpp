# 07_clamp_variant

- Model: `hf.co/jinaai/jina-code-embeddings-1.5b-GGUF:BF16`
- Query example: `07_clamp_variant`
- Query instruction: `Find an equivalent code snippet given the following code snippet:`
- Expected function: `clamp`
- Expected rank: `1`
- Top match: `clamp` (95.65%)
- Top-1 vs top-2 gap: `10.09` percentage points
- Correct top-1 match: `Yes`

Cosine similarity is multiplied by 100 for presentation; it is not a probability.

| Rank | Database Function   | Similarity |
| ---: | ------------------- | ---------: |
|    1 | clamp               |      95.65% |
|    2 | min                 |      85.55% |
|    3 | max                 |      81.79% |
|    4 | upper_bound         |      65.86% |
|    5 | abs                 |      65.35% |
|    6 | partition           |      62.97% |
|    7 | lower_bound         |      61.41% |
|    8 | binary_search       |      59.62% |
|    9 | merge_sorted        |      59.22% |
|   10 | gcd                 |      57.85% |
|   11 | linear_interpolate  |      57.52% |
|   12 | qsort               |      56.14% |
|   13 | copy_if             |      55.97% |
|   14 | iota                |      55.89% |
|   15 | memmove             |      55.36% |
|   16 | memcmp              |      55.25% |
|   17 | swap_ranges         |      55.11% |
|   18 | linear_search       |      54.37% |
|   19 | fill                |      54.14% |
|   20 | lcm                 |      54.06% |
|   21 | remove_value        |      53.44% |
|   22 | bsearch             |      53.31% |
|   23 | pow                 |      53.17% |
|   24 | count_value         |      52.73% |
|   25 | is_prime            |      52.43% |
|   26 | atoi                |      52.28% |
|   27 | rotate_array        |      51.84% |
|   28 | memcpy              |      50.33% |
|   29 | heap_sort           |      49.64% |
|   30 | first_difference    |      49.38% |
|   31 | is_sorted           |      48.27% |
|   32 | rotate_left         |      48.20% |
|   33 | strcmp              |      48.01% |
|   34 | rotate_right        |      47.76% |
|   35 | strchr              |      47.71% |
|   36 | sqrt                |      47.34% |
|   37 | manhattan_distance  |      47.27% |
|   38 | strstr              |      46.91% |
|   39 | unique              |      45.28% |
|   40 | average             |      45.28% |
|   41 | next_power_of_two   |      44.78% |
|   42 | factorial           |      44.76% |
|   43 | strlen              |      44.67% |
|   44 | sum_array           |      44.34% |
|   45 | product_array       |      44.22% |
|   46 | evaluate_polynomial |      42.89% |
|   47 | cos                 |      40.55% |
|   48 | distance_squared    |      39.21% |
|   49 | dot_product         |      38.45% |
|   50 | fibonacci           |      36.22% |
