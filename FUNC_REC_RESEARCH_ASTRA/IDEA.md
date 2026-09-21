You are GPT-6 Astra acting as a **research scientist, reverse-engineering specialist, compiler/IR expert, program-analysis researcher, and algorithm designer**.

Your task is **NOT to write production code**.

Your task is to perform a deep theoretical and algorithmic research effort and produce a detailed Markdown research/implementation plan for a next-generation binary function recognizer.

The final deliverable must be a single detailed Markdown document:

`RESEARCH_PLAN.md`

This document will later be given to a cheaper coding model, which will implement the proposed research prototype in Python.

---

# 1. The actual research goal

We want to build a highly accurate function recognizer whose primary purpose is recognizing:

* inline functions,
* library functions,
* utility functions,
* compiler-transformed/inlined implementations,
* wrapper/thunk-like functions,
* ordinary non-inline functions,
* nested/callee functions inside larger functions,
* functions containing calls to external library functions,
* functions containing calls to ordinary internal functions,
* combinations of all of the above.

The primary target workload is **GTA5.exe**, but the architecture should generalize to arbitrary libraries and binaries.

Ultimately we want to be able to build databases for libraries such as:

* Windows / Kernel32 / NTDLL / user-mode system libraries,
* CRT,
* C/C++ runtime,
* Boost,
* Steam API,
* Bullet Physics,
* RakNet,
* networking libraries,
* graphics/audio libraries,
* custom proprietary libraries,
* game engine libraries,
* and arbitrary third-party codebases.

The long-term goal is extremely ambitious:

> Given low-level P-Code for a function or larger code region, recognize the semantic functions contained in it — including aggressively optimized/inlined functions — with extremely high precision and extremely low false-positive rates.

The recognizer should primarily behave as a **classifier**, not merely as a signature matcher.

Precision is more important than aggressively recognizing everything.

---

# 2. Very important: challenge the initial hypothesis

Below is our current research hypothesis.

It is NOT a fixed architecture.

You are explicitly allowed — and encouraged — to:

* replace parts of it,
* generalize it,
* simplify it,
* combine it with existing academic/industrial techniques,
* reject parts that are fundamentally weak,
* invent better techniques,
* introduce new representations,
* use several cooperating algorithms,
* use staged or hierarchical classification,
* use adaptive probing,
* use probabilistic/hypothesis-based reasoning,
* use symbolic execution where justified,
* use abstract interpretation where useful,
* use SMT/SAT techniques where appropriate,
* use existing sub-solutions when a subproblem is already well solved.

Do not blindly implement our ideas just because they are listed below.

Instead:

1. Understand the underlying goal.
2. Identify the actual mathematical/computational problem.
3. Identify where our hypothesis is weak.
4. Replace weak parts with better approaches.
5. Preserve useful ideas where they are genuinely advantageous.
6. Explicitly explain why your proposed solution is better.

This is a **research project**, not an implementation specification.

---

# 3. Core hypothesis we currently have

The central idea is:

> Function recognition can be treated as behavioral identification by executing P-Code and comparing observable output states against a database of known functions.

Conceptually:

`Function / Region = transition from an input state to an observable output state`

For example:

`X -> F(X) -> Y`

For a sequence of probes:

`F(X1), F(X2), ..., F(XM)`

we can define a behavioral fingerprint:

`Fingerprint(F) = [F(X1), F(X2), ..., F(XM)]`

However, this simple idea is intentionally incomplete.

You must investigate how to make it robust against:

* unknown argument layout,
* pointers,
* aliasing,
* memory effects,
* loops,
* branches,
* architecture-specific behavior,
* registers,
* flags,
* multiple outputs,
* hidden inputs,
* global state,
* context dependencies,
* library calls,
* nested callees,
* compiler optimizations,
* inlining,
* dead code,
* constant propagation,
* partial evaluation,
* instruction reordering,
* transformed calling conventions,
* merged/split computations,
* and finite-probe collisions.

You should determine whether the correct abstraction is really `X -> Y`, or a richer transition model such as:

`State_in -> ObservableState_out`

or some hierarchical/partial-state representation.

---

# 4. Input to the recognizer

The critical design requirement is:

> The recognizer receives **low-level P-Code for a function F**.

That is the primary input.

We do NOT want a dependency on a full Ghidra ProgramDB or a traditional decompiler framework.

P-Code is the fundamental input representation.

The research prototype should conceptually be able to operate as:

`Low P-Code(F)`
→ `IR`
→ `program analysis`
→ `candidate region discovery`
→ `input/output hypothesis generation`
→ `behavioral execution`
→ `candidate matching`
→ `function classification`

The system must recognize both:

1. The function F itself.
2. Functions/regions embedded inside F, including inline functions.

Therefore, a normal non-inline function is simply a special case where the recognizable region is effectively the whole function F.

Do NOT design two unrelated recognizers for “normal functions” and “inline functions”.

Design a unified model.

---

# 5. Build a proper IR from low P-Code

One of the most important parts of the project is the intermediate representation.

We effectively need to build a small custom decompiler/program-analysis layer.

The goal is NOT necessarily to produce beautiful C code.

The goal is to produce an analysis-friendly IR that makes it easy to reason about:

* data flow,
* control flow,
* SSA,
* memory dependencies,
* types/roles,
* regions,
* calls,
* side effects,
* inputs,
* outputs,
* aliases,
* and behavioral execution.

Investigate what the best IR should look like.

Potentially combine:

* CFG,
* SSA,
* memory SSA,
* use-def / def-use chains,
* dominators/post-dominators,
* loop structure,
* call graph,
* data-flow graph,
* dependence graph,
* control dependence,
* memory dependence,
* type/role hypotheses.

Do NOT assume one representation is enough.

Determine whether we should:

