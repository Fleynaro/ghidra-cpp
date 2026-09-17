# Task: Implement an Autonomous C++23 Port of Ghidra BSim

Implement a complete, autonomous C++23 port of the relevant Ghidra BSim functionality in:

`NEW/services/bsim`

The goal is very specific:

> Given a function from our existing analysis/decompiler pipeline, generate the BSim feature representation, build the corresponding BSim vector, and return that vector through an independent service contract.

This must be a serious port of the original Ghidra implementation, not a simplified reimplementation and not a new algorithm inspired by BSim.

The original Ghidra implementation is the source of truth.

---

# 1. FIRST: Study the Original Ghidra Implementation

Before writing implementation code, thoroughly inspect the original Ghidra sources.

The most important Java sources are under:

```text
Ghidra/Features/BSim/src/main/java/
Ghidra/Framework/Generic/src/main/java/generic/lsh/
Ghidra/Features/Decompiler/src/main/java/ghidra/app/decompiler/
```

Start specifically with these areas/files:

```text
Ghidra/Features/BSim/src/main/java/ghidra/features/bsim/query/GenSignatures.java

Ghidra/Features/Decompiler/src/main/java/ghidra/app/decompiler/DecompInterface.java

Ghidra/Features/Decompiler/src/main/java/ghidra/app/decompiler/signature/SignatureResult.java
```

Then inspect the complete LSH/vector implementation:

```text
Ghidra/Framework/Generic/src/main/java/generic/lsh/
Ghidra/Framework/Generic/src/main/java/generic/lsh/vector/
```

In particular, locate and study the implementations/interfaces corresponding to:

```text
LSHVector
LSHVectorFactory
LSHCosineVector
LSHCosineVectorAccum
WeightedLSHCosineVectorFactory
WeightFactory
IDFLookup
HashEntry
VectorCompare
```

Also inspect every Java class that `GenSignatures` directly depends on for signature generation/vector construction.

Do NOT assume the files listed above are the complete dependency set.

Trace the call graph yourself.

---

# 2. VERY IMPORTANT: Study the Native Decompiler BSim Implementation Too

Although this task is primarily described as a Java → C++23 port, the actual Ghidra BSim feature generation is tightly connected to the native decompiler.

Therefore also inspect:

```text
Ghidra/Features/Decompiler/src/decompile/cpp/signature.hh
Ghidra/Features/Decompiler/src/decompile/cpp/signature.cc
Ghidra/Features/Decompiler/src/decompile/cpp/signature_ghidra.cc
```

These native files are extremely important.

Pay particular attention to:

```text
Signature
SignatureEntry
VarnodeSignature
BlockSignature
CopySignature
SigManager
GraphSigManager
SignaturesAt
```

and methods involved in:

```text
generate()
signatureIterate()
signatureBlockIterate()
collectVarnodeSigs()
collectBlockSigs()
initializeBlocks()
calculateShadow()
removeNoise()
noisePostOrder()
noiseDominator()
getSignatureVector()
```

Do not replace this algorithm with your own graph hashing algorithm.

Understand exactly what Ghidra does.

---

# 3. Create an Explicit Source Mapping Before Implementation

Before implementing the service, create:

```text
NEW/services/bsim/PORTING_NOTES.md
```

This document must contain a source mapping such as:

```text
Ghidra source
    ↓
NEW source

GenSignatures.java
    → bsim_signature_service.cppm

WeightedLSHCosineVectorFactory.java
    → bsim_vector_factory.cppm

LSHCosineVector.java
    → bsim_cosine_vector.cppm

...
```

For every substantial ported class/method, record:

1. Original Ghidra file.
2. Original class.
3. Original method.
4. New C++23 file/class/method.
5. Any intentional adaptation.
6. Why that adaptation was necessary.

This is important because the resulting implementation must remain auditable against Ghidra.

---

# 4. Source References Are Mandatory

Every ported implementation must contain comments referencing the original Ghidra source.

For example:

```cpp
// Ported from:
// Ghidra/Features/Decompiler/src/decompile/cpp/signature.cc
// Original: GraphSigManager::generate()
```

or:

