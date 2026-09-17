# BSim Service

This service ports Ghidra’s computational BSim vector pipeline: sorted 32-bit
feature hashes from the existing native Decompiler, weighted sparse vector,
and cosine/significance comparison. It contains no
Ghidra Java runtime, Python bridge, database, or server dependency.

## API

`recode.core.contracts.bsim` defines the independent value contract:

- `FunctionSimilarityFeatures` is the value-owned signature result produced by
  the native Decompiler.
- `IFunctionSimilarityService::generate_signature` validates and canonicalizes
  that sorted feature result.
- `IFunctionSimilarityService::generate_vector` returns a sparse weighted
  `SimilarityVector` from it.
- `IFunctionSimilarityService::analyze` returns both stages.
- `IFunctionSimilarityService::compare` returns cosine, dot-product, counts,
  intersection, and normalized significance metadata.

The implementation is `recode.service.bsim` and is exposed as
`ReCode::BsimService`.

## Architecture

`bsim_service.cppm` implements the public vector/service boundary.
`bsim_decompiler_adapter.cppm` is a thin adapter to the existing
`NEW/services/decompiler/src/signature.cppm` and
`NEW/services/decompiler/src/decompiler_impl.cppm`; it does not duplicate
GraphSigManager, SignatureEntry, or block hashing. Vector work is split across
`bsim_hash_entry.cppm`, `bsim_weight_factory.cppm`,
`bsim_idf.cppm`, `bsim_vector_compare.cppm`, `bsim_cosine_vector.cppm`, and
`bsim_vector_factory.cppm`.

At runtime, callers use the existing Sleigh and Decompiler services. The
Decompiler owns normalized SSA, GraphSigManager, signature settings, and
feature generation; BSim consumes only its value-owned result.

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
The tests cover weighted vector mathematics, feature-result validation,
determinism, resource loading, semantic function groups,
and the end-to-end PE Loader -> Sleigh -> native Decompiler -> BSim ranking
pipeline in `tests/bsim_similarity_integration_tests.cppm`.