* keep P-Code almost directly and add analysis layers,
* lower it into a custom IR,
* build a richer graph representation,
* maintain multiple synchronized views,
* or use an existing IR concept.

Explain the trade-offs.

---

# 6. Candidate region discovery is critical

A major problem is deciding which part of the function corresponds to a possible inline function.

Naively considering every instruction interval is unacceptable.

For a function with N operations, we absolutely cannot afford a huge O(N²) or worse candidate explosion unless there is a very strong pruning mechanism.

We want the **minimum number of candidate regions that still safely preserves real possibilities**.

The principle is:

> If we are not sufficiently confident that something can be excluded, preserve it as a candidate.

But at the same time:

> Do not generate massive numbers of meaningless regions.

This is a fundamental optimization problem.

Research how to construct meaningful candidate regions using combinations of:

* CFG structure,
* dominators,
* post-dominators,
* natural loops,
* SCCs,
* SSA dependencies,
* use-def chains,
* data-flow slicing,
* side-effect boundaries,
* call boundaries,
* memory dependencies,
* control dependence,
* producer/consumer relationships,
* region formation,
* semantic boundaries,
* repeated computation patterns,
* library call boundaries,
* return-like boundaries,
* strongly connected components,
* dependence closures.

Investigate multiple possible region-generation algorithms.

For each one discuss:

* completeness,
* candidate count,
* complexity,
* false negatives,
* false positives,
* behavior on loops,
* behavior on branches,
* behavior on nested inline functions,
* behavior on partially merged computations,
* behavior on compiler-optimized code.

Prefer a conservative strategy when uncertainty exists.

Potentially design a system where candidate regions have confidence levels or hypothesis states rather than being immediately accepted/rejected.

---

# 7. Nested callees must be first-class citizens

The recognizer must correctly handle callees inside the analyzed function.

There are at least three important categories.

## 7.1 External/library callee

Example:

```text
F
 |
 +--> kernel32!CreateFileA
```

The external library call should not necessarily be executed internally.

Instead, it may be modeled as an observable event.

For example:

```text
CALL(CreateFileA,
     arg0=path,
     arg1=access,
     arg2=share_mode,
     ...)
```

The call itself becomes part of the observable behavior.

---

# 7.2 Ordinary internal callee

Example:

```text
F
 |
 +--> sub_140123456
```

This callee may itself contain useful recognizable behavior.

The recognizer should support executing it.

Therefore, the execution model may recursively traverse:

```text
F
 -> callee A
    -> callee B
       -> external library
```

The system needs to determine how recursive execution should work and how to avoid uncontrolled explosion.

---

# 7.3 Treat an ordinary callee as opaque/external

We also explicitly need a mode where an ordinary internal callee is **not executed**.

Instead, it is treated like an external function:

```text
CALL(sub_140123456,
     arg0=...
     arg1=...)
```

The passed arguments are recorded in the observable output state.

This is important because:

* the callee may not yet be analyzed,
* execution may be expensive,
* recursive dependencies may be undesirable,
* a function may be recognized based on its calling behavior,
* we may want hierarchical recognition,
* we may want to summarize a call without exploring its body.

Design a clean policy mechanism for:

* `EXECUTE_CALLEE`
* `OPAQUE_CALLEE`

or a better general mechanism if you find one.

Investigate whether there should also be:

* `SUMMARIZED_CALLEE`
* `PARTIALLY_EXECUTED_CALLEE`
* `KNOWN_EXTERNAL`
* `UNKNOWN_EXTERNAL`
* `SPECULATIVE_EXECUTION`

or another abstraction.

---

# 8. Observable output state

The output of execution must be substantially richer than just a return value.

Investigate a robust canonical representation such as:

```text
ObservableState {
    return_values
    modified_registers
    modified_memory
    flags
    library_calls
    callee_calls
    side_effects
}
```

But do not treat this structure as final.

Research what should actually be observable.

Potential channels include:

### Registers

* return registers,
* modified caller-visible registers,
* hidden architectural state when relevant.

### Memory

* modified memory,
* initialized/read memory,
* pointer-relative memory,
* buffers,
* strings,
* heap-like regions.

### Flags

Only when semantically relevant.

### Calls

Each call may contain:

```text
callee identity
argument values
argument roles
return value
memory regions passed
call order
possibly selected side effects
```

Avoid raw addresses when they are not semantically meaningful.

Normalize addresses and memory regions whenever possible.

For example:

```text
INPUT_BUFFER_0
OUTPUT_BUFFER_0
GLOBAL_0
STACK_REGION
HEAP_REGION_0
```

rather than embedding arbitrary absolute addresses into fingerprints.

---

# 9. Library calls must be a major recognition signal

The recognizer must not treat library calls as merely an implementation detail.

For many real functions, the most informative semantic behavior is exactly the use of a known API.

Examples:

```text
CreateFileA
ReadFile
WriteFile
CloseHandle
VirtualAlloc
VirtualFree
CreateThread
socket
connect
send
recv
memcpy
memmove
strlen
strcmp
malloc
free
```

and many others.

Investigate how to combine:

* direct behavioral output,
* memory side effects,
* returned values,
* call sequence,
* normalized argument semantics,
* known API identities,
* API argument relationships,
* API return behavior.

For example, these two pieces of behavior should ideally be distinguishable:

```text
memcpy(dst, src, n)
```

versus

```text
memmove(dst, src, n)
```

even when finite probes happen to produce similar outputs.

Likewise:

```text
CreateFileA(path, access, ...)
```

should preserve useful semantic information about:

* the path argument,
* access flags,
* sharing flags,
* creation mode,
* returned handle.

Investigate how to represent these observations compactly and robustly.

---

# 10. Function database design

The function database is one of the most important architectural questions.

We need a database that can be expanded easily.

Adding a function should be straightforward.

The user should not have to manually write huge amounts of metadata for every function.