```cpp
// Ported from:
// Ghidra/Framework/Generic/src/main/java/generic/lsh/vector/LSHCosineVector.java
// Original: LSHCosineVector::compare()
```

Do this systematically for the implementation.

Do not merely put one giant comment at the top saying "ported from Ghidra".

The correspondence must be discoverable at the method/class level.

---

# 5. Architecture: Independent Contracts

Our architecture is:

```text
NEW/core/domain
NEW/core/contracts
NEW/services
```

Services communicate through contracts.

BSim MUST NOT expose an API that depends directly on Ghidra classes.

Create an independent BSim contract under:

```text
NEW/core/contracts/
```

Use an appropriate name, for example:

```text
bsim_contract.cppm
```

or another name consistent with the existing repository conventions.

The contract must be implementation-independent.

It must NOT assume that BSim is the only possible function-similarity implementation.

We may implement other similarity algorithms later.

The contract should describe concepts such as:

```text
Function similarity feature extraction
Function vectorization
Similarity vector
Similarity result
```

without exposing Ghidra-specific implementation details.

---

# 6. Independent Domain Types

Place shared, implementation-independent domain entities under:

```text
NEW/core/domain/
```

Do NOT put BSim-specific internal implementation structures into `core/domain`.

The domain should contain only reusable concepts needed to communicate between services/runtime.

For example, depending on what the existing architecture already provides:

```text
Function
FunctionId
Address
NormalizedFunction
PCode operation representation
Varnode representation
BasicBlock representation
```

Reuse existing domain types where they already exist.

Do not duplicate existing contracts/entities.

If something is missing, add the smallest appropriate domain abstraction.

---

# 7. Reuse Existing Services

We already have:

```text
NEW/services/sleigh
NEW/services/decompiler
```

Use them.

Do NOT:

* embed another Sleigh implementation;
* embed another P-code decoder;
* copy the decompiler into BSim;
* invoke Ghidra Java at runtime;
* invoke Python/Ghidra scripts;
* require a running Ghidra process;
* make BSim dependent on Ghidra installation.

The intended architecture is:

```text
                    ┌─────────────────────┐
                    │     BSim Contract   │
                    └──────────┬──────────┘
                               │
                               ▼
                    ┌─────────────────────┐
                    │   BSim C++23        │
                    │      Service        │
                    └──────────┬──────────┘
                               │
                    ┌──────────┴──────────┐
                    ▼                     ▼
              Decompiler              Sleigh
```

Use the existing services through their public contracts.

---

# 8. Input / Output Must Be Normalized

The BSim service must have a clear input/output boundary.

Conceptually:

```text
Input:
    normalized/analyzable function

Output:
    BSim feature hashes
    BSim vector
    metadata required to interpret the result
```

The service should be callable without knowing anything about Ghidra's `Program`, `Function`, `ProgramDB`, `HighFunction`, `DecompInterface`, etc.

Those are implementation details of the original Ghidra architecture and must not leak into our contract.

---

# 9. Do NOT Port Ghidra's Database/Software Modeling Infrastructure

This is a critical requirement.

Ghidra contains a lot of infrastructure around BSim for:

* databases;
* BSim servers;
* PostgreSQL;
* H2;
* executable records;
* database queries;
* BSim repository management;
* GUI;
* search UI;
* persistence;
* Software Modeling;
* BSim query infrastructure.

Do NOT blindly port those parts.

We need the autonomous computational core.

The desired flow is:

```text
Function
    ↓
BSim feature generation
    ↓
feature hashes
    ↓
BSim weighting/vectorization
    ↓
BSim vector
```

Database/search-server infrastructure is outside the scope of this service.

Do not introduce unnecessary database dependencies.

---

# 10. C++23 Module Structure

This project uses C++23 modules.

Do NOT create:

```text
.h
.hpp
.cpp
```

for the implementation.

Use `.cppm` modules.

However, there is an important requirement:

> Declarations and implementations must be logically separated inside the module architecture.

Do not create one gigantic `.cppm` containing every class and every implementation.

Prefer a structure like:

