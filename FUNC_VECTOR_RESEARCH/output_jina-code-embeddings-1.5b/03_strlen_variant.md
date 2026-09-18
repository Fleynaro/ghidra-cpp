# 03_strlen_variant

- Model: `hf.co/jinaai/jina-code-embeddings-1.5b-GGUF:BF16`
- Query example: `03_strlen_variant`
- Query instruction: `Find an equivalent code snippet given the following code snippet:`
- Expected function: `strlen`
- Expected rank: `1`
- Top match: `strlen` (95.66%)
- Top-1 vs top-2 gap: `21.90` percentage points
- Correct top-1 match: `Yes`

Cosine similarity is multiplied by 100 for presentation; it is not a probability.

| Rank | Database Function   | Similarity |
| ---: | ------------------- | ---------: |
|    1 | strlen              |      95.66% |
|    2 | strchr              |      73.75% |
|    3 | strstr              |      67.76% |
|    4 | strcmp              |      66.89% |
|    5 | count_value         |      62.44% |
|    6 | unique              |      62.08% |
|    7 | atoi                |      61.75% |
|    8 | iota                |      59.88% |
|    9 | sum_array           |      59.69% |
|   10 | fill                |      59.31% |
|   11 | linear_search       |      58.25% |
|   12 | memcpy              |      56.60% |
|   13 | memcmp              |      56.52% |
|   14 | remove_value        |      56.19% |
|   15 | copy_if             |      56.11% |
|   16 | lower_bound         |      55.57% |
|   17 | first_difference    |      55.01% |
|   18 | upper_bound         |      54.53% |
|   19 | memmove             |      53.72% |
|   20 | next_power_of_two   |      53.26% |
|   21 | abs                 |      52.49% |
|   22 | max                 |      52.41% |
|   23 | partition           |      51.69% |
|   24 | min                 |      50.67% |
|   25 | gcd                 |      50.34% |
|   26 | lcm                 |      50.28% |
|   27 | binary_search       |      49.89% |
|   28 | bsearch             |      49.26% |
|   29 | linear_interpolate  |      48.74% |
|   30 | fibonacci           |      48.66% |
|   31 | average             |      48.64% |
|   32 | swap_ranges         |      47.29% |
|   33 | clamp               |      46.92% |
|   34 | evaluate_polynomial |      46.71% |
|   35 | manhattan_distance  |      46.65% |
|   36 | merge_sorted        |      46.34% |
|   37 | product_array       |      45.99% |
|   38 | factorial           |      45.77% |
|   39 | rotate_array        |      44.88% |
|   40 | is_prime            |      44.74% |
|   41 | pow                 |      44.55% |
|   42 | sqrt                |      43.73% |
|   43 | is_sorted           |      41.65% |
|   44 | dot_product         |      41.08% |
|   45 | distance_squared    |      40.87% |
|   46 | qsort               |      40.29% |
|   47 | cos                 |      40.24% |
|   48 | rotate_left         |      37.19% |
|   49 | heap_sort           |      37.16% |
|   50 | rotate_right        |      36.78% |