At the same time, minimizing metadata must NOT significantly reduce recognition quality.

Investigate the minimum useful information required for a database entry.

Potential information includes:

```text
function identity
library identity
prototype / parameter schema
return schema
parameter roles/types
memory semantics
side-effect semantics
call behavior
probe generation rules
expected observable outputs
semantic tags
determinism/environment requirements
```

But your task is to determine which of these are actually necessary.

Do not assume everything must be manually provided.

Investigate whether the database can automatically derive much of this from:

* reference implementations,
* compiler-generated P-Code,
* normal execution,
* test generation,
* symbolic analysis,
* documentation,
* headers,
* prototypes,
* source code,
* generated probe suites.

The ideal workflow for adding a new function should be close to:

```text
Add function definition
        ↓
Generate / collect probes
        ↓
Execute reference implementation
        ↓
Store normalized behavioral model
        ↓
Function becomes recognizable
```

Investigate how close we can get to this ideal.

---

# 11. Probe generation

We currently consider using algebraic structures such as finite rings `Z_N` and vectors to generate many standardized inputs.

For example, an input may conceptually be represented as:

```text
X = [x0, x1, ..., xn, maxLen]
```

with components generated from a finite domain.

However, we already know a problem:

> A vector does not automatically solve the problem of understanding which values correspond to which semantic arguments or memory layout.

For example, for:

```text
strlen(char *s)
```

the system must understand that some part of the input represents a byte sequence in memory and the result is a length.

We therefore need a more principled probe-generation system.

Investigate:

* randomized probing,
* structured probing,
* algebraic probe sets,
* boundary-value probes,
* pairwise probes,
* metamorphic testing,
* differential probing,
* adaptive probes,
* candidate-discriminating probes,
* type-aware probes,
* memory-layout-aware probes,
* pointer/value probes,
* aliasing probes,
* symbolic probes,
* adversarial probes.

A particularly important idea to investigate is **adaptive behavioral identification**:

Instead of running a fixed huge probe set:

```text
probe1
probe2
probe3
...
probe1000
```

select the next probe based on the remaining candidate set.

Conceptually:

```text
Candidates:
    strlen
    strnlen
    memchr
    ...
           ↓
choose probe maximizing discrimination
           ↓
execute candidate
           ↓
eliminate candidates
           ↓
choose next probe
```

Determine whether information-gain style probe selection, active learning, decision-tree construction, or related methods can be useful.

We want high discrimination with a low number of executions.

---

# 12. Unknown argument layout

This is a fundamental difficulty.

For inline code there may be no explicit ABI-level parameter list.

Instead we may observe:

```text
r8 = ...
r9 = ...
tmp17 = ...
[mem + 0x20] = ...
```

and the actual semantic input may be buried inside these values.

We need to infer:

* which SSA values are external inputs,
* which memory areas are external inputs,
* which values are derived,
* which values are constants,
* which values correspond to likely parameters,
* how memory buffers are structured,
* whether inputs alias,
* whether one semantic argument is represented by multiple machine values,
* whether a pointer and length form one logical argument,
* whether flags/context are implicit inputs.

Investigate robust input-role inference.

Do not require exact source-level typing.

A set of hypotheses may be preferable:

```text
value V17:
    {uint32, int32, enum, pointer-offset, unknown}
```

or:

```text
argument hypothesis:
    pointer -> buffer_0
    length -> r9
```

Preserving uncertainty is important.

---

# 13. Type inference and semantic roles

We do not need a perfect C/C++ type recovery system.

We need enough semantic information to improve candidate pruning and behavioral testing.

Research lightweight type/role inference based on:

* P-Code operation semantics,
* width,
* signedness,
* pointer arithmetic,
* comparisons,
* loads/stores,
* shifts,
* floating-point operations,
* call interfaces,
* memory access patterns,
* constants,
* loop bounds,
* API conventions,
* ABI knowledge.

Possible roles include:

```text
INTEGER
SIGNED_INTEGER
UNSIGNED_INTEGER
BOOLEAN
FLOAT
DOUBLE
POINTER
BUFFER
STRING
LENGTH
HANDLE
ENUM
INDEX
ADDRESS
UNKNOWN
```

Investigate whether semantic roles are more useful than conventional static types.

---

# 14. Inline function boundaries are inherently ambiguous

A source-level function may be:

* completely inlined,
* partially inlined,
* merged with surrounding logic,
* duplicated,
* constant-folded,
* transformed,
* vectorized,
* split,
* partially dead,
* fused with another operation.

Therefore, “find the exact original source function boundary” may be fundamentally impossible in many cases.

The recognizer should instead recognize **semantic regions**.

For example:

```text
outer function:
    A
    B
    C
    D
    E
```

where:

```text
B + C + D
```

may correspond to `strlen`.

The research problem is:

> Find a region whose observable behavior matches a known function under compatible inputs and context.

Investigate how to formally define a region and its interface.

A possible abstraction:

```text
Region {
    entry
    exits
    inputs
    outputs
    memory dependencies
    side effects
    calls
    CFG fragment
}
```

But again, improve this model if necessary.

---

# 15. Execution model

The behavioral engine should execute actual P-Code semantics.

The research prototype may use a custom Python P-Code emulator.

The key principle is:

> Static analysis proposes hypotheses; concrete execution tries to falsify or confirm them.

Do NOT make static heuristics the final authority when concrete execution can cheaply reject a candidate.

Investigate:

* concrete execution,
* symbolic/concolic execution,
* hybrid execution,
* state snapshotting,
* memoization,
* execution caching,
* partial execution,
* path control,
* loop limits,
* path explosion,
* branch selection,
* multi-path probing.

The goal is not merely “execute code”.

The goal is:

> efficiently obtain enough behavioral evidence to classify a candidate.

---

