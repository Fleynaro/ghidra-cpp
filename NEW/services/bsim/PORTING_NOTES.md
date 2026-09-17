# BSim Porting Notes

This file is the audit trail for the autonomous C++23 computational port. The
original Ghidra sources are the behavioral source of truth. Database, server,
GUI, and repository-management code is intentionally outside this service.

## Source Mapping

| Original Ghidra source/class/method | C++23 source/class/method | Adaptation |
| --- | --- | --- |
| `Ghidra/Features/BSim/src/main/java/ghidra/features/bsim/query/GenSignatures.java`, `SignatureTask.decompile` | `NEW/services/bsim/bsim_service.cppm`, `BsimService::generate_signature`; `NEW/services/bsim/bsim_decompiler_adapter.cppm`, `analyze_decompiler` | BSim validates and vectorizes the value-owned result produced by the existing Decompiler. It does not recreate the native graph algorithm. |
| `Ghidra/Features/Decompiler/src/main/java/ghidra/app/decompiler/DecompInterface.java`, `generateSignatures` | `NEW/services/decompiler/src/decompiler_impl.cppm`, native signature request; `NEW/services/bsim/bsim_decompiler_adapter.cppm` | The existing native decompiler owns normalization and signature generation; the adapter keeps the Java process protocol out of the C++ contract. |
| `Ghidra/Features/Decompiler/src/main/java/ghidra/app/decompiler/signature/SignatureResult.java`, `decode` | `NEW/core/contracts/bsim.cppm`, `FunctionSimilarityFeatures` | The returned value contains sorted `uint32_t` hashes, unimplemented/bad-data flags, and direct-call metadata. XML decoder ownership is removed. |
| `Ghidra/Features/Decompiler/src/decompile/cpp/signature.hh/.cc`, `Signature`, `SignatureEntry`, `BlockSignatureEntry`, `GraphSigManager` | `NEW/services/decompiler/src/signature.cppm`; `NEW/services/decompiler/src/decompiler_impl.cppm` | Delegated to the existing native C++ port. The former duplicate BSim modules (`bsim_signature.cppm`, `bsim_signature_entry.cppm`, `bsim_block_signature.cppm`, `bsim_graph_signature.cppm`) were removed. |
| `Ghidra/Framework/Generic/src/main/java/generic/lsh/vector/LSHVector.java` | `NEW/core/contracts/bsim.cppm`, `SimilarityVector`; `NEW/services/bsim/bsim_cosine_vector.cppm` | The public representation is value-only and sparse, with implementation-specific merge logic hidden in the service. |
| `LSHVectorFactory.java`, `WeightedLSHCosineVectorFactory.java` | `NEW/services/bsim/bsim_vector_factory.cppm`, `WeightedVectorFactory` | Factory settings, self-significance, comparison significance, and weighted vector creation are preserved. |
| `LSHCosineVector.java`, `installFeatures`, `compare`, `compareCounts`, `calcUniqueHash` | `NEW/services/bsim/bsim_cosine_vector.cppm`, `CosineVector` | Sorted feature runs, TF multiplicity, unsigned hash merge, lower-TF dot-product selection, zero/empty behavior, and CRC unique hash are retained. |
| `LSHCosineVectorAccum.java` | `NEW/services/bsim/bsim_cosine_vector.cppm`, `CosineVectorAccumulator` | Unsigned hash ordering, duplicate-key replacement semantics, lazy finalization, and post-finalization rejection are retained. |
| `HashEntry.java` | `NEW/services/bsim/bsim_hash_entry.cppm`, `HashEntry` | Internal TF is stored as count minus one and clamped to six bits; public counts remain one through 64. IDF is clamped to nine bits. |
| `WeightFactory.java` | `NEW/services/bsim/bsim_weight_factory.cppm`, `WeightFactory` | 512 IDF entries, 64 TF entries, logarithmic TF initialization, scale/addend, and normalized penalty parameters are ported. |
| `IDFLookup.java` | `NEW/services/bsim/bsim_idf.cppm`, `IDFLookup` | The two-times-power-of-two open-addressed table and absent-count-zero behavior are retained. |
| `VectorCompare.java` | `NEW/services/bsim/bsim_vector_compare.cppm`, `VectorCompare` | Dot product, multiplicity counts, intersection, flip count, and difference calculations are retained. |
| `Ghidra/Features/BSim/data/lshweights_32.xml`, `lshweights_64.xml`, `lshweights_64_32.xml`, `lshweights_nosize.xml`, `lshweights_cpool.xml` | `NEW/services/bsim/resources/lshweights_*.xml` | The original XML values are copied as independent resources. Resource selection follows `GenSignatures.getWeightsFile`; the C++ loader does not use Ghidra resource APIs. |

## Signature Settings

The native signature settings use `(modifiers << 2) | 1`. The default BSim
configuration is `0x49`, corresponding to `SIG_DONOTUSE_CONST` and
`SIG_COLLAPSE_INDNOISE`, plus the required check bit. `SIG_DONOTUSE_INPUT` is
intentionally not applied to the input marker in Ghidra’s `localHash`; BSim
does not reimplement that stage. The accepted mask is the one in
`GraphSigManager::testSettings`.

The native algorithm emits sorted 32-bit hashes, but does not make them
unique. Repeated hashes are term-frequency observations and are retained into
the vector layer.

## Vector Semantics

`HashEntry` stores a six-bit internal TF slot (`count - 1`, clamped at 63), a
nine-bit IDF slot (clamped at 511), and `idfweight[idf] * tfweight[tf]`.
`LSHCosineVector` coalesces already-sorted repeated hashes, computes length as
the square root of the coefficient-square sum, and counts multiplicity using
the public TF count. Comparison merges unsigned hash values. For a shared
hash, the coefficient of the vector with the lower TF contributes to the raw
dot product; ties select the second vector, matching the Java branch.

Cosine is zero when either vector has no entries. Otherwise the raw dot
product is divided by both lengths, including the original floating-point
behavior when a non-empty vector has a zero length. Significance is:

```text
dot - numflip * (flip0 + flip1 / max)
    - diff * (diff0 + diff1 / max) + addend
```

The copied XML resources contain the actual Ghidra weight and IDF values.
The service accepts an explicit resource directory and has a deterministic
built-in fallback for development environments where resources are absent;
the fallback is reported in the result metadata and is not presented as
Ghidra-compatible weight data.

## Omitted Infrastructure

The port deliberately omits `ProgramDB`, `DescriptionManager`, executable and
library records, SQL/H2/PostgreSQL storage, BSim server/query protocols, GUI
filters, call-graph persistence, and Java process management. Direct call
addresses are retained as metadata because they are part of the computational
signature result, but no database link is created.

## Compatibility Risks

Exact feature equality is provided by the existing native Decompiler, whose
signature request now follows Ghidra's `signature_ghidra.cc` lifecycle: it
captures `GraphSigManager` output immediately after `normalize`, then rebuilds
the function graph for ordinary decompiler artifacts. This prevents the full
decompiler action chain from contaminating the BSim graph while preserving the
normal C-output path for callers that request it. BSim receives the resulting
value-owned signature instead of inferring features from C text or machine
bytes. The fixture integration test
(`tests/bsim_similarity_integration_tests.cppm`) exercises the real PE Loader
-> Sleigh -> native Decompiler -> BSim path, validates semantic ranking, and
compares twelve weighted cosine pairs with the independent PyGhidra oracle in
(`tests/data/run_ghidra.py`).
