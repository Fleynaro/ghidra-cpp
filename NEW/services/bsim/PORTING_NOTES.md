# BSim Porting Notes

This file is the audit trail for the autonomous C++23 computational port. The
original Ghidra sources are the behavioral source of truth. Database, server,
GUI, and repository-management code is intentionally outside this service.

## Source Mapping

| Original Ghidra source/class/method | C++23 source/class/method | Adaptation |
| --- | --- | --- |
| `Ghidra/Features/BSim/src/main/java/ghidra/features/bsim/query/GenSignatures.java`, `SignatureTask.decompile` | `NEW/services/bsim/bsim_service.cppm`, `BsimService::generate_signature` | Accepts a normalized value graph instead of `Program`/`Function`/`DecompInterface`; this keeps the service independent of Java and database objects while preserving the signature pipeline. |
| `Ghidra/Features/Decompiler/src/main/java/ghidra/app/decompiler/DecompInterface.java`, `generateSignatures` | `NEW/core/contracts/bsim.cppm`, `IFunctionSimilarityService::generate_signature`; `NEW/services/bsim/bsim_service.cppm` | The existing native decompiler remains the producer of normalized analysis data. The serialized Java process protocol is not needed at the service boundary. |
| `Ghidra/Features/Decompiler/src/main/java/ghidra/app/decompiler/signature/SignatureResult.java`, `decode` | `NEW/core/contracts/bsim.cppm`, `FunctionSimilarityFeatures` | The returned value contains sorted `uint32_t` hashes, unimplemented/bad-data flags, and direct-call metadata. XML decoder ownership is removed. |
| `Ghidra/Features/Decompiler/src/decompile/cpp/signature.hh`, `Signature` | `NEW/services/bsim/bsim_signature.cppm`, `SignatureFeature` | Feature hashes remain 32-bit values and are sorted as unsigned values. |
| `Ghidra/Features/Decompiler/src/decompile/cpp/signature.cc`, `hash_mixin` and `crc_update` | `NEW/services/bsim/bsim_crc32.cppm`, `hash_mixin`; `NEW/services/bsim/bsim_signature_entry.cppm` | The reflected CRC-32 update and eight-round 64-bit mix are ported with explicit fixed-width wraparound. |
| `signature.cc`, `SignatureEntry::calculateShadow` | `NEW/services/bsim/bsim_signature_entry.cppm`, `SignatureEntry::calculate_shadow` | COPY/INDIRECT/CAST shadow chains are followed exactly when indirect-noise collapsing is disabled. |
| `signature.cc`, `SignatureEntry::removeNoise`, `noisePostOrder`, `noiseDominator` | `NEW/services/bsim/bsim_signature_entry.cppm`, `SignatureEntry::remove_noise` | The marker graph, virtual root, iterative immediate-dominator pass, and path compression are retained over normalized SSA IDs. |
| `signature.cc`, `SignatureEntry::localHash`, `getOpHash`, `standaloneCopyHash` | `NEW/services/bsim/bsim_signature_entry.cppm` | Constants, inputs, persistent values, annotations, operand conventions, CPOOLREF tags, and stand-alone copies retain Ghidra’s constants and modifier behavior. |
| `signature.cc`, `SignatureEntry::hashIn` | `NEW/services/bsim/bsim_signature_entry.cppm`, `SignatureEntry::hash_in` | Non-commutative inputs are ordered; commutative inputs are mixed through the additive accumulator used by Ghidra. |
| `signature.cc`, `BlockSignatureEntry::localHash`, `hashIn` | `NEW/services/bsim/bsim_block_signature.cppm`, `BlockSignatureEntry` | CFG predecessor/successor counts and conditional-edge markers are represented by normalized basic-block IDs. |
| `signature.cc`, `GraphSigManager::setCurrentFunction` | `NEW/services/bsim/bsim_graph_signature.cppm`, `GraphSignatureGenerator::prepare` | The native `Funcdata` overlay is replaced by a value-owned normalized graph; no native pointer or Ghidra class crosses the contract. |
| `signature.cc`, `GraphSigManager::signatureIterate` | `NEW/services/bsim/bsim_graph_signature.cppm`, `GraphSignatureGenerator::iterate_varnodes` | Iterations use a previous/current hash pair and preserve synchronous update ordering. |
| `signature.cc`, `GraphSigManager::signatureBlockIterate` | `NEW/services/bsim/bsim_graph_signature.cppm`, `GraphSignatureGenerator::iterate_blocks` | Incoming block order and conditional edge direction are preserved. |
| `signature.cc`, `GraphSigManager::collectVarnodeSigs` | `NEW/services/bsim/bsim_graph_signature.cppm`, `GraphSignatureGenerator::collect_varnode_features` | Duplicate feature hashes are retained. |
| `signature.cc`, `GraphSigManager::collectBlockSigs` | `NEW/services/bsim/bsim_graph_signature.cppm`, `GraphSignatureGenerator::collect_block_features` | Root operations, overlapping pairs, call accumulation, stand-alone COPY accumulation, and control-flow features are ported. |
| `signature.cc`, `GraphSigManager::generate` | `NEW/services/bsim/bsim_graph_signature.cppm`, `GraphSignatureGenerator::generate` | Default three varnode rounds and one block round are retained; settings and iteration limits are explicit options. |
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
intentionally not applied to the input marker in Ghidra’s `localHash` and is
not applied here. The accepted mask is the one in
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

Exact feature equality requires the input to be normalized SSA p-code with the
same Varnode flags, operation input conventions, marker classification, and
CFG edge order as Ghidra. The independent boundary therefore records these
facts explicitly instead of inferring them from C text or machine bytes. The
fixture integration test (`tests/bsim_similarity_integration_tests.cppm`) now
exercises the real PE Loader -> Sleigh -> native Decompiler -> BSim path and
validates semantic ranking. Differential tests can compare the resulting
sorted feature arrays when both pipelines receive equivalent normalized
graphs.