# 16. Handling loops

Functions such as:

* `strlen`,
* `memcpy`,
* `memcmp`,
* parsers,
* string functions,
* search functions,

can contain loops.

Probe execution may therefore become expensive or non-terminating.

Research:

* bounded loops,
* symbolic loop summaries,
* cycle detection,
* loop invariants,
* repeated-state detection,
* adaptive input sizes,
* path-aware probes,
* dynamic loop summarization.

The system should distinguish:

```text
execution timeout
```

from:

```text
candidate mismatch
```

and preserve useful information from partial execution.

---

# 17. Memory model

Memory is central.

We need to support:

* buffers,
* arrays,
* strings,
* pointers,
* pointer arithmetic,
* overlapping buffers,
* aliasing,
* stack memory,
* heap-like memory,
* global/static memory,
* memory-mapped or external memory abstractions.

Investigate a normalized memory model suitable for behavioral fingerprints.

For example:

```text
MEM_0
MEM_1
INPUT_BUFFER_0
OUTPUT_BUFFER_0
GLOBAL_0
```

rather than raw addresses.

Investigate how to compare memory states robustly.

For example:

```text
compare only semantically relevant modified bytes
```

rather than dumping the entire emulator memory.

---

# 18. Context dependence

A region may depend on surrounding code.

Examples:

```text
x = previous_computation
inline_region(x)
```

or:

```text
flag was set before entering region
```

or:

```text
global state influences behavior
```

or:

```text
memory was initialized outside the region
```

Therefore, the region input state may include more than obvious parameter-like values.

Investigate how to define the **minimal external state boundary** of a region.

Potentially:

```text
ExternalState =
    SSA inputs
    +
    memory inputs
    +
    flags
    +
    relevant global state
    +
    selected context
```

Research how to avoid incorrectly treating internal values as external inputs and vice versa.

---

# 19. Alias analysis

Aliasing can make behavioral matching difficult.

For example:

```text
memcpy(dst, src, n)
```

with:

```text
dst != src
```

behaves differently from overlapping buffers.

Similarly:

```text
f(p, p)
```

may be meaningful.

Investigate lightweight but useful alias hypotheses:

```text
MUST_ALIAS
MAY_ALIAS
MUST_NOT_ALIAS
UNKNOWN
```

and determine how these hypotheses should affect:

* probes,
* execution,
* fingerprinting,
* candidate matching.

---

# 20. Side effects and external environment

Some functions are inherently difficult to model as pure deterministic transformations.

Examples:

* time,
* randomness,
* I/O,
* filesystem operations,
* sockets,
* atomics,
* synchronization,
* thread interaction,
* syscalls,
* environment-dependent APIs.

The recognizer must not simply collapse these into “unknown”.

Investigate how to model such operations as observable events.

For example:

```text
CALL(CreateFileA, ...)
CALL(ReadFile, ...)
CALL(CloseHandle, ...)
```

rather than trying to emulate an actual Windows kernel.

Investigate a layered abstraction:

```text
Pure computational behavior
External API behavior
Environment-dependent behavior
```

and determine how each should be represented in the fingerprint.

---

# 21. Recursive recognition

We want the system to support hierarchical recognition.

Suppose:

```text
F
├── region A
│   ├── known function X
│   └── known function Y
│
└── region B
    └── external API Z
```

The system may recognize subregions first and then use those results to improve recognition of the surrounding region.

Investigate whether recognition should be:

* bottom-up,
* top-down,
* iterative,
* fixed-point,
* beam-search based,
* graph-based,
* hierarchical classification,
* probabilistic.

A strong solution may recognize callees first, replace them with semantic summaries, and then recognize the caller.

---

# 22. Ordinary functions as a special case

The system must not assume that “inline recognition” and “normal function recognition” are different problems.

We want a unified algorithm such that:

```text
region = entire F
```

is a valid candidate.

Then:

```text
normal function recognition
```

becomes:

```text
recognition of a region covering the whole function
```

while:

```text
inline recognition
```

becomes:

```text
recognition of a proper subregion
```

This should simplify the architecture and improve consistency.

---

# 23. Minimizing false positives

This is one of the highest-priority requirements.

A recognizer that frequently says:

```text
“This looks like strlen”
```

when it is not strlen is not useful.

We therefore want:

* very high precision,
* extremely low false-positive rate,
* conservative classification,
* explicit confidence,
* rejection / unknown state,
* evidence tracking.

Do not force every region to receive a label.

The recognizer must be allowed to say:

```text
UNKNOWN
```

when evidence is insufficient.

Investigate:

* multi-probe confirmation,
* negative probes,
* candidate competition,
* minimum evidence thresholds,
* probabilistic confidence,
* confidence intervals,
* contradiction detection,
* adversarial discrimination probes,
* nearest-candidate vs abstention,
* calibrated confidence,
* class-specific thresholds.

Think like a **high-precision classifier**, not like a fuzzy search engine.

---

# 24. False-positive defense

Research especially how to reject cases such as:

```text
different_function ≈ same_behavior_for_current_probes
```

Examples:

```text
strlen
custom_strlen
loop_that_counts_until_zero
```

or:

```text
memcpy
small_hand_written_copy_loop
```

or:

```text
cos approximation
unrelated arithmetic that coincidentally matches a finite probe set
```

The system should actively seek counterexamples.

Investigate:

> Given candidate C and database function D, how do we find the smallest / cheapest probe that disproves equivalence?

This may be more powerful than simply trying to prove equivalence.

---

# 25. Behavioral equivalence is not binary

Two implementations may differ internally but be equivalent for the relevant API contract.

Examples:

* different instruction sequences,
* different register allocation,
* different loop implementation,
* vectorized vs scalar implementation,
* compiler intrinsic vs library call,
* inlined vs non-inlined implementation.