```text
NEW/services/bsim/

    bsim_contract_adapter.cppm

    bsim_signature.cppm
    bsim_signature_entry.cppm
    bsim_graph_signature.cppm
    bsim_block_signature.cppm
    bsim_varnode_signature.cppm
    bsim_copy_signature.cppm

    bsim_vector.cppm
    bsim_cosine_vector.cppm
    bsim_vector_factory.cppm
    bsim_weight_factory.cppm
    bsim_idf.cppm

    bsim_service.cppm

    tests/
        ...
```

Adapt the exact structure to the repository conventions.

Each class should have its implementation in the appropriate module implementation section/module unit rather than stuffing unrelated classes together.

No header files.

No traditional `.cpp` files.

No artificial "header-only" implementation where every declaration and implementation is dumped into one massive module.

---

# 11. Preserve the Original Algorithm

This is one of the most important requirements.

Do NOT simplify the algorithm.

Do NOT replace:

```text
GraphSigManager
```

with:

```text
hash(function bytes)
```

Do NOT replace it with:

```text
hash(P-code text)
```

Do NOT replace it with:

```text
hash(opcodes)
```

Do NOT replace it with a neural embedding.

Do NOT replace it with an arbitrary graph hash.

The objective is Ghidra-compatible BSim behavior.

Preserve:

* signature construction;
* Varnode signatures;
* block signatures;
* COPY signatures;
* iterative graph hashing;
* noise handling;
* shadow calculation;
* feature collapsing;
* commutative handling;
* input/constant/persistent-node handling;
* signature settings;
* feature sorting;
* duplicate feature semantics;
* feature weighting;
* TF handling;
* IDF handling;
* cosine similarity;
* confidence-related calculations where they belong to the vector layer.

If a piece of original code appears unnecessary, first prove that it is unnecessary.

Do not remove functionality merely because it is inconvenient to port.

---

# 12. Pay Special Attention to Feature Semantics

The output of the signature-generation stage is conceptually:

```text
sorted sequence of uint32 feature hashes
```

Duplicate hashes are meaningful.

For example:

```text
[h1, h1, h1, h2, h3, h3]
```

must NOT automatically become:

```text
[h1, h2, h3]
```

because frequency information is used by the vectorization layer.

Preserve the exact semantics of the original implementation.

---

# 13. Preserve BSim Weighting

Do not implement similarity as merely:

```text
cosine(feature-count-vector-A, feature-count-vector-B)
```

unless that is exactly what the relevant Ghidra implementation does.

Inspect and port the actual:

```text
LSHVector
LSHCosineVector
WeightedLSHCosineVectorFactory
WeightFactory
IDFLookup
```

behavior.

Determine exactly:

* how TF is calculated;
* how IDF is obtained;
* how feature weights are applied;
* how sparse vectors are represented;
* how vector normalization is performed;
* how cosine is calculated;
* how duplicate features affect weights;
* what numerical precision is used;
* how zero vectors are handled.

Document these findings in `PORTING_NOTES.md`.

---

# 14. Weight Data / XML Resources

Locate the BSim weight resources in Ghidra, including files such as:

```text
lshweights_32.xml
lshweights_64.xml
lshweights_nosize.xml
lshweights_cpool.xml
```

or their current equivalents in the exact Ghidra revision being studied.

Determine:

1. Which resources are actually used.
2. How Ghidra locates them.
3. How they are parsed.
4. How architecture affects selection.
5. Whether the service can load them independently of Ghidra installation.

Make the C++23 service self-contained.

Do not require Java resource loading.

If necessary, copy/transform the required data into an appropriate NEW resource format, while preserving the original values and semantics.

Document the provenance.

---

# 15. Tests Are Mandatory and Extensive

Testing is a first-class part of this task.

Do not consider the implementation finished until the tests pass.

We want significantly more than a few unit tests.

Create:

```text
NEW/services/bsim/tests/
```

and follow the testing conventions already used elsewhere in the repository.

Use Google Test.

---

# 16. Create a Real C++ Fixture Binary

Create:

```text
NEW/services/bsim/tests/data/
```

Inside it create a standalone C++ source file containing **30 utility functions**.

For example:

