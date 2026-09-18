# Vector Function ID Results: qwen

Model: `qwen3-embedding:4b`
Query instruction: `(none; raw source)`
Passage instruction: `(none; raw source)`

| Example | Expected Function | Top Match | Similarity | Margin (pp) | Expected Rank | Correct? |
| ------- | ----------------- | --------- | ---------: | ----------: | ------------: | :------: |
| 01_bsearch_variant | bsearch | bsearch | 97.40% | 1.65 | 1 | Yes |
| 02_qsort_variant | qsort | heap_sort | 80.86% | 2.58 | 2 | No |
| 03_strlen_variant | strlen | strlen | 95.07% | 21.71 | 1 | Yes |
| 04_strcmp_variant | strcmp | strcmp | 93.29% | 5.47 | 1 | Yes |
| 05_memmove_variant | memmove | memmove | 97.02% | 4.92 | 1 | Yes |
| 06_gcd_variant | gcd | gcd | 95.95% | 3.26 | 1 | Yes |
| 07_clamp_variant | clamp | clamp | 97.12% | 18.45 | 1 | Yes |
| 08_NONE_prefix_sum | NONE | sum_array | 84.05% | 2.81 | N/A | No |
| 09_NONE_count_bits | NONE | next_power_of_two | 74.31% | 14.15 | N/A | No |
| 10_NONE_reverse | NONE | swap_ranges | 85.27% | 6.70 | N/A | No |

Similarity is cosine similarity converted to a percentage for presentation.
Margin is top-1 minus top-2 in percentage points; it measures ranking separation, not calibrated confidence.

## Retrieval Metrics

- Positive Top-1 accuracy: 6/7.
- Positive Top-3 accuracy: 7/7.
- Average positive expected rank: 1.14.
- Average positive Top-1 similarity: 93.81%.
- Unknown-example Top-1 similarity range: 74.31% to 85.27%.
- Overall Top-1 vs Top-2 margin range: 1.65 to 21.71 percentage points.

## Measured Observations

- Positive Top-1 similarity range: 80.86% to 97.40%.
- Unknown functions are reported as Top-1 misses because no rejection threshold is applied.
- Strongest unknown attraction: `10_NONE_reverse` -> `swap_ranges` at 85.27%.
- Narrowest Top-1/Top-2 separation: `01_bsearch_variant` at 1.65 percentage points.
- Widest Top-1/Top-2 separation: `03_strlen_variant` at 21.71 percentage points.
- These observations describe this dataset and run only; they do not establish general-purpose function identification performance.