The recognizer should therefore compare **semantic behavior**, not implementation shape.

At the same time, semantic behavior may legitimately include implementation-visible effects such as:

* call sequence,
* relevant side effects,
* modified memory,
* API interactions.

Research the right equivalence notion.

Possibly define multiple equivalence levels:

```text
strict
semantic
observational
partial
contract-level
```

and determine when each should be used.

---

# 26. Database generation from known implementations

A particularly important research direction:

Can we automatically generate a high-quality database entry from a known implementation?

For example:

```text
known function:
strlen
```

Potential pipeline:

```text
reference implementation
        ↓
known prototype
        ↓
automatic probe generation
        ↓
reference execution
        ↓
observable state collection
        ↓
normalization
        ↓
behavioral model
        ↓
database entry
```

Investigate whether this can be automated enough that adding 1000 library functions is realistic.

The database should ideally scale.

---

# 27. Function families and parameterized behavior

We do not necessarily want to store only raw probe/output pairs.

Investigate whether the database should represent families such as:

```text
memcpy(dst, src, n)
```

with parameter relationships:

```text
output[0:n] = input[1][0:n]
```

or:

```text
strlen(s) = index_of_zero(s)
```

or:

```text
malloc(n) -> allocated_pointer_of_size(n)
```

or:

```text
CreateFileA(path, access, ...)
    -> external call with path/access relationship
```

This could dramatically improve generalization.

Research whether the optimal database representation is:

* raw behavioral fingerprints,
* symbolic contracts,
* executable reference models,
* test suites,
* semantic summaries,
* hybrid representation.

---

# 28. GTA5.exe as the primary benchmark

The system will primarily be evaluated on GTA5.exe.

This means you must explicitly consider:

* huge binary size,
* heavily optimized native C/C++ code,
* aggressive compiler transformations,
* inlining,
* wrappers,
* templates,
* STL-like code,
* runtime support code,
* Windows APIs,
* networking,
* file operations,
* memory management,
* floating-point/math routines,
* game-specific utility code,
* third-party libraries.

Examples of desired recognitions include functions corresponding to:

```text
cos
tan
sin
strlen
strcmp
memcpy
memmove
malloc
free
file APIs
socket APIs
memory APIs
thread APIs
Windows APIs
third-party library APIs
```

and many others.

Do not optimize the architecture only for toy arithmetic examples.

It must be designed around the realities of a large optimized AAA game binary.

---

# 29. Existing solutions and reusable sub-solutions

You are explicitly encouraged to investigate existing work.

For every major subproblem ask:

> Has this already been solved well enough by another system, paper, compiler, decompiler, binary-analysis framework, symbolic execution engine, malware-analysis framework, or academic project?

Potential areas to investigate include:

* binary lifting,
* IR design,
* SSA,
* memory SSA,
* value-set analysis,
* type inference,
* region formation,
* slicing,
* dependence analysis,
* symbolic execution,
* concolic execution,
* fuzzing,
* active learning,
* metamorphic testing,
* API recognition,
* function identification,
* binary similarity,
* semantic hashing,
* library recognition,
* dynamic function signatures,
* program summaries,
* abstract interpretation,
* partial evaluation,
* equivalence checking,
* compiler optimization analysis,
* side-effect analysis.

Do not reinvent things unnecessarily.

However, do not blindly import existing algorithms either.

Evaluate whether they actually fit:

* low P-Code input,
* large optimized binaries,
* inline functions,
* behavioral classification,
* high precision,
* low false-positive requirements.

---

# 30. Ring/vector hypothesis

We still want you to evaluate the usefulness of finite algebraic structures such as:

* `Z_N`,
* vector spaces,
* structured finite domains,
* product spaces.

The idea is to generate standardized, diverse, reproducible test states.

However, determine whether this is actually the right abstraction.

It may be:

* highly useful,
* useful only for some parameter classes,
* useful only for probe generation,
* useful for hashing/fingerprinting,
* inferior to adaptive/randomized strategies,
* or fundamentally the wrong abstraction.

Do not preserve it merely because it sounds mathematically elegant.

Experimentally compare it with better alternatives.

---

# 31. Important theoretical limitations

You must explicitly analyze fundamental limits.

At minimum investigate:

* undecidability,
* semantic equivalence,
* finite-observation collisions,
* contextual behavior,
* hidden state,
* environment dependence,
* path explosion,
* loops,
* aliasing,
* self-modifying code if relevant,
* undefined behavior,
* compiler-specific semantics,
* floating-point corner cases,
* exceptional behavior,
* concurrency,
* syscalls,
* nondeterminism.

We do NOT need a theoretical system that solves undecidable problems.

We need a practical system with clearly bounded failure modes.

The research plan should explain:

1. what cannot be guaranteed,
2. what can be made highly reliable,
3. where conservative abstention is required,
4. how experiments can reveal the actual practical limits.

Do not claim “100% accuracy” as a mathematical guarantee.

Instead design toward extremely high precision on the target workload and define measurable empirical targets.

---

# 32. Multiple candidate hypotheses

A critical design principle:

> Do not throw information away too early.

If the system is uncertain whether something is:

```text
int
uint32
pointer
enum
length
```

preserve multiple hypotheses when practical.

If region boundaries are uncertain:

```text
keep several plausible region candidates
```

If callee behavior is uncertain:

```text
keep both:
    execute
    opaque
```

where computationally feasible.

If multiple argument mappings are possible:

```text
preserve them until later evidence eliminates them
```

This should be a deliberate architectural principle.

Investigate efficient ways to implement this without exponential explosion.

---

# 33. Hierarchical search and pruning

The whole problem potentially creates an enormous search space.

Research a staged architecture such as:

