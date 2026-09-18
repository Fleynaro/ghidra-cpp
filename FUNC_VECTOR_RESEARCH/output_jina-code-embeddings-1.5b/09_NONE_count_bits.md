# 09_NONE_count_bits

- Model: `hf.co/jinaai/jina-code-embeddings-1.5b-GGUF:BF16`
- Query example: `09_NONE_count_bits`
- Query instruction: `Find an equivalent code snippet given the following code snippet:`
- Expected function: `NONE`
- Expected rank: `N/A`
- Top match: `next_power_of_two` (80.25%)
- Top-1 vs top-2 gap: `18.23` percentage points
- Correct top-1 match: `No`

Cosine similarity is multiplied by 100 for presentation; it is not a probability.

| Rank | Database Function   | Similarity |
| ---: | ------------------- | ---------: |
|    1 | next_power_of_two   |      80.25% |
|    2 | rotate_right        |      62.03% |
|    3 | strlen              |      61.12% |
|    4 | rotate_left         |      60.82% |
|    5 | lower_bound         |      57.12% |
|    6 | upper_bound         |      56.69% |
|    7 | gcd                 |      56.62% |
|    8 | unique              |      55.16% |
|    9 | count_value         |      54.06% |
|   10 | fibonacci           |      53.91% |
|   11 | remove_value        |      51.42% |
|   12 | bsearch             |      50.90% |
|   13 | linear_search       |      50.86% |
|   14 | abs                 |      50.46% |
|   15 | fill                |      50.15% |
|   16 | iota                |      50.03% |
|   17 | pow                 |      49.96% |
|   18 | is_prime            |      49.90% |
|   19 | strchr              |      48.43% |
|   20 | atoi                |      48.43% |
|   21 | copy_if             |      48.18% |
|   22 | factorial           |      48.09% |
|   23 | sqrt                |      47.49% |
|   24 | lcm                 |      46.99% |
|   25 | partition           |      46.78% |
|   26 | max                 |      46.72% |
|   27 | min                 |      46.63% |
|   28 | binary_search       |      46.55% |
|   29 | memcpy              |      45.30% |
|   30 | sum_array           |      44.97% |
|   31 | strcmp              |      44.43% |
|   32 | rotate_array        |      44.35% |
|   33 | heap_sort           |      43.93% |
|   34 | first_difference    |      43.17% |
|   35 | memmove             |      41.81% |
|   36 | average             |      41.79% |
|   37 | cos                 |      41.37% |
|   38 | evaluate_polynomial |      41.35% |
|   39 | swap_ranges         |      40.96% |
|   40 | memcmp              |      40.73% |
|   41 | qsort               |      40.45% |
|   42 | strstr              |      40.06% |
|   43 | linear_interpolate  |      39.51% |
|   44 | clamp               |      39.51% |
|   45 | merge_sorted        |      39.28% |
|   46 | product_array       |      38.92% |
|   47 | manhattan_distance  |      37.06% |
|   48 | is_sorted           |      36.12% |
|   49 | dot_product         |      28.96% |
|   50 | distance_squared    |      28.63% |
