# Vector Function ID Results: jina

Model: `hf.co/jinaai/jina-code-embeddings-1.5b-GGUF:BF16`
Query instruction: `Find an equivalent code snippet given the following code snippet:`
Passage instruction: `Candidate code snippet:`

| Example | Expected Function | Top Match | Similarity | Margin (pp) | Expected Rank | Correct? |
| ------- | ----------------- | --------- | ---------: | ----------: | ------------: | :------: |
| 01_bsearch_variant | bsearch | bsearch | 94.00% | 5.44 | 1 | Yes |
| 02_qsort_variant | qsort | qsort | 91.68% | 3.65 | 1 | Yes |
| 03_strlen_variant | strlen | strlen | 95.66% | 21.90 | 1 | Yes |
| 04_strcmp_variant | strcmp | memcmp | 88.48% | 2.29 | 2 | No |
| 05_memmove_variant | memmove | memmove | 96.95% | 4.91 | 1 | Yes |
| 06_gcd_variant | gcd | gcd | 89.82% | 6.65 | 1 | Yes |
| 07_clamp_variant | clamp | clamp | 95.65% | 10.09 | 1 | Yes |
| 08_NONE_prefix_sum | NONE | iota | 88.09% | 3.20 | N/A | No |
| 09_NONE_count_bits | NONE | next_power_of_two | 80.25% | 18.23 | N/A | No |
| 10_NONE_reverse | NONE | swap_ranges | 89.32% | 4.40 | N/A | No |

Similarity is cosine similarity converted to a percentage for presentation.
Margin is top-1 minus top-2 in percentage points; it measures ranking separation, not calibrated confidence.

## Retrieval Metrics

- Positive Top-1 accuracy: 6/7.
- Positive Top-3 accuracy: 7/7.
- Average positive expected rank: 1.14.
- Average positive Top-1 similarity: 93.18%.
- Unknown-example Top-1 similarity range: 80.25% to 89.32%.
- Overall Top-1 vs Top-2 margin range: 2.29 to 21.90 percentage points.

## Measured Observations

- Positive Top-1 similarity range: 88.48% to 96.95%.
- Unknown functions are reported as Top-1 misses because no rejection threshold is applied.
- Strongest unknown attraction: `10_NONE_reverse` -> `swap_ranges` at 89.32%.
- Narrowest Top-1/Top-2 separation: `04_strcmp_variant` at 2.29 percentage points.
- Widest Top-1/Top-2 separation: `03_strlen_variant` at 21.90 percentage points.
- These observations describe this dataset and run only; they do not establish general-purpose function identification performance.
