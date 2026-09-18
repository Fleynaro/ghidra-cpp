# Vector Function ID Results: bsim

Model: `Ghidra BSim x64`
Query instruction: `(none; native Ghidra BSim signature)`
Passage instruction: `(none; native Ghidra BSim signature)`

| Example | Expected Function | Top Match | Similarity | Margin (pp) | Expected Rank | Correct? |
| ------- | ----------------- | --------- | ---------: | ----------: | ------------: | :------: |
| 01_bsearch_variant | bsearch | binary_search | 75.83% | 3.65 | 4 | No |
| 02_qsort_variant | qsort | swap_ranges | 44.23% | 2.58 | 29 | No |
| 03_strlen_variant | strlen | factorial | 28.61% | 0.05 | 2 | No |
| 04_strcmp_variant | strcmp | atoi | 29.71% | 11.03 | 11 | No |
| 05_memmove_variant | memmove | memmove | 67.66% | 24.06 | 1 | Yes |
| 06_gcd_variant | gcd | gcd | 44.40% | 3.07 | 1 | Yes |
| 07_clamp_variant | clamp | clamp | 59.73% | 15.65 | 1 | Yes |
| 08_NONE_prefix_sum | NONE | swap_ranges | 76.62% | 2.49 | N/A | No |
| 09_NONE_count_bits | NONE | strlen | 52.04% | 9.53 | N/A | No |
| 10_NONE_reverse | NONE | swap_ranges | 68.18% | 13.19 | N/A | No |

Similarity is cosine similarity converted to a percentage for presentation.
Margin is top-1 minus top-2 in percentage points; it measures ranking separation, not calibrated confidence.

## Retrieval Metrics

- Positive Top-1 accuracy: 3/7.
- Positive Top-3 accuracy: 4/7.
- Average positive expected rank: 7.00.
- Average positive Top-1 similarity: 50.02%.
- Unknown-example Top-1 similarity range: 52.04% to 76.62%.
- Overall Top-1 vs Top-2 margin range: 0.05 to 24.06 percentage points.

## Measured Observations

- Positive Top-1 similarity range: 28.61% to 75.83%.
- Unknown functions are reported as Top-1 misses because no rejection threshold is applied.
- Strongest unknown attraction: `08_NONE_prefix_sum` -> `swap_ranges` at 76.62%.
- Narrowest Top-1/Top-2 separation: `03_strlen_variant` at 0.05 percentage points.
- Widest Top-1/Top-2 separation: `05_memmove_variant` at 24.06 percentage points.
- These observations describe this dataset and run only; they do not establish general-purpose function identification performance.