```text
P-Code
   ↓
IR
   ↓
cheap structural analysis
   ↓
candidate regions
   ↓
input/output hypotheses
   ↓
cheap semantic/type pruning
   ↓
function-family candidates
   ↓
probe generation
   ↓
behavioral execution
   ↓
candidate elimination
   ↓
high-confidence classification
```

But this is only one hypothesis.

You should investigate better architectures, including:

* coarse-to-fine classification,
* beam search,
* branch-and-bound,
* active candidate elimination,
* learned ranking,
* graph matching,
* memoized execution,
* hierarchical function families,
* lazy region expansion.

The final research plan should recommend an experimentally testable architecture.

---

# 34. Computational efficiency

Accuracy is critical, but the recognizer must also be practical.

Investigate the cost of:

* IR construction,
* SSA,
* region generation,
* candidate matching,
* P-Code execution,
* probe count,
* recursive callee handling,
* memory snapshots,
* state comparison.

Find opportunities for:

* caching,
* memoization,
* snapshot/restore,
* shared execution prefixes,
* probe reuse,
* candidate grouping,
* early rejection,
* database indexing,
* function-family prefiltering,
* call-summary caching,
* region reuse.

We want the expensive behavioral execution stage to be reached only after strong cheap pruning.

---

# 35. Research prototype architecture

The first implementation will be a **Python research prototype**.

It should be deliberately optimized for:

* experimentation,
* correctness,
* observability,
* measuring hypotheses,
* changing algorithms quickly.

It is NOT production code.

Possible structure:

```text
FUNC_RECOGNIZER_RESEARCH/
    src/
        ir/
        cfg/
        ssa/
        memory/
        typing/
        regions/
        probes/
        emulator/
        summaries/
        database/
        matching/
        analysis/
        experiments/
    corpus/
    database/
    experiments/
    results/
    docs/
```

This is only a suggestion.

Design a better structure if appropriate.

---

# 36. Experimental methodology

This is extremely important.

Do not jump straight into one giant implementation.

Design a sequence of controlled experiments.

For example:

```text
Experiment 001
IR construction

Experiment 002
SSA / data-flow extraction

Experiment 003
candidate region generation

Experiment 004
input/output inference

Experiment 005
probe generation

Experiment 006
behavioral execution

Experiment 007
behavioral matching

Experiment 008
library-call modeling

Experiment 009
callee recursion

Experiment 010
opaque callees

Experiment 011
adaptive probe selection

Experiment 012
false-positive resistance

Experiment 013
large-function scaling

Experiment 014
GTA5-oriented corpus evaluation
```

Again, these are examples.

You should create a better experiment hierarchy if needed.

Each experiment must have:

* hypothesis,
* setup,
* input corpus,
* methodology,
* metrics,
* expected outcome,
* interpretation,
* failure criteria,
* follow-up decision.

---

# 37. Benchmark corpus

Design a dataset that contains:

### Simple functions

```text
strlen
memcpy
memset
strcmp
math
arithmetic
search
copy
allocation
```

### Optimized variants

* different compilers,
* optimization levels,
* inlining,
* vectorization,
* LTO where practical,
* different optimization layouts.

### Wrapped functions

```text
wrapper -> strlen
wrapper -> CreateFileA
wrapper -> memcpy
```

### Nested functions

```text
F
 ├── A
 │    └── B
 └── C
```

### Inline functions

Actual compiler-inlined implementations.

### Confusers

Functions that look superficially similar but are semantically different.

### Negative dataset

This is especially important for measuring false positives.

Design the corpus so that the recognizer is forced to reject plausible but incorrect matches.

---

# 38. Metrics

Define precise metrics.

At minimum investigate:

* precision,
* recall,
* false-positive rate,
* false-negative rate,
* abstention rate,
* candidate reduction ratio,
* number of behavioral executions,
* average execution cost,
* worst-case execution cost,
* probes per recognition,
* recognition accuracy by function family,
* accuracy by optimization level,
* accuracy by inline depth,
* accuracy with/without callee execution,
* accuracy with/without API-call observations.

Do not optimize only for aggregate accuracy.

A recognizer may achieve high aggregate accuracy while being terrible for one important class.

---

# 39. Recognition confidence and evidence

For every recognition result, investigate whether we should return not only:

```text
strlen
```

but something like:

```text
candidate:
    strlen

confidence:
    high

evidence:
    - 7 probes matched
    - 2 discriminating probes passed
    - call behavior compatible
    - memory behavior compatible
    - no contradictory observations
```

The exact representation is up to you.

The important requirement is:

> The classifier should be able to explain why a candidate was accepted or rejected.

This will be essential for debugging research failures.

---

# 40. What the final recognizer should conceptually do

We ultimately want something conceptually like:

```text
INPUT:
    low P-Code of function F

STEP 1:
    Build analysis IR

STEP 2:
    Build CFG / SSA / memory dependencies

STEP 3:
    Infer external inputs and observable outputs

STEP 4:
    Discover a small but conservative set of candidate semantic regions

STEP 5:
    Analyze calls / callees

STEP 6:
    For each candidate region:
        infer possible interface
        select compatible database functions

STEP 7:
    Generate informative probes

STEP 8:
    Execute P-Code concretely / symbolically as appropriate

STEP 9:
    Collect normalized observable state

STEP 10:
    Compare observations with the function database

STEP 11:
    Generate counterexample probes when necessary

STEP 12:
    Eliminate inconsistent candidates

STEP 13:
    Produce high-confidence recognitions and explicit UNKNOWN states

STEP 14:
    Optionally use recognized callees/regions as semantic summaries

STEP 15:
    Re-analyze larger surrounding regions

OUTPUT:
    recognized functions / semantic regions
    with evidence and confidence
```

Again, this pipeline is a starting hypothesis, not a requirement.

---

# 41. Critical examples to reason through

Your research plan must contain concrete worked examples.

At minimum analyze examples conceptually similar to:

## Example A — ordinary function

