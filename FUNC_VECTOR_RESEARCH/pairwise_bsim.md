# Pairwise Similarity: bsim

- Model: `Ghidra BSim x64`
- Database functions: `50`
- Pairwise comparisons: `1225`
- Values are weighted Ghidra BSim cosine similarities; no threshold or classification was applied.

## Basic Statistics

- Maximum off-diagonal similarity: `1.000000`
- Minimum off-diagonal similarity: `0.000000`
- Mean pairwise similarity: `0.142548`

## Top 20 Most Similar Distinct-Function Pairs

| Rank | Function A | Function B | Cosine Similarity |
| ---: | ---------- | ---------- | ----------------: |
|    1 | max | min |         1.000000 |
|    2 | copy_if | remove_value |         0.921733 |
|    3 | fill | iota |         0.876259 |
|    4 | distance_squared | dot_product |         0.818188 |
|    5 | product_array | sum_array |         0.793795 |
|    6 | count_value | remove_value |         0.774446 |
|    7 | lower_bound | upper_bound |         0.771520 |
|    8 | rotate_left | rotate_right |         0.767965 |
|    9 | fill | product_array |         0.736656 |
|   10 | fill | sum_array |         0.736656 |
|   11 | binary_search | lower_bound |         0.718483 |
|   12 | dot_product | swap_ranges |         0.708055 |
|   13 | iota | product_array |         0.706694 |
|   14 | iota | sum_array |         0.706694 |
|   15 | fill | swap_ranges |         0.685266 |
|   16 | copy_if | count_value |         0.681172 |
|   17 | distance_squared | swap_ranges |         0.678445 |
|   18 | dot_product | product_array |         0.668171 |
|   19 | dot_product | sum_array |         0.668171 |
|   20 | iota | swap_ranges |         0.657395 |