```text
qsort-like implementations
bsearch-like implementations
max/min
absolute value
clamp
swap
reverse
linear search
binary search
array sum
array product
find first
find last
count
memcpy-like loop
memcmp-like loop
string length
string compare
case conversion
hash function
checksum
rotate
bit counting
power
gcd
lcm
sorting helpers
partition
merge helper
etc.
```

These must be actual independently implemented functions.

Do NOT call the standard library implementation.

Write the algorithms yourself.

Each function must have a comment explaining what it does.

Example:

```cpp
// Sorts an integer array in ascending order using an in-place
// quicksort partitioning strategy.
extern "C" void qsort_impl_1(...);
```

---

# 17. The 30 Functions Must Include Semantic Twins

This is extremely important.

Some functions must implement essentially the same behavior but be written differently.

For example:

```text
qsort_impl_1
qsort_impl_2
qsort_impl_3
```

may all sort an array but use different control structures.

Likewise:

```text
max_impl_1
max_impl_2
```

could use different expressions/control flow.

Create several groups of semantic twins.

The source code should differ substantially enough that the compiler generates potentially different low-level representations, while the underlying operation remains similar.

Also include unrelated functions.

The fixture should therefore contain:

```text
semantic twins
near-twins
related utilities
unrelated utilities
very small functions
medium functions
more complicated functions
```

This is intended to test whether BSim actually captures structural/semantic similarity instead of merely textual similarity.

---

# 18. Fixture Build Script

Create:

```text
NEW/services/bsim/tests/data/build.bat
```

It must compile the fixture into an `.exe`.

The test suite should consume this executable as a fixture.

Do not make the test suite dependent on manually compiling it.

The build/test process should make it obvious how the fixture is produced.

Follow the repository's existing fixture conventions, especially similar tests such as:

```text
NEW/services/debugger/win_ttd/tests/data
```

---

# 19. Integration Test Pipeline

The BSim tests should exercise as much of the real pipeline as possible:

```text
fixture.exe
    ↓
PE Loader / existing analysis infrastructure
    ↓
Sleigh
    ↓
Decompiler
    ↓
normalized function
    ↓
BSim
    ↓
feature hashes
    ↓
vector
```

Do not construct fake P-code graphs for all integration tests.

Unit tests may use synthetic inputs where appropriate, but integration tests must exercise the real services.

---

# 20. Similarity Test

Create a test where one selected function is compared against all relevant fixture functions.

For example:

```text
qsort_impl_2
```

is compared against:

```text
qsort_impl_1
qsort_impl_3
max_impl_1
...
```

Collect:

```text
function name
similarity
vector statistics
```

Sort the results for diagnostic output.

The expected semantic twin:

```text
qsort_impl_1
```

should produce the strongest similarity among the intentionally designed candidates.

Do NOT hardcode arbitrary similarity thresholds before actually inspecting the results.

First run the implementation, inspect the distribution, then establish justified assertions.

Tests should verify both:

1. expected semantic similarity;
2. expected separation from unrelated functions.

---

# 21. Add Multiple Similarity Groups

Do not stop at qsort.

Create several families, for example:

```text
sorting:
    qsort_impl_1
    qsort_impl_2
    qsort_impl_3

search:
    bsearch_impl_1
    bsearch_impl_2
    bsearch_impl_3

max:
    max_impl_1
    max_impl_2

absolute value:
    abs_impl_1
    abs_impl_2

checksum/hash:
    checksum_impl_1
    checksum_impl_2
```

Test representative pairs across these groups.

This prevents the implementation from accidentally passing because of one lucky example.

---

# 22. Unit Tests for Signature Generation

Create detailed tests for the internal signature algorithm.

Test at minimum:

* empty/minimal functions where applicable;
* constants;
* inputs;
* outputs;
* COPY;
* LOAD;
* STORE;
* arithmetic;
* logical operations;
* comparisons;
* branches;
* calls;
* returns;
* multiple basic blocks;
* loops;
* conditional branches;
* PHI/MULTIEQUAL-like data flow;
* commutative operations;
* repeated operations;
* different operand order;
* dead/noise operations;
* persistent/global values;
* feature collapsing;
* duplicate features;
* signature sorting;
* deterministic output.

