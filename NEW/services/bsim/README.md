# BSim Service

This service ports Ghidra’s computational BSim pipeline: normalized function
graph, iterative data/control-flow signatures, sorted 32-bit feature hashes,
weighted sparse vector, and cosine/significance comparison. It contains no
Ghidra Java runtime, Python bridge, database, or server dependency.

## API

`recode.core.contracts.bsim` defines the independent value contract:

- `NormalizedFunction` is the normalized SSA/control-flow input produced by
  analysis/decompiler adapters.
- `IFunctionSimilarityService::generate_signature` returns sorted feature
  hashes and decompiler status metadata.
- `IFunctionSimilarityService::generate_vector` returns a sparse weighted
  `SimilarityVector`.
- `IFunctionSimilarityService::analyze` returns both stages.
- `IFunctionSimilarityService::compare` returns cosine, dot-product, counts,
  intersection, and normalized significance metadata.

The implementation is `recode.service.bsim` and is exposed as
`ReCode::BsimService`.

## Architecture

`bsim_service.cppm` implements the public service boundary. Signature work is
split across `bsim_signature.cppm`, `bsim_signature_entry.cppm`,
`bsim_block_signature.cppm`, and `bsim_graph_signature.cppm`. Vector work is
split across `bsim_hash_entry.cppm`, `bsim_weight_factory.cppm`,
`bsim_idf.cppm`, `bsim_vector_compare.cppm`, `bsim_cosine_vector.cppm`, and
`bsim_vector_factory.cppm`.

The service consumes normalized values from `NEW/core/domain/normalized_function.cppm`.
At runtime, callers should use the existing `NEW/services/sleigh` and
`NEW/services/decompiler` contracts to obtain that normalized representation;
BSim does not embed either implementation.

## Vectors

Features remain a sorted sequence, including duplicates. The vector coalesces
equal adjacent hashes into sparse entries while retaining term frequency,
looks up IDF, computes Ghidra’s TF/IDF coefficient, and normalizes cosine by
the Euclidean lengths. Weight resources are copied under `resources/` and are
selected using the architecture-size rules documented in
`PORTING_NOTES.md`.

## Build And Tests

Build the focused service with:

```text
NEW\build.bat bsim
```

Run the registered tests from the preserved build directory with:

```text
ctest --test-dir NEW\build -R bsim --output-on-failure
```

The fixture source and reproducible build wrapper are under `tests/data/`.
The tests cover exact hash primitives, graph signature edge cases, weighted
vector mathematics, determinism, resource loading, semantic function groups,
and the end-to-end PE Loader -> Sleigh -> native Decompiler -> BSim ranking
pipeline in `tests/bsim_similarity_integration_tests.cppm`.