```text
int strlen(const char* s)
```

represented directly as a normal function.

Show how the system recognizes the whole function.

---

## Example B — inline strlen

```text
foo(...)
{
    ...
    n = strlen(s);    // compiler inlined
    ...
}
```

Show how the system discovers the semantic region corresponding to `strlen`.

---

## Example C — inline arithmetic/library function

```text
x = cos(v);
```

after aggressive compiler optimization.

Show how transformed floating-point P-Code could still be recognized.

---

## Example D — library call

```text
f(...)
{
    h = CreateFileA(path, ...);
    ...
}
```

Explain how the call becomes part of the observable behavior.

---

## Example E — ordinary internal callee

```text
F
  -> sub_140123456
```

where the callee is executed recursively.

Show the behavioral model.

---

## Example F — ordinary callee treated as opaque

Same example, but:

```text
sub_140123456
```

is treated as external.

Show how:

```text
CALL(sub_140123456, normalized_args...)
```

is recorded in output state.

---

## Example G — nested structure

```text
F
 ├── inline A
 │    ├── inline B
 │    └── external API C
 └── internal callee D
      └── external API E
```

Show how the recognizer can reason about the entire structure.

---

## Example H — false positive

Create a function that produces outputs identical to `strlen` for several naive probes but is not actually equivalent.

Show how your proposed system discovers a discriminating probe.

---

# 42. Required research questions

Your plan MUST explicitly answer these questions:

1. What is the mathematically correct abstraction for recognizing an inline function?
2. Is the right abstraction a region, slice, subgraph, state transition, trace, summary, or something else?
3. What is the best IR for this problem?
4. How should SSA and memory SSA be used?
5. How can candidate regions be generated without combinatorial explosion?
6. How can region completeness be preserved conservatively?
7. How should hidden/context inputs be detected?
8. How should memory inputs/outputs be represented?
9. How should aliases be handled?
10. How should types/semantic roles be inferred?
11. How should calls be represented?
12. When should callees be executed?
13. When should callees be opaque?
14. Can recognition be recursive?
15. What should an ObservableState actually contain?
16. How should external APIs be normalized?
17. How should library calls influence classification?
18. How should function database entries be represented?
19. What is the minimum useful metadata?
20. Can database entries be generated automatically?
21. What is the best probe-generation strategy?
22. Are algebraic/ring-based probes actually useful?
23. Should probes be adaptive?
24. How should candidate-specific probes be chosen?
25. How do we detect false positives?
26. How do we handle loops?
27. How do we handle floating-point behavior?
28. How do we handle environment-dependent calls?
29. How do we handle compiler optimizations?
30. How do we handle partially inlined functions?
31. How do we handle merged computations?
32. How do we prevent candidate explosion?
33. How do we cache execution?
34. How do we represent uncertainty?
35. When should the system abstain?
36. What existing academic/industrial solutions can we reuse?
37. What is fundamentally impossible?
38. Which parts are likely to work extremely well on GTA5.exe?
39. Which parts are likely to fail on GTA5.exe?
40. How can the architecture minimize those failure modes?

---

# 43. Compare alternative architectures

Do not give us only one design.

Identify the major competing approaches.

For important decisions, compare several possibilities.

For example:

```text
A. Pure concrete behavioral execution
B. Symbolic execution
C. Hybrid concrete + symbolic
D. Behavioral fingerprints
E. Semantic summaries/contracts
F. Graph + behavioral matching
G. Hierarchical recognition
H. Active-learning recognition
```

You do NOT have to enumerate meaningless alternatives.

Only compare approaches that could realistically matter.

For every important alternative provide:

* strengths,
* weaknesses,
* complexity,
* implementation difficulty,
* expected accuracy,
* expected false positives,
* scaling characteristics,
* suitability for GTA5,
* recommended experiments.

---

# 44. When uncertain, preserve alternatives

For any design decision where evidence is insufficient to confidently choose one approach:

Do NOT arbitrarily choose one and pretend it is obviously correct.

Instead provide:

1. Option A
2. Option B
3. Option C if relevant
4. trade-offs,
5. failure modes,
6. implementation cost,
7. expected performance,
8. what experiment can decide between them.

Then state which option should be tested first and why.

For important uncertainty, design the prototype so that two alternatives can be implemented and compared.

---

# 45. Explicitly analyze additional constraints yourself

The following constraints are already known, but you MUST look for additional hidden constraints that we have not identified.

Known constraints include:

* optimized binaries,
* aggressive inlining,
* lack of clean source-level boundaries,
* low P-Code as the only fundamental input,
* unknown argument layout,
* memory aliasing,
* loops,
* branch/path explosion,
* hidden context state,
* external APIs,
* normal callees,
* recursive callees,
* false-positive sensitivity,
* finite probe collisions,
* database maintenance cost,
* huge GTA5-scale binaries,
* computational cost,
* compiler transformations,
* floating-point semantics,
* environment dependence,
* incomplete information,
* uncertain types,
* uncertain region boundaries.

Search for other constraints that could fundamentally affect the approach.

Examples might include:

* ABI quirks,
* exceptional control flow,
* tail calls,
* indirect calls,
* function pointers,
* thunks,
* import forwarding,
* variadic functions,
* structure returns,
* SIMD/vector semantics,
* unusual memory models,
* undefined behavior,
* partial register semantics,
* endian/width normalization,
* NaN behavior,
* exception paths,
* thread-local state,
* stack unwinding,
* interprocedural context,
* compiler intrinsics,
* intrinsic lowering,
* opaque predicates,
* dead stores,
* speculative execution artifacts,
* security mitigations.

Do not assume this list is complete.

---

# 46. The plan must prioritize experimental truth

The result should not be a beautiful theoretical document that cannot be tested.

The final plan must specify:

