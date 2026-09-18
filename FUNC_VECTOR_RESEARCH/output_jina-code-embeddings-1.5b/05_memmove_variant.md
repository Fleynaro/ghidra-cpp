# 05_memmove_variant

- Model: `hf.co/jinaai/jina-code-embeddings-1.5b-GGUF:BF16`
- Query example: `05_memmove_variant`
- Query instruction: `Find an equivalent code snippet given the following code snippet:`
- Expected function: `memmove`
- Expected rank: `1`
- Top match: `memmove` (96.95%)
- Top-1 vs top-2 gap: `4.91` percentage points
- Correct top-1 match: `Yes`

Cosine similarity is multiplied by 100 for presentation; it is not a probability.

| Rank | Database Function   | Similarity |
| ---: | ------------------- | ---------: |
|    1 | memmove             |      96.95% |
|    2 | memcpy              |      92.03% |
|    3 | swap_ranges         |      77.44% |
|    4 | rotate_array        |      75.91% |
|    5 | merge_sorted        |      73.21% |
|    6 | fill                |      72.52% |
|    7 | iota                |      70.45% |
|    8 | partition           |      69.52% |
|    9 | remove_value        |      67.92% |
|   10 | heap_sort           |      65.63% |
|   11 | qsort               |      65.40% |
|   12 | strstr              |      63.96% |
|   13 | strchr              |      62.76% |
|   14 | memcmp              |      62.42% |
|   15 | upper_bound         |      62.10% |
|   16 | clamp               |      62.03% |
|   17 | lower_bound         |      60.91% |
|   18 | copy_if             |      60.48% |
|   19 | binary_search       |      59.74% |
|   20 | min                 |      59.03% |
|   21 | max                 |      57.35% |
|   22 | first_difference    |      55.79% |
|   23 | count_value         |      55.58% |
|   24 | linear_search       |      55.57% |
|   25 | unique              |      55.10% |
|   26 | strlen              |      54.92% |
|   27 | strcmp              |      54.83% |
|   28 | sum_array           |      53.36% |
|   29 | gcd                 |      53.33% |
|   30 | rotate_right        |      52.76% |
|   31 | rotate_left         |      51.89% |
|   32 | linear_interpolate  |      51.41% |
|   33 | bsearch             |      51.36% |
|   34 | lcm                 |      49.66% |
|   35 | evaluate_polynomial |      48.59% |
|   36 | pow                 |      47.64% |
|   37 | abs                 |      47.05% |
|   38 | average             |      46.87% |
|   39 | manhattan_distance  |      45.45% |
|   40 | is_sorted           |      45.12% |
|   41 | product_array       |      43.90% |
|   42 | dot_product         |      43.82% |
|   43 | is_prime            |      43.55% |
|   44 | next_power_of_two   |      43.43% |
|   45 | atoi                |      41.66% |
|   46 | factorial           |      40.78% |
|   47 | sqrt                |      38.69% |
|   48 | distance_squared    |      38.18% |
|   49 | fibonacci           |      37.62% |
|   50 | cos                 |      36.32% |