Where appropriate, construct synthetic P-code/graph inputs for precise unit tests.

---

# 23. Determinism Tests

For the same function:

```text
generate(function)
generate(function)
generate(function)
```

must produce identical feature hashes and identical vectors.

Test this explicitly.

Also test that changing irrelevant metadata does not unexpectedly alter the signature if Ghidra ignores that metadata.

---

# 24. Regression Tests

Whenever a bug is discovered during implementation, add a regression test.

Do not merely fix the implementation.

The test suite should permanently capture the discovered behavior.

---

# 25. Numerical Tests

For vectorization and cosine similarity test:

* identical vectors;
* orthogonal vectors;
* partially overlapping vectors;
* duplicate features;
* heavily repeated features;
* zero vector;
* one-element vector;
* very sparse vector;
* large feature counts;
* floating-point precision boundaries.

Use tolerances appropriate to the original implementation.

Do not use overly loose assertions just to make tests pass.

---

# 26. Performance Tests / Diagnostics

BSim may eventually be run over very large binaries.

Add basic timing diagnostics around:

```text
feature generation
vectorization
similarity
```

Do not prematurely optimize.

First make the implementation correct.

If something is unexpectedly slow, report the bottleneck in test output or a short diagnostic document.

---

# 27. No Fake Passing Tests

This is critical.

Do NOT create tests where the expected result is derived from the implementation itself.

Bad:

```cpp
auto expected = implementation.generate(...);
EXPECT_EQ(actual, expected);
```

Good:

```cpp
EXPECT_EQ(actual.features, known_expected_features);
```

or:

```cpp
EXPECT_LT(distance_to_expected, threshold);
```

or:

```cpp
EXPECT_GT(similar_function_score, unrelated_function_score);
```

The test must be capable of catching an incorrect implementation.

---

# 28. Prefer Differential Validation Against Ghidra

Where practical, use the original Ghidra implementation as an oracle during development.

For representative fixture functions, obtain:

```text
Ghidra feature hashes
Ghidra vector representation
```

and compare them with the C++23 implementation.

If necessary, create a temporary helper/script under the test infrastructure to extract Ghidra's results.

The final runtime service must remain autonomous, but development-time differential testing against Ghidra is highly desirable.

For exact signature hashes, aim for:

```text
Ghidra output == NEW BSim output
```

not merely "similar".

If exact equality is impossible due to a documented architectural adaptation, explain precisely why.

---

# 29. No Silent Deviations

If you encounter an original Ghidra dependency that cannot be directly ported:

DO NOT silently replace it.

Instead:

1. identify it;
2. understand its purpose;
3. determine whether it is algorithmically relevant;
4. find the smallest autonomous equivalent;
5. document the difference;
6. add tests covering the behavior.

The final implementation should contain as few semantic deviations as possible.

---

# 30. Expected Service API

The final service should expose a clean contract along the lines of:

```text
generateSignature(function)
generateVector(function)
```

or an equivalent API consistent with the repository architecture.

The important property is:

```text
Function → BSim result
```

with no Ghidra Java classes exposed.

The caller should not need to know about:

```text
GenSignatures
DecompInterface
ProgramDB
DescriptionManager
BSim database
PostgreSQL
H2
```

---

# 31. Expected Internal Pipeline

The implementation should conceptually look like:

```text
BSim Contract
      │
      ▼
BSim Service
      │
      ▼
Normalized Function
      │
      ▼
Signature Generation
      │
      ├── Varnode signatures
      ├── Block signatures
      ├── Copy signatures
      ├── Noise handling
      └── Iterative hashing
      │
      ▼
Sorted uint32 feature hashes
      │
      ▼
Weight / TF / IDF processing
      │
      ▼
Sparse weighted vector
      │
      ▼
BSim result
```

Keep these stages separable internally.

---

# 32. Repository Conventions

Before adding files, inspect existing modules in:

```text
NEW/core
NEW/services
```

and especially recently implemented services.

Follow their conventions for:

* module names;
* namespaces;
* exports;
* imports;
* dependency direction;
* tests;
* CMake integration;
* naming;
* error handling;
* result types;
* documentation;
* fixtures.