* a minimal viable research prototype,
* exact staged experiments,
* what to implement first,
* what to postpone,
* what can be mocked,
* what must be real,
* what datasets are required,
* how success/failure is measured.

The coding model should be able to take your document and start implementing a Python research prototype without inventing the architecture again.

---

# 47. The plan must remain research-oriented

Do not over-engineer the first implementation.

We are not building the final C++ production recognizer yet.

We are trying to answer:

> Does this fundamental approach actually work, and what is the strongest practical algorithm we can construct around it?

The Python prototype exists to validate the science.

Production concerns such as:

* packaging,
* APIs,
* plugin systems,
* distributed execution,
* GUI,
* deployment,

are secondary.

---

# 48. Explicitly design falsification experiments

This is mandatory.

Do not only design experiments intended to prove that the recognizer works.

Design experiments intended to BREAK it.

For every major hypothesis:

```text
Hypothesis:
Behavioral fingerprints identify functions reliably.

Falsification:
Construct semantically different functions that collide under many probes.
```

Likewise:

```text
Hypothesis:
CFG/SSA can produce a compact set of candidate inline regions.

Falsification:
Construct optimized code where the true semantic region is split/merged/context-dependent.
```

And:

```text
Hypothesis:
Library-call observations dramatically improve recognition precision.

Falsification:
Construct wrappers or unrelated functions producing similar call traces.
```

The goal is to discover failure boundaries early.

---

# 49. The final deliverable structure

Produce `RESEARCH_PLAN.md` with a structure roughly like:

```text
1. Executive Summary
2. Problem Definition
3. Research Hypothesis
4. Critical Assumptions
5. Fundamental Theoretical Limits
6. Existing Related Techniques
7. Proposed Overall Architecture
8. IR Design
9. CFG / SSA / Memory Analysis
10. Input / Output Inference
11. Region Discovery
12. Callee Handling
13. External API Modeling
14. Behavioral Execution
15. Observable State
16. Probe Generation
17. Adaptive Probe Selection
18. Function Database
19. Function Database Construction
20. Candidate Matching
21. False Positive Defense
22. Uncertainty / Hypothesis Management
23. Recursive Recognition
24. Optimization / Caching
25. Scalability
26. GTA5-Specific Considerations
27. Alternative Designs
28. Recommended Design
29. Experimental Roadmap
30. Dataset / Corpus
31. Metrics
32. Falsification Experiments
33. Failure Modes
34. Python Research Prototype Architecture
35. Detailed Implementation Plan
36. Acceptance Criteria
37. Future Extensions
```

Improve this structure where necessary.

---

# 50. Implementation plan requirements

The last part of the document must be concrete enough for a cheaper coding model to implement.

For every major component specify:

* purpose,
* inputs,
* outputs,
* data structures,
* algorithm,
* important invariants,
* complexity,
* edge cases,
* interactions with other components,
* test strategy,
* experiment strategy.

Provide pseudocode where useful, but do not write the actual implementation.

Examples of useful pseudocode:

```text
discover_regions(F)
infer_external_inputs(region)
generate_probe(region, candidates)
execute(region, probe)
normalize_state(state)
compare(candidate, observation)
select_next_probe(candidates, evidence)
```

---

# 51. What success should look like

The final research architecture should aim for a recognizer where:

```text
low P-Code
    ↓
analysis
    ↓
small conservative candidate set
    ↓
high-quality input hypotheses
    ↓
targeted behavioral experiments
    ↓
strong behavioral evidence
    ↓
candidate elimination
    ↓
high-confidence classification
```

The system should be able to recognize:

* complete ordinary functions,
* completely inlined functions,
* partially inlined functions,
* optimized/transformed implementations,
* functions containing external library calls,
* functions containing ordinary internal callees,
* functions containing nested recognized regions,
* wrappers and combinations of these cases.

It should prefer:

```text
UNKNOWN
```

over a weakly supported incorrect classification.

---

# 52. Most important design philosophy

Keep these principles central throughout the research:

### Principle 1

**Static analysis narrows the search space. Behavioral execution provides evidence.**

### Principle 2

**Do not discard uncertainty prematurely.**

### Principle 3

**Recognition is classification, not source reconstruction.**

### Principle 4

**Semantic behavior matters more than syntactic similarity.**

### Principle 5

**External calls are semantic observations, not merely implementation details.**

### Principle 6

**Normal functions and inline functions should share one unified recognition model.**

### Principle 7

**False positives are more dangerous than abstaining.**

### Principle 8

**Database construction must scale.**

### Principle 9

**Probe selection should maximize information, not merely quantity.**

### Principle 10

**Every major assumption must have a falsification experiment.**

### Principle 11

**Reuse solved subproblems whenever that is superior to reinventing them.**

### Principle 12

**Optimize for GTA5-scale real-world optimized binaries, not toy examples.**

---

# 53. Final instruction to you

Think deeply before deciding on the architecture.

Do not merely restate this prompt.

Do not simply turn our current idea into software modules.

Instead:

> Invent the strongest practical research architecture you can derive from the underlying problem.

You have permission to fundamentally change our approach when necessary.

The ultimate goal is an advanced, high-precision, low-false-positive recognizer for real optimized binaries, with inline functions as a first-class target.

The conceptual idea of:

```text
P-Code
→ IR
→ semantic regions
→ input/output hypotheses
→ behavioral execution
→ observable state
→ database comparison
```

is a starting point.

Your job is to determine what the **best possible version of this idea** actually looks like.

The final document must be detailed, rigorous, experimentally grounded, and directly usable by another model as the implementation blueprint for a Python research prototype.

**Do not write production code.**

**Do not optimize for elegance at the expense of accuracy.**

**Do not hide uncertainty.**

**Do not assume that our initial architecture is correct.**

**Research it, attack it, improve it, and design the strongest version you can.**