Do not invent a completely different project style.

---

# 33. Build Integration

Integrate the service into the existing CMake/build system.

The entire project must build successfully.

The BSim tests must be buildable and runnable independently.

Ensure:

```text
C++23
modules
Google Test
existing Sleigh service
existing Decompiler service
```

work together.

Do not introduce unnecessary third-party dependencies.

---

# 34. Final Required Documentation

Create/update:

```text
NEW/services/bsim/README.md
NEW/services/bsim/PORTING_NOTES.md
```

README must explain:

* what BSim does;
* input;
* output;
* service API;
* architecture;
* how it interacts with Decompiler/Sleigh;
* how vectors are represented;
* how similarity is calculated;
* how tests are executed.

`PORTING_NOTES.md` must explain:

* exact Ghidra source files studied;
* source-to-source mapping;
* ported classes;
* ported methods;
* intentional adaptations;
* omitted infrastructure;
* weight resources;
* compatibility considerations;
* known limitations, if any.

Do not claim 1:1 compatibility unless the relevant behavior has actually been validated.

---

# 35. Completion Criteria

Do NOT stop after creating the files.

The task is complete only when:

1. All relevant original Ghidra BSim sources have been studied.
2. The source mapping is documented.
3. Independent BSim contracts exist under `NEW/core/contracts`.
4. Shared domain entities are under `NEW/core/domain`.
5. BSim implementation lives under `NEW/services/bsim`.
6. Implementation uses C++23 modules.
7. There are no `.h` / `.hpp` / `.cpp` implementation files for the BSim service.
8. Implementations are properly separated into appropriate module units/modules.
9. Original Ghidra source references exist in the ported implementation.
10. The implementation does not depend on Ghidra Java/Python/database infrastructure.
11. Existing Sleigh/Decompiler services are reused.
12. A real 30-function C++ fixture exists.
13. `build.bat` produces the fixture executable.
14. Google Tests cover unit behavior extensively.
15. Google Tests cover real end-to-end fixture functions.
16. Multiple semantic-twin groups are tested.
17. Similarity rankings are validated.
18. Determinism is tested.
19. Vector mathematics is tested.
20. Feature generation is tested.
21. The complete build succeeds.
22. All BSim tests pass.
23. Tests are actually executed after implementation.
24. Any failures are investigated and fixed rather than ignored.
25. Final output is documented.

---

# 36. Working Strategy

Use the following workflow:

```text
PHASE 1
Study Ghidra
    ↓
Trace dependencies
    ↓
Create PORTING_NOTES.md

PHASE 2
Inspect existing NEW architecture
    ↓
Define domain objects
    ↓
Define independent BSim contract

PHASE 3
Port signature-generation algorithm
    ↓
Unit tests
    ↓
Validate

PHASE 4
Port vector/weight/cosine layer
    ↓
Unit tests
    ↓
Validate

PHASE 5
Create 30-function fixture
    ↓
Compile fixture
    ↓
Run real Sleigh/Decompiler/BSim pipeline

PHASE 6
Compare semantic twins
    ↓
Investigate false positives
    ↓
Fix implementation
    ↓
Add regression tests

PHASE 7
Full build
    ↓
Run entire BSim test suite
    ↓
Review source mapping
    ↓
Review architecture
    ↓
Final report
```

Do not skip directly to implementation.

---

# 37. Final Report

At the end, provide a concise but technically detailed report containing:

```text
Files created
Files modified
Original Ghidra files ported
Classes/methods ported
Architecture
Contract API
Test count
Fixture functions
Ghidra-vs-NEW differential results
Similarity examples
Build command
Test command
Known deviations
Known limitations
Performance observations
```

Most importantly:

> Keep working until the implementation builds and the complete BSim test suite passes.

Do not stop at "implementation complete" if tests are failing.
Do not weaken assertions merely to get green tests.
Do not remove functionality from Ghidra merely to simplify the port.
Do not replace BSim with a simpler algorithm.

The target is an autonomous, auditable C++23 implementation of the actual Ghidra BSim computational pipeline.
