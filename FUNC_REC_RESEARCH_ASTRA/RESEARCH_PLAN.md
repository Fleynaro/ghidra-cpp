# High-Precision P-Code Semantic Function Recognition

## 1. Executive Summary

This document is a research and implementation plan for recognizing functions and
semantic function regions from low-level Ghidra P-Code. The primary benchmark is
optimized native code in `GTA5.exe`, but the design is intended to generalize to
libraries and arbitrary binaries.

The central conclusion is that a finite list of concrete input/output pairs is
not, by itself, a sufficiently safe function identity. It is useful evidence, but
it has unavoidable collisions. The recognizer should therefore be a conservative,
hybrid classifier with four cooperating layers:

1. **Normalized P-Code analysis.** Build a small analysis IR that preserves P-Code
   semantics and widths while adding CFG, SSA, MemorySSA-like memory versions,
   dependence, call, and effect views.
2. **Semantic-region hypothesis generation.** Generate a small beam of candidate
   regions, not every instruction interval. A candidate has an interface, context
   assumptions, exits, effects, and uncertainty. The whole function is always a
   valid candidate; inline recognition is recognition of a proper semantic region.
3. **Contract-directed behavioral testing.** Infer possible input roles and memory
   layouts, generate typed and alias-aware probes, execute P-Code concretely, and
   use symbolic or metamorphic tests only where they add discrimination. External
   calls are recorded as normalized events rather than emulated as real operating
   system behavior.
4. **Evidence-based classification with abstention.** Match a region against a
   database containing executable contracts, effect summaries, probe suites, and
   implementation variants. Actively seek counterexamples. Accept only when the
   leading candidate has enough independent evidence and a calibrated margin over
   competitors; otherwise return `UNKNOWN`.

The mathematical object being recognized is not a source-level function boundary.
It is a **context-parametric partial observational contract**:

```text
H = (G, I, E, O, A, C)

G = a selected P-Code subgraph with explicit entries and exits
I = value, memory, flag, and global inputs crossing its boundary
E = execution/environment policy for calls and nondeterminism
O = normalized observations produced by the region
A = alias and path assumptions under which the observation is valid
C = the candidate's confidence/evidence state
```

The region is accepted as an instance of database contract `D` only when its
observations agree with `D` for a probe sequence selected to separate `D` from
plausible competitors, and no static or dynamic contradiction is found. This is
contract-level recognition, not proof of source provenance. It can recognize an
inlined or transformed implementation even when the original source boundary is
gone, while explicitly refusing claims that the evidence cannot support.

The first Python prototype should be deliberately instrumented and experimental.
It should reuse the working concrete emulator in
`FUNC_RECOGNIZER_RESEARCH/src/pcode_emulator.py`, but must add analysis and
measurement rather than prematurely optimize for production throughput.

Recommended first milestone:

- x86-64 P-Code input from the existing PyGhidra adapter;
- normalized operation model and CFG;
- SSA/value-def and conservative memory dependence extraction;
- SESE/dependence-closed region candidates plus the whole-function candidate;
- typed buffer/integer probes for `strlen`, `memcpy`, `memmove`, `strcmp`, and
  arithmetic functions;
- normalized return, memory-diff, call, and termination observations;
- database contracts generated from small reference functions;
- candidate elimination, counterexample probes, evidence records, and
  `UNKNOWN` decisions;
- controlled experiments before adding symbolic execution or learned ranking.

The plan does not claim perfect accuracy. Semantic equivalence is undecidable in
general, finite probes collide, and optimized binaries erase provenance. The
engineering target is measurable high precision on a declared corpus, with
well-defined abstention and failure modes.

## 2. Problem Definition

### 2.1 Input and output

The fundamental input is low-level P-Code for one function or a larger analyzed
code region. No ProgramDB, decompiler output, source code, or debug type
information may be required by the recognizer core. A front end may use PyGhidra
to extract P-Code and optional metadata, but the core accepts a serializable
P-Code bundle.

The output is a set of recognition records. Each record identifies:

- a semantic region (operation or basic-block membership, entries, exits);
- zero or more candidate function contracts and family labels;
- confidence and calibrated decision state (`ACCEPT`, `REVIEW`, or `UNKNOWN`);
- the inferred interface and context assumptions;
- normalized observations and evidence supporting the decision;
- contradictions, rejected candidates, and resource limits encountered.

One input function can yield several non-overlapping or nested records. A record
may describe the whole input function, a proper inline region, or a recognized
callee summary. The system must not imply that a region is an exact source
boundary unless independent evidence establishes that fact.

### 2.2 Recognition target

The target classes are:

- ordinary complete functions;
- library or runtime implementations;
- utility functions and wrappers;
- compiler intrinsics and transformed implementations;
- completely or partially inlined functions;
- regions containing calls to external APIs;
- regions containing ordinary internal callees;
- nested and combined semantic regions.

The classifier should recognize a function family and contract first. An optional
implementation or ABI-specific label is a separate, stricter decision. For
example, a vectorized `memcpy` and a scalar `memcpy` may share the contract-level
label while retaining different implementation-variant evidence.

### 2.3 The correct abstraction

The naive abstraction `X -> Y` is insufficient because a machine-code region can
read and write memory, consume incoming flags, call an environment, terminate
along several paths, and depend on values computed outside its apparent
arguments. The useful abstraction is:

```text
(external values, memory, flags, global/context state, call policy)
    -> distribution or set of normalized observations
```

For a deterministic pure function this reduces to a state transition. For a
region with branches, it is a partial transition relation with path and
termination observations. For nondeterministic or environmental behavior, it is
an observation trace with abstract event values. The region's interface is part
of the hypothesis; it is not assumed to be known in advance.

### 2.4 Equivalence levels

The prototype must distinguish these levels instead of mixing them in one score:

1. **Bit/trace strict equivalence:** same normalized bits, memory effects, event
   order, and termination under a specified probe and environment policy.
2. **Machine-observable equivalence:** same contract-relevant outputs and effects,
   ignoring dead temporaries, register allocation, and unobservable flags.
3. **API contract equivalence:** same documented function behavior and permitted
   side effects, allowing implementation-specific call sequence differences.
4. **Family similarity:** useful for retrieval and candidate generation only; never
   sufficient for a high-confidence label.

The database entry declares which level is authoritative for each channel. A
`memcpy` contract can ignore whether the implementation uses a library call, but
an API-wrapper contract should preserve the `CreateFileA` event and its argument
relationships.

## 3. Critical Assumptions and Their Tests

The prototype must record assumptions explicitly and attach a falsification test
to each one.

| Assumption | Why it may help | Falsification | Default response |
| --- | --- | --- | --- |
| P-Code preserves enough semantics | It is the required common input | Unsupported userops, lost indirect targets, or target-specific semantics | Mark unsupported portions and abstain |
| Dependence-closed regions approximate inline units | Inlining usually preserves data/control dependencies | Split, fused, vectorized, or context-dependent examples | Keep multiple region hypotheses and widen only under budget |
| A small typed probe set separates common library families | Boundary and alias cases are highly informative | Construct colliding implementations | Generate counterexamples or abstain |
| API identity is a strong signal | Calls often reveal semantics directly | Generic wrappers and dispatch stubs | Require argument/effect compatibility, not call name alone |
| Callee summaries compose | Bottom-up summaries reduce caller complexity | Recursive, stateful, or unknown callees | Keep opaque and executed alternatives |
| Concrete execution is cheap enough after pruning | P-Code execution is observable and deterministic | Large loops/path explosion | Bounded execution, caching, symbolic summary, or abstention |
| Role inference need not recover source types | Roles guide probes without exact C reconstruction | ABI casts and polymorphic pointers | Maintain role hypotheses and test memory layouts |

No assumption should be elevated to an invariant until the corresponding
experiment has passed on both positive and adversarial corpora.

## 4. Fundamental Theoretical Limits

The recognizer cannot prove arbitrary binary semantic equivalence. Rice-style
undecidability, halting, environment dependence, and unknown external state apply
even when P-Code semantics are perfectly defined. Two different functions can
agree on every finite probe set selected so far, and an inline source boundary
may have no unique representation after optimization.

Practical guarantees are narrower:

- a concrete observation is reproducible under a fixed emulator and policy;
- a contradiction is valid when the compared channel is modeled correctly;
- a static dependency is a safe exclusion only when the analysis is conservative;
- a high-confidence decision is an empirical/calibrated claim, not a proof;
- `UNKNOWN` is the correct result for unsupported or underdetermined cases.

The system must distinguish:

- **mismatch:** a modeled observation contradicts a candidate;
- **inconclusive:** timeout, unsupported operation, unknown environment, or
  insufficient interface mapping;
- **resource exhaustion:** candidate or path budget was reached;
- **invalid input:** malformed P-Code or an unresolved control-flow target.

These states must not be collapsed into false negatives. Experiments should report
coverage and abstention alongside precision and recall.

Additional irreducible limits include:

- finite observations cannot distinguish all total functions;
- a callee may alter hidden global, TLS, or scheduler state;
- undefined behavior means a source contract may not determine machine behavior;
- floating point requires a declared NaN, signed-zero, exception, and tolerance
  policy;
- indirect calls and function pointers may have an open target set;
- self-modifying code and anti-analysis behavior can invalidate a static graph;
- concurrency makes a single-thread trace a projection, not a complete behavior;
- exception edges, stack unwinding, and security instrumentation may be outside a
  simple normal-return model.

The design minimizes damage from these limits by keeping provenance, marking
unknown channels, using negative probes, and refusing unsupported high-confidence
claims.

## 5. Existing Techniques and Reuse Decisions

The research should reuse mature ideas where they fit while retaining a small
custom core.

### 5.1 Binary lifting and P-Code

Ghidra P-Code is the input contract. The front end should preserve opcode, address
space, byte width, sequence address, branch target, and userop identity. The
prototype must not depend on Java objects after serialization. The existing
custom emulator is an appropriate starting point for concrete integer execution,
sparse memory, control flow, and observer hooks. It currently lacks the analysis
layers and broad floating-point/indirect-call coverage needed here; unsupported
operations must remain explicit.

### 5.2 SSA, MemorySSA, and dependence graphs

Use conventional value SSA for register/unique definitions and phi nodes at CFG
joins. Use a conservative memory-version graph rather than pretending to have a
perfect alias analysis. Memory definitions are partitioned by address-space and
abstract region; uncertain stores create may-def edges to affected partitions.
This provides use-def and slice queries without requiring a complete decompiler.

### 5.3 Region formation and slicing

Use dominators, post-dominators, SCCs, loops, control dependence, and program
dependence graphs to generate SESE-like candidates. A candidate is not required to
be a contiguous instruction interval. Slices and dependence closures are useful
for proposed semantic behavior, but omitted control/effect dependencies must be
represented as context inputs or cause rejection.

### 5.4 Symbolic/concolic execution

Do not begin with whole-program symbolic execution. Use concrete execution as the
default falsification engine. Add a small bit-vector/path-constraint layer for:

- finding branch inputs that reach an untested path;
- solving a candidate-vs-database relational mismatch;
- summarizing simple loops or arithmetic;
- producing a counterexample when random/adaptive probes stall.

Z3 or another solver may be an optional experiment dependency; the core must have
a deterministic fallback and report when symbolic reasoning is unavailable.

### 5.5 Fuzzing and metamorphic testing

Use property-based generation, boundary-value generation, and metamorphic
relations. A mutation-based fuzzer is useful for expanding a database's negative
corpus and finding collisions, but raw fuzzing is not an interface inference
algorithm. Every generated probe needs a role/memory-layout description.

### 5.6 Binary similarity and embeddings

Ghidra BSim, graph similarity, and code embeddings are useful cheap retrieval
signals. Existing vector research in `FUNC_VECTOR_RESEARCH` should be treated as a
prefilter experiment, not a semantic authority. They cannot reliably identify
inlined or transformed behavior and can produce high-confidence-looking nearest
neighbors without a contract contradiction test.

### 5.7 Active learning and decision trees

Information gain is appropriate for selecting the next probe after the candidate
set and their predicted observations are available. Cost, safety, memory layout,
and expected ambiguity must be included. A learned ranker is optional and must be
evaluated out-of-sample; rule-based discrimination is the baseline because it is
auditable.

## 6. Proposed Overall Architecture

```text
P-Code bundle
    |
    v
Parse and normalize -----------------------------+
    |                                             |
    v                                             |
CFG, SSA, memory versions, effects, calls        |
    |                                             |
    v                                             |
Structural family/index prefilter                |
    |                                             |
    v                                             |
Seeded semantic-region beam                      |
    |                                             |
    +--> external/context interface hypotheses   |
    +--> callee policy hypotheses                 |
    +--> type/role and alias hypotheses           |
    |                                             |
    v                                             |
Contract compatibility filter                    |
    |                                             |
    v                                             |
Probe planner <---- database schemas/relations   |
    |                                             |
    v                                             |
Concrete emulator / optional concolic executor   |
    |                                             |
    v                                             |
Normalized observations and evidence             |
    |                                             |
    +--> candidate elimination                    |
    +--> counterexample generation --------------+
    |
    v
Calibrated accept/review/UNKNOWN decision
    |
    v
Bottom-up summaries and surrounding-region refinement
```

The search is staged and budgeted. Each stage may preserve several hypotheses,
but it must expose counts and pruning reasons. A practical implementation uses:

- an indexed database by role/effect/family/call signature;
- a beam of region/interface/callee-policy hypotheses;
- memoized execution keyed by region, normalized input, policy, and emulator
  version;
- early rejection on hard contradictions;
- a final acceptance gate stricter than the retrieval score.

### 6.1 Recognition phases

1. Validate and normalize P-Code.
2. Build basic blocks, CFG, SSA, memory/effect summaries, and call graph edges.
3. Extract cheap structural features and seed operations.
4. Generate candidate regions including whole function, calls, loops, SESE
   fragments, and dependence closures.
5. Infer boundary inputs, outputs, roles, memory frames, alias alternatives, and
   call policies.
6. Retrieve compatible database families.
7. Run a common baseline probe suite.
8. Select candidate-discriminating probes adaptively.
9. Run counterexample and adversarial probes for the leader.
10. Calibrate evidence and either accept, retain for a later recursive pass, or
    abstain.
11. Replace accepted subregions with summaries and repeat caller analysis to a
    fixed point or bounded number of rounds.

## 7. Analysis IR Design

### 7.1 Design principles

The IR should be analysis-friendly rather than pretty source code. It should have
multiple synchronized views:

- immutable normalized P-Code operations for semantic execution;
- basic blocks and CFG edges for reachability;
- SSA values and phi nodes for data flow;
- memory access nodes and version edges for memory dependence;
- call/effect nodes for observable behavior;
- region interfaces and observations for recognition.

Do not lower away original P-Code. Each derived node stores source operation IDs,
addresses, widths, and transformations so a finding can be explained and a
candidate can be executed exactly.

### 7.2 Core data structures

The Python implementation should use dataclasses or immutable records with stable
integer IDs.

```text
PcodeBundle:
    architecture, endian, pointer_width
    spaces: address-space descriptors
    functions: normalized FunctionIR records
    externals: import/API metadata

Op:
    id, mnemonic, inputs, output, address, sequence
    block_id, source_varnodes, userop, width metadata

Value:
    id, width, kind=(constant|input|temporary|register|phi|memory-token)
    defs, uses, role_hypotheses, provenance

BasicBlock:
    id, op_ids, predecessors, successors
    entry/exit addresses, loop_id, dominator metadata

MemoryAccess:
    id, kind=(read|write|call|phi), space
    address_expression, width, alias_class, reaching_versions

CallNode:
    id, target_kind, normalized_identity, argument_values
    return_values, memory/effect summary, indirect-target hypotheses

FunctionIR:
    ops, blocks, cfg, values, memory_graph, calls
    dominators, post_dominators, loops, effects, feature_index
```

Address expressions should be represented as a small symbolic form, such as a
base value plus constant offset and optional bounded index. Preserve an opaque
expression when simplification is unsafe. Do not turn a raw address into a
semantic buffer name until it has been assigned by an input hypothesis.

### 7.3 Normalization rules

- Preserve operation widths and mask semantics at operation boundaries.
- Canonicalize register aliases to a physical storage plus slice, retaining the
  original varnode for diagnostics.
- Give unique temporaries stable per-function IDs; do not use host object IDs.
- Canonicalize constant expressions only when the operation semantics permit it.
- Normalize endianness and pointer width in the bundle header.
- Preserve signedness as an operation/use property, not as an assumed type.
- Keep `CALLOTHER`, floating-point operations, and unsupported ops as explicit
  nodes with capability flags.
- Distinguish direct, indirect, imported, tail, and unresolved calls.

### 7.4 CFG construction

Flatten instruction-level P-Code only after retaining instruction/frame boundaries.
Identify branch, conditional branch, call, return, and fall-through successors.
Resolve constant-relative P-Code targets using the frame convention; resolve
direct absolute targets through the extracted address map. An unresolved target
creates an explicit unknown edge. Split blocks at branch targets, branch
terminators, call/return boundaries, and exception-like operations when known.

CFG invariants:

- every operation belongs to exactly one block;
- all normal successors are either block IDs or explicitly unknown;
- unreachable blocks are retained but marked;
- entry and return blocks are explicit;
- a call is not silently treated as a fall-through-only operation when it can
  modify memory or control flow.

### 7.5 SSA and memory SSA

Construct dominator-based SSA for register and unique-space definitions. At joins,
insert phi values for locations with multiple reaching definitions. For a region,
values defined outside it and used inside are boundary inputs; values defined
inside and live after an exit are boundary outputs.

Construct a conservative memory graph:

- each address-space entry has an initial memory token;
- each store creates a new memory version for its possible alias classes;
- each load records reaching must/may versions;
- unknown-address stores touch an `UNKNOWN_MEMORY` partition;
- calls consume and produce versions according to their effect summary;
- memory phi nodes merge versions at CFG joins.

This is not a proof of alias disjointness. It is a dependency structure. If a
candidate relies on a may-alias edge, the uncertainty is carried into the
interface and probe plan.

Complexity should be approximately linear in operations plus edges for ordinary
CFG/SSA construction. Memory dependence can become quadratic with many unknown
accesses; cap partitions and fall back to a summary edge when the cap is reached.

## 8. Input, Output, Role, and Context Inference

### 8.1 Boundary definition

For a candidate region `R`, compute its minimal conservative external state:

```text
ExternalState(R) =
    SSA values read before an in-region definition
  + memory locations/expressions with reaching definitions outside R
  + incoming flags that influence a reachable operation
  + globals/TLS/context partitions read by R
  + call-policy/environment tokens required by R
```

Do not classify a value as a parameter merely because it is in an ABI argument
register. For an inline region, ABI registers may contain unrelated values. The
ABI is a prior, not evidence.

Outputs include values live across each exit, returned values, modified memory
partitions, calls/events, relevant flags, and termination/exception state. Dead
stores and dead flags are not contract outputs unless they are observable through
an unknown alias or policy.

### 8.2 Role hypotheses

Assign a weighted set of roles rather than one inferred C type:

```text
INTEGER, SIGNED_INTEGER, UNSIGNED_INTEGER, BOOLEAN, FLOAT, DOUBLE,
POINTER, BUFFER, STRING, LENGTH, HANDLE, ENUM, INDEX, ADDRESS, VECTOR,
MASK, CONTEXT, UNKNOWN
```

Evidence includes width, signed comparisons, pointer-like arithmetic, loads and
stores, loop induction, zero-termination checks, API argument position, alignment,
and use as a shift/count. Roles are hypotheses with provenance, for example:

```text
v17:
    POINTER 0.65 (used as RAM address)
    INTEGER 0.35 (also compared arithmetically)
```

The planner should retain the top few role assignments when their score margin is
small. It should not spend exponential effort on all combinations; use a capped
beam and merge hypotheses with the same observable memory layout.

### 8.3 Memory frame inference

Represent a probe memory frame as named abstract objects, not absolute addresses:

```text
INPUT_BUFFER_0, INPUT_BUFFER_1, OUTPUT_BUFFER_0,
STACK_FRAME, GLOBAL_0, HEAP_OBJECT_0, UNKNOWN_EXTERNAL
```

Each object has size bounds, alignment, initialization pattern, mutability, and
alias relation. A pointer hypothesis maps an SSA value to object plus offset. A
length hypothesis maps a value to a size expression. Sentinel bytes and canaries
must surround allocated objects to detect overread/overwrite.

### 8.4 Alias hypotheses

Use the lattice `MUST_ALIAS`, `MUST_NOT_ALIAS`, `MAY_ALIAS`, `UNKNOWN`. Generate
explicit probes for equal, disjoint, and overlapping objects when the contract is
alias-sensitive. A probe that assumes disjointness must not be used as evidence
against an implementation whose contract permits overlap.

### 8.5 Context inputs

If a candidate reads a global, TLS slot, incoming flag, memory outside its named
arguments, or an unresolved call result, represent that as a context token. Probe
plans can vary it only when safe; otherwise the result is conditional:

```text
strlen-under-context(global_2 = value_class_A)
```

A candidate that cannot be executed with a stable context must not receive a
pure-function acceptance label.

## 9. Candidate Semantic-Region Discovery

### 9.1 Why intervals are rejected

All `O(N^2)` instruction intervals include enormous numbers of syntactically valid
but semantically meaningless fragments. They also fail when a compiler splits a
logical operation across blocks or merges shared computations. Region discovery
must be seed-driven and dependency-aware.

### 9.2 Region representation

```text
RegionHypothesis:
    op_ids or block_ids
    entry_frontier, exit_frontier, internal_edges
    interface (inputs, outputs, memory, flags)
    closure_reason and omitted-dependency summary
    callee policies
    role/alias hypotheses
    structural features
    confidence, status, evidence, budget counters
```

The region may have multiple entries/exits. A strict SESE candidate is preferred
for easy execution, but a multi-entry or sliced candidate can be retained when
its interface explicitly contains incoming control/context predicates.

### 9.3 Seed sources

Generate seeds from:

- the whole function;
- direct/imported/API call nodes;
- loops and SCCs;
- return-producing value definitions;
- distinctive operations such as string terminator scans, vector instructions,
  population count, division, allocation patterns, or floating-point intrinsics;
- recognized child regions and callee summaries;
- repeated dependence motifs and strongly connected producer/consumer clusters;
- known database structural anchors.

### 9.4 Candidate generation algorithms

The prototype should implement and compare three algorithms.

#### A. SESE structural regions

Use dominator/post-dominator pairs to enumerate bounded single-entry/single-exit
regions, then include natural loops and call bodies. This is cheap and executable,
but can miss split or multi-entry inline computations.

#### B. Backward/forward dependence closure

Start from a seed output, call, or effect. Add SSA producers, required memory
definitions, control predicates, and side-effect dependencies. Optionally grow
forward to selected consumers. Stop when the interface is stable or a size budget
is reached. This captures semantic slices but can accidentally absorb surrounding
logic.

#### C. Beam search over region frontiers

Start with a seed and repeatedly add a frontier block/op group. Score additions by
dependence closure, interface reduction, call/effect coherence, database family
compatibility, and size penalty. Keep the best `K` states, retain alternatives
with materially different boundaries, and always keep the whole-function state.

Recommended implementation: use A for the initial pool, B to close dependencies,
and C only when B produces ambiguous boundaries. Do not use a learned model until
these baselines are measured.

### 9.5 Completeness and pruning

Completeness cannot mean enumerating every subgraph. Define an empirical safety
contract: for corpus ground truth, a true region must be represented by at least
one candidate whose omitted computations are either dead or included in its
explicit interface. Track missed-region rate separately from candidate count.

Safe/near-safe pruning rules:

- discard unreachable-only fragments;
- discard a region with no seed, effect, output, or recognized dependency;
- reject candidates whose hard effect signature contradicts every database family;
- merge candidates with identical op membership and equivalent interface;
- subsume a candidate only when its observations and interface are strictly
  covered by a larger candidate and the smaller one has no distinct exit/effect;
- cap depth, frontier size, and number of alias/role hypotheses, but label the
  result as budget-limited rather than complete.

### 9.6 Expected behavior by code shape

| Shape | SESE | Dependence closure | Beam/frontier |
| --- | --- | --- | --- |
| Straight-line inline code | strong | strong | unnecessary |
| Natural loop | strong if loop header is seed | strong | useful for surrounding context |
| Branching implementation | may produce several regions | includes predicates | useful for boundary alternatives |
| Partially inlined function | often too small/large | best initial candidate | needed for split boundaries |
| Merged/vectorized computation | can miss semantic partition | may over-include | retains several weighted hypotheses |
| Nested inline functions | child seeds plus closure | compositional | bottom-up refinement |

## 10. Calls and Callees

### 10.1 Normalized call event

Every call has a canonical identity when available:

- imported API semantic identity (`kernel32!CreateFileA`);
- known library identity and version;
- internal stable function ID, not a raw address alone;
- indirect target set or `UNKNOWN_INDIRECT`.

Record argument expressions and normalized values/roles, pointer-object mappings,
return value token, memory/effect summary, call order, branch/path conditions, and
whether the event was executed, summarized, or opaque.

### 10.2 Callee policy lattice

Use a policy object rather than only two flags:

```text
KNOWN_EXTERNAL       use a trusted contract/event model
EXECUTE              enter the callee under a bounded recursive budget
SUMMARY              use a cached semantic summary
OPAQUE               record identity, normalized arguments, and unknown effects
PARTIAL              execute selected blocks or known child summaries
SPECULATIVE          execute one branch/policy as a hypothesis
UNKNOWN              preserve an unconstrained event/effect
```

Policy selection depends on call identity, database coverage, recursion/SCC state,
cost, and whether the caller candidate needs callee outputs. For precision, keep
`EXECUTE` and `OPAQUE` alternatives when their cost is within budget. An opaque
call can support recognizing a wrapper based on call identity even when its body
is unavailable.

### 10.3 Recursive recognition

Analyze the call graph bottom-up where possible. For recursive SCCs:

1. initialize calls inside the SCC as opaque summaries;
2. recognize non-recursive child regions;
3. replace accepted child regions with summaries;
4. iterate summaries to a fixed point or `max_rounds`;
5. keep the least precise safe summary when convergence fails.

Never recursively execute without a call-depth, SCC, step, and state-size budget.
Memoize summaries by normalized callee IR hash, policy, and context schema.

## 11. External API Modeling

The environment is modeled as an event-producing abstract machine. It does not
open real files, sockets, threads, or Windows handles during recognition.

API specifications contain:

- identity and aliases/forwarders;
- parameter roles and width/ABI mapping;
- pointer read/write/size relationships;
- return role and failure classes;
- event side effects and determinism class;
- allowed call order or partial-order constraints;
- probe generation rules;
- normalization of opaque handles, addresses, and error values.

For `CreateFileA`, an event may retain normalized path bytes, access/share/create
flags, security/attribute role classes, and a symbolic handle token. For a
filesystem-dependent result, the output is an abstract success/failure class or
an event, not a host-specific handle.

API calls should influence classification through independent channels:

1. identity and call order;
2. argument relationships and role compatibility;
3. memory/effect compatibility;
4. returned-token use;
5. surrounding computation.

A call-name match alone must never accept a candidate. Wrappers that call the same
API are expected; their argument and effect context differentiates them.

## 12. Behavioral Execution Model

### 12.1 Concrete engine

Extend the existing emulator behind an adapter interface:

```text
execute(region, probe, policy, limits) -> ExecutionResult
```

The result must include observations even on bounded timeout or unsupported
operation. Use state snapshot/restore around regions and calls. Preserve P-Code
width, endian, partial-register behavior, and branch semantics. Add explicit
handlers or capability flags for floating point, vector operations, userops,
indirect calls, and exceptions; never silently treat them as integer operations.

### 12.2 Path and loop control

Concrete execution follows a probe-selected path by default. For branch coverage,
run several probes or use concolic branch constraints. A per-run step limit,
loop-iteration limit, call-depth limit, and memory-allocation limit are mandatory.

Loop handling tiers:

1. bounded concrete runs with several input lengths;
2. repeated-state/induction detection for simple loops;
3. relational loop summaries such as `scan_until_zero`, `copy_n`, or
   `compare_until_difference`;
4. optional symbolic invariant discovery;
5. `INCONCLUSIVE_LOOP` when no safe summary is available.

An execution timeout is evidence about resource behavior only; it is not a
candidate mismatch. Partial traces remain useful for path/call pruning.

### 12.3 Hybrid execution

Use symbolic values selectively. A symbolic expression need not model all bytes;
it can track relationships such as `output[i] = input[j]`, zero tests, length
constraints, and branch conditions. When a solver finds a counterexample, replay
it concretely before using it as evidence.

## 13. Observable State and Fingerprints

### 13.1 Observation schema

```text
Observation:
    status: COMPLETE | TIMEOUT | UNSUPPORTED | FAULT | INCONCLUSIVE
    return_values: normalized values/tokens and widths
    live_register_outputs: only boundary-live locations
    modified_memory: byte/range diffs plus input/output object relations
    read_memory: relevant ranges, sentinel reads, uninitialized reads
    flags: only live or contract-declared flags
    calls: normalized ordered/partial-order events
    branches: path predicates or abstract branch classes
    termination: return, exception, non-return, allocation failure class
    environment_events: time/random/io/atomic abstractions
    resource: steps, loop counts, calls, bytes touched
```

Separate raw instrumentation from normalized semantic channels. Raw traces are
needed for debugging; normalized observations are used for matching.

### 13.2 Memory normalization

Compare only semantically relevant bytes and relations:

- bytes written relative to `INPUT_BUFFER_0`/`OUTPUT_BUFFER_0`;
- changed ranges and length expressions;
- copied/permuted/equal/zeroed relationships;
- overlap behavior and untouched canaries;
- read-before-write and sentinel access;
- abstract global/heap effects.

Do not compare absolute emulator addresses. Handle allocation pointers as symbolic
tokens with size/alignment attributes. Preserve raw addresses only as diagnostics.

### 13.3 Trace and partial-order normalization

Call order matters for APIs such as open/read/close. Independent pure operations
do not need a total instruction trace. Store a sequence of observable events and,
where control paths merge, a partial order with path predicates. This prevents
register allocation and scheduling details from dominating the fingerprint.

### 13.4 Fingerprint limitations

A fingerprint is a compact index and evidence cache, not a proof. Store:

- cheap structural feature hashes;
- canonical effect signature;
- contract relations;
- selected probe/observation pairs;
- known confusers and discriminating probes;
- coverage and provenance.

Use hashes only for retrieval and cache keys. Never accept solely on hash equality.

## 14. Probe Generation

### 14.1 Probe object

```text
Probe:
    role_assignment
    scalar_values
    memory_objects and byte contents
    pointer mappings and offsets
    alias_constraints
    initial globals/flags/context
    expected safety/termination bounds
    purpose: baseline | boundary | alias | metamorphic | discriminator
```

### 14.2 Probe tiers

1. **Schema probes:** zero, one, all bits, signed boundaries, powers of two,
   alignment boundaries, empty/single/maximum small lengths.
2. **Memory probes:** distinct buffers, same pointer, partial overlap in both
   directions, canaries, embedded zero, no zero within bound, unaligned pointers.
3. **Control probes:** values that force each known comparison outcome and loop
   exit; generated from branch constraints where possible.
4. **Metamorphic probes:** append a nonzero byte to a string, vary copy length,
   swap disjoint buffers, repeat a pure operation, or transform inputs according
   to a contract relation.
5. **Adversarial probes:** values likely to separate a candidate from a confuser,
   including signed overflow, NaN/negative zero, overlapping memory, and boundary
   failure modes.

Random probing is useful after structured coverage. Seed the PRNG and record the
seed, memory layout, and purpose for reproducibility.

### 14.3 Ring/vector hypothesis

Finite rings such as `Z_N` are useful for scalar modular arithmetic, bit-vector
operation coverage, reproducibility, and algebraic relation tests. They are not a
universal semantic input model. They do not express strings, pointer validity,
aliasing, external resources, floating-point NaNs, or ABI layout by themselves.

Implement ring probes as one lane in a product probe domain:

```text
ProductProbe = scalar bit-vectors
             x role/schema assignments
             x memory object graphs
             x alias relations
             x boundary/control choices
             x environment policy
```

Compare ring-only, structured-only, random-only, and adaptive mixtures in an
ablation experiment. Do not retain ring probing merely for mathematical elegance.

## 15. Adaptive Probe Selection

Given surviving candidate contracts `C`, estimate each candidate's predicted
observation for an available probe `p`. Select the probe using a cost-aware score:

```text
score(p) = expected weighted partition reduction
         + expected contradiction value
         + branch/effect coverage gain
         - execution_cost(p)
         - safety/context risk(p)
```

For probabilistic candidates, weighted entropy or expected posterior loss is
appropriate. For deterministic exact predictions, use partition size and pairwise
separation. Include a minimum baseline suite so an accidental tie is not accepted
after one lucky probe.

Candidate-specific probes should target the strongest remaining ambiguity. Examples:

- `memcpy` vs `memmove`: overlapping buffers with both copy directions;
- `strlen` vs bounded scan: a zero after the candidate's suspected bound;
- `strcmp` vs boolean equality: unequal strings with different sign ordering;
- `cos` approximation vs unrelated polynomial: large, small, negative, NaN, and
  near-zero values with a declared floating-point contract;
- wrapper A vs wrapper B: argument values that produce distinct normalized API
  events.

Stop when one candidate passes all mandatory discriminators, its evidence margin
is calibrated, and a held-out confirmation probe passes. Stop with `UNKNOWN` when
the candidate set remains observationally equivalent under the available budget.

## 16. Function Database

### 16.1 Hybrid entry

The minimum useful database entry is not a huge manual metadata file. It is:

```text
FunctionEntry:
    stable identity and family
    contract level and determinism/environment policy
    parameter/return role schema (possibly partial)
    memory/effect relation summary
    call/event schema, if relevant
    executable reference or summary model
    generated baseline and discriminator probes
    normalized expected observations
    confusers and mandatory negative tests
    provenance, compiler/ABI/version, coverage
```

Role schema and effects are more valuable than source names alone. Unknown fields
are allowed. Every field has a provenance (`header`, `reference execution`,
`symbolic derivation`, `manual`, or `inferred`) and confidence.

### 16.2 Contract families

Represent common families parametrically:

- `strlen`: result is the first zero index within a valid string object;
- `strnlen`: result is the minimum of zero index and bound;
- `memcpy`: output prefix equals source prefix for `n`, with no overlap allowed;
- `memmove`: same relation but defined for overlap and direction-sensitive;
- `memset`: output prefix equals repeated byte value;
- `strcmp`: sign/order relation at first differing byte, exact return policy marked;
- `malloc`: symbolic fresh object with requested size and failure policy;
- API wrappers: event identity and normalized argument relationships;
- arithmetic/math: bit-vector or floating contract plus exceptional behavior.

Raw observations remain attached as regression evidence. Contracts generalize
beyond the exact generated probes and give the matcher a reason for a decision.

### 16.3 Automatic construction

The database builder should accept a reference implementation, known prototype or
role schema, optional headers/docs, and an execution adapter. It then:

1. compiles or imports the reference;
2. extracts its P-Code or invokes a trusted reference executor;
3. generates schema/boundary/alias/metamorphic probes;
4. collects normalized observations;
5. infers candidate relations and effect summaries;
6. asks a human only to resolve ambiguous contract choices;
7. generates confuser/discriminator tests;
8. stores provenance and versioned artifacts.

Adding a function should not require writing thousands of expected output rows.
However, automatic inference cannot safely invent API contracts, undefined behavior
policy, or environment semantics. Those fields require explicit review.

## 17. Candidate Matching and False-Positive Defense

### 17.1 Cascaded matcher

Use the following order:

1. **Capability filter:** architecture, supported opcodes, floating/vector needs,
   direct/indirect call availability.
2. **Structural filter:** width, loop/branch/effect/call features, cheap graph
   signature, and BSim/embedding retrieval if enabled.
3. **Interface filter:** role, arity hypothesis, memory objects, alias cases,
   output channels, and required context.
4. **Hard semantic checks:** impossible return/effect/call contradictions.
5. **Baseline execution:** compare multiple independent probes by channel.
6. **Adaptive discrimination:** select probes against surviving competitors.
7. **Counterexample search:** actively attack the leader and replay any witness.
8. **Acceptance gate:** calibrated thresholds, minimum evidence, margin, and no
   unresolved mandatory channel.

### 17.2 Evidence record

Each candidate keeps:

```text
Evidence:
    matched probes by channel
    failed probes and exact contradiction
    unexecuted/inconclusive probes and reason
    structural compatibility factors
    interface/alias/role assumptions
    call policy and summary provenance
    candidate margin and calibration bucket
    resource limits and unsupported operations
```

The final report must be explainable. A score without evidence is insufficient.

### 17.3 Acceptance policy

Use class-specific thresholds. A pure `strlen` contract may require complete
memory and loop evidence; an opaque API wrapper may rely more on call identity and
argument relations. Require:

- at least two non-equivalent probe categories;
- all mandatory contract invariants passed;
- no contradiction in a contract-relevant channel;
- a minimum leading-vs-runner-up margin;
- a held-out or adversarial confirmation when available;
- no unmodeled effect that could change the label.

If these conditions are not met, return `UNKNOWN` or `REVIEW`, not the nearest
candidate.

### 17.4 Explicit false-positive example

The implementation:

```text
fake_strlen(s) {
    if (s[0] == 0) return 0;
    if (s[1] == 0) return 1;
    if (s[2] == 0) return 2;
    return 3;
}
```

matches a naive probe set containing only strings of length 0 through 3. A
candidate-specific probe with a nonzero first four bytes and a zero at byte 4
returns 3 rather than 4, producing a concrete contradiction. A no-terminator
bounded-memory probe also distinguishes it from an unbounded `strlen` policy.

Never call the candidate `strlen` merely because seven small examples matched.

## 18. Uncertainty and Hypothesis Management

Represent uncertainty explicitly in a capped hypothesis set:

```text
HypothesisState:
    region alternative
    input mapping alternative
    role assignments
    alias relation choices
    callee policy choices
    environment assumptions
    weight/evidence
```

Combine hypotheses by lazy branching. Merge states when they have equivalent
observable interface and policy; prune only on hard contradiction or dominated
weight plus no unique evidence potential. The beam is a computational budget, not
a claim of completeness. Record discarded alternatives and the reason.

A hypothesis can be:

- `PROPOSED`, `TESTED`, `SUPPORTED`, `CONTRADICTED`, `INCONCLUSIVE`, or
  `ABSTAINED`.

Keep both executed and opaque callee modes when their downstream observations are
materially different. Preserve multiple region boundaries until one is separated
by evidence or a later summary pass.

## 19. Recursive and Hierarchical Recognition

Recognition should be iterative and primarily bottom-up, with bounded top-down
context refinement:

1. recognize direct external/API events and cheap leaf regions;
2. recognize internal callees using execute/summary/opaque policies;
3. attach accepted child summaries to the caller IR;
4. regenerate caller region candidates with child nodes as semantic atoms;
5. classify whole-function and proper subregions together;
6. repeat until no accepted summary changes the caller's candidate ranking.

Do not force an exact partition of a function. Overlapping records are allowed when
they describe nested semantic facts. A parent contract can contain recognized
children as evidence without claiming that all parent instructions belong to one
source function.

## 20. Optimization and Caching

Cache at several levels:

- normalized IR by P-Code bundle hash and normalizer version;
- CFG/SSA/memory analysis by IR hash and analysis configuration;
- candidate region/interface signatures;
- reference contract observations by entry/probe hash;
- emulator runs by region hash, probe hash, call policy, and emulator version;
- callee summaries by callee IR/context/policy;
- probe discrimination predictions by candidate/probe schema.

Use copy-on-write or snapshot/restore emulator states. Share a common setup prefix
for probes with the same memory frame. Normalize objects before hashing so host
addresses never invalidate caches. Include all semantic configuration in cache
keys; a stale cache can create a false recognition.

At GTA5 scale, prefiltering and indexing must happen before execution. Analyze
functions independently in parallel only after proving that global/context cache
keys are correct. Keep a per-function resource budget and a global work queue.

## 21. GTA5-Specific Considerations

GTA5 is a large, optimized, C/C++-heavy Windows binary. Expect:

- aggressive inlining, templates, LTO-like transformations, and cloned code;
- STL/runtime patterns and allocator wrappers;
- SIMD/vector and floating-point operations;
- import thunks, tail calls, jump tables, indirect dispatch, and exception paths;
- Windows ABI details, shadow space, partial registers, structure returns,
  variadic or unusual calls, TLS, and SEH/unwinding;
- third-party networking, physics, graphics, audio, and custom engine code;
- stripped or incomplete symbols and uncertain external identities.

The initial benchmark should therefore use a staged corpus, not the whole game
binary immediately. First validate on extracted, labeled slices and imported API
wrappers. Then measure scaling, unsupported-operation rate, and abstention on
representative GTA5 functions. A high precision result on ten toy arithmetic
functions is not evidence that the architecture works at GTA5 scale.

Likely strong cases:

- direct imported API wrappers with stable argument/effect patterns;
- complete scalar library functions with clear memory contracts;
- leaf loops such as scans, copies, compares, and reductions;
- ordinary functions where the whole-function region is valid.

Likely weak cases:

- heavily merged partial inlines with no unique interface;
- indirect calls with large target sets;
- global/TLS/stateful engine code;
- floating-point approximations with undocumented accuracy contracts;
- vector/SIMD userops not supported by the emulator;
- exception/concurrency behavior and undefined behavior.

Minimize these failures by preserving unknown channels, using API/event evidence,
adding capability-aware thresholds, and reporting per-family coverage instead of
forcing labels.

## 22. Alternative Architectures

| Architecture | Strengths | Weaknesses | GTA5 suitability | Decision |
| --- | --- | --- | --- | --- |
| Pure concrete fingerprints | Simple, observable, easy to prototype | Finite-probe collisions, weak unknown-interface handling | Good for leaf baseline only | Baseline, not final |
| Whole-program symbolic equivalence | Strong relational reasoning | Path/loop explosion, difficult environment model | Poor as default | Selective counterexamples only |
| Pure structural graph matching | Fast, transformation-tolerant | Confuses similar semantics, weak inline boundaries | Useful retrieval | Prefilter only |
| Embedding/learned ranking | Scales retrieval, may generalize | Calibration and adversarial failure opaque | Candidate ranking | Optional ablation |
| Contract/summary matching | Generalizes probes, explains effects | Database construction cost | Strong for libraries/APIs | Required core |
| Hybrid static + concrete + symbolic | Static pruning plus falsifiable evidence | More engineering and state management | Best balance | Recommended |
| Top-down only | Uses caller context | Misses leaf/callee evidence | Weak for nested code | Not primary |
| Bottom-up fixed-point | Composes summaries and handles nesting | Recursive SCC complexity | Good with budgets | Recommended with bounded refinement |
| Exhaustive interval enumeration | Apparent completeness | O(N^2), mostly meaningless, still misses split regions | Unusable | Reject |

The recommended design is not one algorithm but a cascade. Each weaker/cheaper
signal is allowed to retrieve or prune, while only contract-compatible behavioral
evidence can produce a high-confidence label.

## 23. Recommended Design in One Algorithm

```text
recognize(function_pcode, database, config):
    ir = normalize(function_pcode)
    validate_capabilities(ir)
    analysis = build_cfg_ssa_memory_effects(ir)

    seeds = find_structural_and_database_seeds(analysis)
    regions = {whole_function(analysis)}
    regions += sese_candidates(seeds, analysis, config.region_budget)
    regions += dependence_closed_candidates(seeds, analysis, config.closure_budget)
    regions = deduplicate_and_beam_prune(regions)

    child_summaries = lookup_or_recognize_callees(analysis, database, config)
    regions = attach_summary_atoms(regions, child_summaries)

    hypotheses = []
    for region in regions:
        interfaces = infer_external_interfaces(region, analysis)
        for interface in capped_beam(interfaces):
            families = database.retrieve(interface, region.features)
            for family in families:
                hypotheses += expand_roles_aliases_policies(region, interface, family)

    hypotheses = hard_filter(hypotheses)
    evidence = run_baseline_probes(hypotheses, config)
    while has_discriminating_budget(evidence):
        probe = select_cost_aware_probe(evidence)
        evidence = execute_and_update(evidence, probe)
        evidence = eliminate_contradictions(evidence)
        if acceptance_or_abstention_is_decidable(evidence):
            break

    for leader in leaders(evidence):
        witness = seek_counterexample(leader, evidence, config)
        if witness is found:
            evidence = execute_and_update(evidence, witness)

    return calibrated_decisions(evidence), evidence, analysis
```

The algorithm's essential invariant is that every accepted label has a concrete
evidence path and every discarded hypothesis has a recorded contradiction or
budget reason. The algorithm does not promise to find every true region; the
region-recall experiment quantifies that limitation.

## 24. Worked Examples

### Example A: ordinary `strlen`

The whole function candidate contains the entry, loop header, zero-byte load,
increment, compare, and return. SSA finds the pointer and induction value as
external/input-derived values and the returned count as an output. Memory analysis
marks a read-only byte range and a zero-terminator condition. The database
retrieves `strlen`, `strnlen`, `memchr`, and custom scan families.

Structured probes include empty, one-byte, embedded-zero, long, unaligned, and
canary-guarded strings. A length-dependent relational observation confirms that
the return value is the first zero index and that no bytes are modified. A probe
with a zero beyond a suspected bound rejects `strnlen`; a probe with a target byte
rejects `memchr`. The whole function is accepted as `strlen` only if the database
contract and required safety policy agree. The result is not based on matching
only the return register.

### Example B: inline `strlen`

In a caller, the scan's producer/consumer slice is seeded by the value later used
as a length. Backward closure includes the loaded byte, zero comparison,
induction phi, and loop control; forward closure stops at the first external use
of the length. The surrounding caller values become context inputs or are left
outside the candidate. If compiler scheduling splits operations across blocks,
the candidate contains those blocks with a multi-exit interface or is widened to a
SESE loop region.

The region is executed with a memory frame mapping the inferred pointer to
`INPUT_BUFFER_0`. Its output is the live length value and its read footprint. The
caller remains a separate candidate. If the scan is fused with a later clamp, the
recognizer may produce both a `strlen` child region and a larger `strlen_then_min`
semantic region, with evidence showing the different contracts.

### Example C: optimized inline `cos`

A floating-point candidate may contain range reduction, polynomial evaluation,
FMA/vector operations, and special-case branches rather than a call to `cos`.
The IR preserves FP widths, rounding-sensitive operations, constants, and NaN/
exception paths. Structural features retrieve trigonometric and polynomial
families, but do not decide.

The database declares the contract: exact libm identity, correctly rounded result,
or an allowed error bound. Probes cover zero, signed zero, small/large positive and
negative values, quadrants, infinities, NaNs, and values near reduction boundaries.
Compare using bitwise or ULP policy declared by the entry. If unsupported SIMD or
FP semantics make the observation incomplete, return `UNKNOWN` rather than
labeling a similar polynomial `cos`.

### Example D: `CreateFileA` wrapper

The call node is a strong seed. The event records normalized path bytes, access,
share, creation, and attribute roles, plus a symbolic handle result. The wrapper's
own return/error transformation and pre/post calls are part of the surrounding
contract. The API event is not executed against Windows.

A function that calls `CreateFileA` with a constant path and returns a boolean is
not automatically the general `CreateFileA` wrapper. The argument relation and
return policy distinguish it. If the call is imported through a thunk, identity
normalization resolves the forwarding chain when possible and retains an
`IMPORT_FORWARD` note when it cannot.

### Example E: recursively executed internal callee

For `F -> sub_140123456`, the call graph identifies a direct internal target. If
the callee is within budget, recognize or summarize it first. The caller execution
enters a fresh frame with shared abstract memory and a call-context token. Its
normalized trace includes the callee's accepted summary and any external events.

If the callee contains `ReadFile`, the parent may be recognized as a
`read-wrapper` only after the callee summary and argument/effect relations compose.
Call depth and SCC guards prevent uncontrolled recursion.

### Example F: opaque internal callee

Under `OPAQUE`, the emulator does not enter `sub_140123456`. It emits:

```text
CALL(INTERNAL:stable_sub_id,
     args=[POINTER(INPUT_BUFFER_0), LENGTH(v9)],
     return=TOKEN_0,
     effects=UNKNOWN,
     policy=OPAQUE)
```

This can recognize a caller's call pattern or wrapper while avoiding a costly or
unavailable body. Because effects are unknown, the parent cannot receive a pure
memory-contract label unless other evidence bounds the effects. A later analysis
pass can replace the event with an accepted summary.

### Example G: nested structure

For `F` containing inline A, inline B, external C, and internal D containing E,
the first pass seeds C, loop/slice patterns in B/A, and the call to D. It recognizes
leaf C and D/E under available policies, then inserts their summaries as semantic
atoms. The second pass evaluates A and B with those atoms and the whole F with all
events. Results may be:

```text
region A -> contract A (supported)
region B -> contract B (supported)
call C  -> known external event
callee D -> opaque or summary (policy recorded)
F       -> composite/wrapper contract or UNKNOWN
```

The system need not force every op into exactly one recognized source function.

### Example H: finite-probe false positive

The bounded `fake_strlen` above returns correct results for short probes. The
adaptive planner notices that `strlen` and `fake_strlen` predict different results
for a nonzero prefix of length four. It allocates a canary-backed buffer, executes
the probe, and records the mismatch. If the fake implementation also handles that
case, the planner chooses an embedded-zero, no-terminator, or larger-length probe.
If all available probes remain equivalent within the declared contract, the result
is `UNKNOWN`, not a forced `strlen` label.

## 25. Research Questions: Explicit Answers

1. **Mathematically correct abstraction:** context-parametric partial observational
   equivalence of a region, not a bare `X -> Y` map.
2. **Region, slice, subgraph, transition, or summary:** all are views. The
   executable object is a CFG subgraph with an explicit boundary; its semantic
   identity is a contract/summary over that subgraph.
3. **Best IR:** normalized P-Code plus CFG, SSA, conservative MemorySSA-like
   versions, effect/call nodes, and role hypotheses; no source-level decompiler IR
   is required.
4. **SSA/memory SSA:** use SSA for value boundary and slicing; use memory versions
   for reaching loads/stores and conservative effect closure.
5. **Avoiding region explosion:** seeds, SESE regions, dependence closure, beam
   search, deduplication, subsumption, and budgets; never enumerate intervals.
6. **Conservative completeness:** retain whole function, all seed classes, explicit
   omitted dependencies/context, and multiple boundary alternatives; measure miss
   rate rather than claiming proof.
7. **Hidden/context inputs:** boundary use-def, memory reaching definitions,
   flags, globals/TLS, unresolved call results, and path predicates.
8. **Memory inputs/outputs:** abstract named objects, address expressions, ranges,
   alias lattice, read/write relation, canaries, and symbolic allocation tokens.
9. **Aliases:** retain must/may/not/unknown hypotheses and actively probe equal,
   disjoint, and overlap cases.
10. **Types/roles:** infer lightweight semantic roles from operations and uses;
    retain weighted alternatives rather than requiring exact C types.
11. **Calls:** normalized identity, arguments/roles, pointer relations, return
    tokens, order, effects, policy, and target uncertainty.
12. **Execute callees:** when body/capability is available, cost is bounded, and
    caller evidence depends on effects or return behavior.
13. **Opaque callees:** when unavailable, expensive, recursive, intentionally
    hierarchical, or when call behavior itself is the recognition signal.
14. **Recursive recognition:** yes, through bounded bottom-up summaries and SCC
    fixed points; never unbounded recursive execution.
15. **ObservableState:** return/live values, relevant memory reads/writes, flags,
    calls, branches/path classes, environment events, termination, and resources.
16. **API normalization:** stable semantic IDs, aliases/forwarders, role-aware
    arguments, abstract memory/handle tokens, and declared event policies.
17. **Library influence:** identity is a high-value feature but must agree with
    argument relations, effects, surrounding computation, and negative probes.
18. **Database entries:** hybrid contract, executable/reference model, schemas,
    effects, generated observations, confusers, provenance, and version.
19. **Minimum metadata:** stable identity/family, role/effect schema, contract
    level, reference or summary, and a reproducible baseline/discriminator suite.
20. **Automatic generation:** largely yes for deterministic references and common
    memory families; API/environment/undefined behavior fields need review.
21. **Probe strategy:** typed structured baseline plus alias/boundary/metamorphic
    probes, followed by cost-aware adaptive discrimination.
22. **Ring probes:** useful for bit-vector/scalar arithmetic and hashing, not for
    pointers, memory, environment, or FP; use only as one product-domain lane.
23. **Adaptive probes:** yes, after a baseline; stop on calibrated acceptance or
    irreducible ambiguity.
24. **Candidate-specific probes:** maximize predicted partition/contradiction gain
    while honoring role, alias, safety, and execution cost.
25. **False positives:** negative/confuser corpus, mandatory discriminators,
    counterexample search, held-out confirmation, class thresholds, abstention.
26. **Loops:** bounded concrete paths, induction/repeated-state detection, known
    relational summaries, optional symbolic invariants, explicit timeout status.
27. **Floating point:** preserve IEEE bit behavior where supported, declare ULP/
    exact contract, probe NaN/signed-zero/exceptions, and abstain on unsupported FP.
28. **Environment calls:** abstract event traces and effects; do not depend on the
    host filesystem, network, time, scheduler, or handles.
29. **Compiler optimizations:** match contracts and dependence/effect relations,
    allow vector/scalar variants, and retain multiple region boundaries.
30. **Partial inlining:** represent proper semantic subregions and larger composite
    regions; use boundary context rather than demanding source recovery.
31. **Merged computations:** use dependence closures and overlapping hypotheses;
    recognize a semantic contract only when shared context is modeled.
32. **Candidate explosion:** seeded generation, caps, beam/subsumption, family
    index, cache, and lazy expansion.
33. **Execution caching:** key normalized region, probe, policy, context, emulator,
    and database versions; snapshot common prefixes.
34. **Uncertainty:** weighted explicit region/interface/role/alias/policy states;
    merge equivalent interfaces and record pruning.
35. **Abstention:** unsupported semantics, unresolved mandatory effects, weak margin,
    inconclusive probes, resource exhaustion, or finite-observation ambiguity.
36. **Reusable solutions:** Ghidra P-Code, SSA/MemorySSA concepts, BSim/graph
    retrieval, SMT bit-vectors, property-based testing, concolic execution, and
    API summaries, subject to measured fit.
37. **Impossible:** universal semantic equivalence, source-boundary recovery, and
    guaranteed finite-probe identification.
38. **Likely strong on GTA5:** imports/wrappers, common memory/string idioms,
    bounded scalar utilities, and whole ordinary functions.
39. **Likely weak on GTA5:** opaque indirect dispatch, global/concurrent engine
    logic, unsupported SIMD/FP, exception paths, and merged partial inlines.
40. **Failure minimization:** capability-aware thresholds, conservative unknown
    channels, bottom-up summaries, adversarial evaluation, and per-family metrics.

## 26. Experimental Roadmap

Every experiment must record hypothesis, setup, corpus, methodology, metrics,
expected outcome, failure criterion, and follow-up decision. Results must include
seeds, configuration, tool versions, and artifact hashes.

### E0: Reproducible baseline

- **Hypothesis:** the current concrete emulator and PyGhidra adapter are a stable
  execution foundation.
- **Setup:** run the existing ten-function corpus and unit/integration tests.
- **Metrics:** pass rate, unsupported opcode count, deterministic trace hash.
- **Failure:** any nondeterminism or unexplained mismatch.
- **Decision:** fix semantic adapter issues before analysis experiments.

### E1: IR and CFG fidelity

- **Hypothesis:** normalized P-Code plus CFG reproduces operation and target order.
- **Setup:** compare extracted blocks, successors, and returns with Ghidra listing
  on corpus functions and hand-written branch/jump-table fixtures.
- **Metrics:** block/edge precision and recall, unresolved-target rate.
- **Falsification:** missing reachable edge or incorrect relative target.
- **Follow-up:** preserve frame boundaries or add explicit unknown edges.

### E2: SSA and memory dependence

- **Hypothesis:** SSA/MemorySSA-like views identify external values and relevant
  memory dependencies without unacceptable over-approximation.
- **Setup:** scalar, pointer, overlapping-buffer, global, and branch-join fixtures.
- **Metrics:** def-use correctness, boundary-input precision/recall, memory-edge
  count, unknown-partition rate.
- **Falsification:** a true dependency is omitted or every memory access aliases.
- **Follow-up:** adjust partitions or use region-level unknown effects.

### E3: Candidate-region algorithms

- **Hypothesis:** seeded SESE plus dependence closure yields much fewer candidates
  than intervals while preserving labeled semantic regions.
- **Setup:** compile standalone and inline variants at multiple optimization levels.
- **Metrics:** candidate count, true-region recall, average/max region size,
  generation time, boundary quality.
- **Falsification:** interval baseline has materially better recall at acceptable
  cost, or closure misses split regions.
- **Follow-up:** enable frontier beam or add seed classes; do not silently expand
  to all intervals.

### E4: Role and interface inference

- **Hypothesis:** role hypotheses improve compatible-family retrieval without exact
  type recovery.
- **Setup:** strings, bounded strings, buffers, scalar arithmetic, handles, and
  intentionally ambiguous pointer/integer fixtures.
- **Metrics:** top-k role accuracy, interface recall, false hard exclusions.
- **Falsification:** role filter removes ground truth more often than it reduces
  candidates.
- **Follow-up:** make the role filter soft and widen hypotheses.

### E5: Observable-state normalization

- **Hypothesis:** normalized semantic channels remain stable across register
  allocation, addresses, and scalar/vector implementation variants.
- **Setup:** same source under MSVC/Clang, optimization levels, and memory layouts.
- **Metrics:** within-contract variation, cross-contract separation, bytes/events
  retained, normalization collision rate.
- **Falsification:** address/register artifacts dominate or relevant effects vanish.
- **Follow-up:** refine object/effect model and strictness levels.

### E6: Probe families

- **Hypothesis:** typed structured probes outperform fixed raw vectors at similar
  execution counts.
- **Setup:** `strlen`, `strnlen`, `memcpy`, `memmove`, `strcmp`, `memset`, math,
  and confusers.
- **Metrics:** true-candidate survival, confuser rejection, probes per decision,
  memory safety, coverage.
- **Falsification:** structured probes do not beat random/ring baselines.
- **Follow-up:** keep only the empirically useful lanes.

### E7: Adaptive discrimination

- **Hypothesis:** information/cost selection reduces probes while preserving
  precision relative to fixed suites.
- **Setup:** candidate sets with deliberate near-neighbors and hidden variants.
- **Metrics:** executions, wall time, candidate entropy/reduction, precision,
  abstention, worst-case probes.
- **Falsification:** adaptive policy selects redundant or unsafe probes.
- **Follow-up:** add cost/safety constraints or use a deterministic decision tree.

### E8: Callee policies

- **Hypothesis:** execute/summary/opaque alternatives improve nested recognition
  without uncontrolled cost.
- **Setup:** nested call chains, recursive SCCs, API wrappers, unknown callees.
- **Metrics:** precision/recall by policy, call depth, summary convergence, time.
- **Falsification:** opaque effects cause unsafe labels or recursion dominates.
- **Follow-up:** strengthen unknown-effect barriers and SCC summaries.

### E9: False-positive resistance

- **Hypothesis:** active counterexample search materially reduces finite-probe
  collisions.
- **Setup:** fake `strlen`, bounded copy, alternate compare, polynomial confusers,
  wrappers with same APIs.
- **Metrics:** false-positive rate at fixed recall/abstention, witness size/cost,
  time to rejection.
- **Falsification:** leaders pass all budgeted adversarial probes while known
  contracts differ on an available test.
- **Follow-up:** expand contract relations, use symbolic witness generation, or
  lower confidence and abstain.

### E10: Loops and symbolic assistance

- **Hypothesis:** bounded concrete plus targeted symbolic constraints improves
  path coverage without solver cost dominating.
- **Setup:** scans, copies, compares, parsers, nested loops, nontermination traps.
- **Metrics:** path/branch coverage, classification, timeout/inconclusive rate,
  solver calls/timeouts.
- **Falsification:** symbolic overhead exceeds its rejection/coverage benefit.
- **Follow-up:** retain concrete summaries and postpone symbolic use.

### E11: Database generation

- **Hypothesis:** reference-driven generation can create useful entries with small
  manual effort.
- **Setup:** 50-100 standard library/CRT/Windows-like contracts and variants.
- **Metrics:** authoring minutes per entry, contract coverage, cross-compiler
  recognition, generated false positives, manual corrections.
- **Falsification:** generated entries overfit probes or require excessive repair.
- **Follow-up:** add family templates and explicit review fields.

### E12: GTA5-scale sample

- **Hypothesis:** the cascade reaches expensive execution only for a small fraction
  of representative GTA5 functions while retaining high precision.
- **Setup:** stratified labeled sample: imports, leaf loops, wrappers, SIMD/FP,
  indirect calls, templates, and large functions.
- **Metrics:** per-family precision/recall/abstention, candidate reduction,
  unsupported rate, wall time, memory, executions, inline depth.
- **Falsification:** candidate explosion, unacceptable unsupported coverage, or
  false positives in high-value families.
- **Follow-up:** narrow the declared capability envelope rather than force labels.

## 27. Benchmark Corpus

Create a versioned corpus with source-level ground truth and extracted P-Code,
while evaluating recognition without exposing names/labels to the recognizer.

### Positive families

- scalar arithmetic, bit operations, min/max/clamp, reductions;
- `strlen`, `strnlen`, `strcmp`, `strncmp`, `memchr`;
- `memcpy`, `memmove`, `memset`, overlap variants;
- allocation/free and handle-like wrappers;
- `sin`, `cos`, `tan`, `sqrt`, polynomial approximations;
- parser/search/sort/array utilities;
- Windows file, memory, thread, and socket API wrappers;
- ordinary internal callees and nested call graphs.

### Transformation matrix

- MSVC and Clang where available;
- no optimization, `-O1`/`-O2`/`-O3`, LTO where practical;
- forced inline, no-inline, partial inline, cloning, vectorization;
- scalar/SIMD and wrapper variants;
- x64 ABI variants and different layout/constant choices.

### Negative/confuser corpus

Include custom implementations that agree on short probes, bounded scans,
constant-return wrappers, same-API wrappers with different arguments/effects,
overlap-incorrect copies, approximate math, dead-code lookalikes, opaque global
state, and intentionally malformed/unsupported P-Code.

Split by source family and compiler, not by random rows, to avoid leakage. Keep a
held-out transformation/compiler split for generalization.

## 28. Metrics and Acceptance Targets

Report all of the following overall and by family/optimization/inline depth:

- precision among accepted labels;
- recall among eligible/labeled regions;
- false-positive rate per candidate and per accepted result;
- false-negative rate;
- abstention and inconclusive rate;
- coverage of supported operations/effects;
- candidate reduction ratio and region recall;
- behavioral executions, probes, wall time, peak memory;
- worst-case execution/path/loop cost;
- API-call and callee-policy ablations;
- calibration error and confidence vs empirical correctness.

Use precision-recall-abstention curves, not one threshold. Suggested initial
research gates are targets rather than guarantees: zero false positives on the
small mandatory confuser suite at a declared abstention level, at least 95%
precision on eligible common leaf/API families, and a measured candidate reduction
of at least an order of magnitude versus interval enumeration. These targets must
be revised after corpus size and difficulty are known. Never trade unexplained
false positives for aggregate recall.

## 29. Falsification Program

The project must actively try to break its own assumptions.

1. Generate pairs of functions with identical outputs on the baseline probes but
   different boundary behavior.
2. Split a known function's computation across blocks and merge it with unrelated
   arithmetic; measure region recall.
3. Replace direct calls with thunks, tail calls, indirect calls, and import
   forwarding; measure identity normalization.
4. Add dead stores, volatile-like unknown effects, and aliasing stores; verify that
   the matcher does not ignore relevant memory.
5. Vary global/TLS state and incoming flags; verify context hypotheses and
   conditional labels.
6. Inject unsupported FP/SIMD/userops and ensure the result is inconclusive, not
   incorrectly accepted.
7. Create nonterminating or huge-loop candidates; verify timeout separation and
   resource caps.
8. Construct wrappers that emit the same API name but differ in argument roles,
   order, or error handling.
9. Compare compiler/optimization variants where implementation shape changes
   drastically but the contract remains the same.
10. Randomly mutate accepted functions and estimate how quickly counterexample
    probes reject them.

Each failure should update either the implementation, the declared capability
envelope, the database contract, or the abstention policy. It must not be hidden
by threshold tuning alone.

## 30. Additional Hidden Constraints

The initial prompt's constraints are not exhaustive. The implementation plan must
explicitly account for:

- x64 ABI shadow space, callee-saved registers, partial-register zero extension,
  structure returns, variadic calls, and stack alignment;
- endian and address-space semantics, segmented/stack/unique spaces, and pointer
  truncation/extension;
- jump tables, tail calls, thunk chains, import forwarding, and indirect calls;
- function pointers, virtual dispatch, RTTI-like code, and unresolved targets;
- vector/SIMD lanes, packed comparisons, FMA, and compiler intrinsics;
- NaN payloads, signed zero, rounding modes, exceptions, and denormals;
- volatile/atomic operations, memory barriers, TLS, locks, and scheduler state;
- SEH, C++ exceptions, stack unwinding, non-local exits, and cleanup paths;
- undefined behavior, sanitizer/security instrumentation, and opaque predicates;
- global constructors, lazy initialization, allocator identity, and process state;
- self-modifying/JIT code or code generated after the P-Code snapshot;
- address randomization and ASLR, which make raw addresses unstable;
- data/model leakage in supervised or embedding experiments;
- P-Code version differences and userop semantics;
- licensing or availability constraints for GTA5 and third-party reference data.

Each unsupported constraint gets a capability flag and a test fixture. The final
report must show how often it caused `UNKNOWN`.

## 31. Python Research Prototype Architecture

Use a package layout that mirrors analysis dependencies and keeps experiments
replaceable:

```text
FUNC_REC_RESEARCH_ASTRA/
    RESEARCH_PLAN.md
    src/
        pcode_model.py       # serializable input and normalized operations
        normalize.py         # P-Code adapter and canonicalization
        cfg.py               # blocks, edges, dominators, loops
        ssa.py               # value SSA and boundary queries
        memory.py            # address expressions, memory versions, aliases
        effects.py           # calls, side effects, capability flags
        roles.py             # weighted type/role hypotheses
        regions.py           # seeds, closures, SESE, beam search
        probes.py            # schemas, memory frames, generators
        emulator_adapter.py  # wrapper around existing concrete executor
        symbolic.py          # optional constraints and witness generation
        observe.py           # raw events and normalized observations
        database.py          # entries, families, indexing, persistence
        matching.py          # cascade, evidence, adaptive selection
        recursion.py         # callee policies and SCC summaries
        report.py             # explainable decisions and artifacts
    database/
        contracts/
        probes/
        observations/
    corpus/
        sources/
        pcode/
        labels/
    experiments/
        e00_baseline.py ...
    tests/
        unit/
        integration/
        adversarial/
    results/
```

The existing `FUNC_RECOGNIZER_RESEARCH/src/pcode_emulator.py` can initially be
imported through a path/configuration adapter. Avoid copying its semantics into a
second emulator. The new layer supplies input frames, observers, policies, and
normalization.

## 32. Detailed Component Implementation Plan

### 32.1 P-Code model and normalizer

- **Purpose:** turn PyGhidra objects or serialized records into stable normalized
  operations.
- **Inputs:** P-Code op records, architecture/endian/address-space metadata,
  optional import/function map.
- **Outputs:** `FunctionIR` operation list and capability diagnostics.
- **Data structures:** immutable `Op`, `VarnodeRef`, `AddressSpace`, `CallTarget`.
- **Algorithm:** validate widths, preserve source identity, canonicalize aliases,
  resolve constants, classify control/call/FP/userop capabilities.
- **Invariants:** no silent opcode loss; source-to-normalized mapping is total for
  every retained op; widths are positive and unchanged.
- **Complexity:** O(number of operations plus metadata).
- **Edge cases:** malformed varnodes, unresolved targets, partial registers,
  unknown userops, address-space IDs.
- **Interactions:** feeds CFG/SSA/emulator and cache key generation.
- **Tests:** golden normalized fixtures and round-trip serialization.
- **Experiment:** E0/E1, including unsupported-operation accounting.

### 32.2 CFG, dominators, loops

- **Purpose:** expose reachability, control dependence, natural loops, and exits.
- **Inputs:** normalized operations and target map.
- **Outputs:** `BasicBlock` graph, dominator/post-dominator trees, SCC/loop data.
- **Algorithm:** split blocks at targets/terminators, add normal/unknown edges,
  run iterative dominator/post-dominator algorithms, identify SCCs and backedges.
- **Invariants:** every reachable successor is represented; unknown edges remain
  explicit; block order is deterministic.
- **Complexity:** near O(V+E) for graph construction; iterative analyses bounded by
  graph size.
- **Edge cases:** irreducible CFGs, jump tables, exception edges, tail calls,
  no-return functions.
- **Interactions:** regions, path probes, recursion, reports.
- **Tests:** diamonds, loops, irreducible/SCC, indirect/unknown target fixtures.
- **Experiment:** E1 and E3.

### 32.3 SSA and memory analysis

- **Purpose:** identify producer/consumer dependencies and region boundaries.
- **Inputs:** CFG and P-Code reads/writes.
- **Outputs:** value IDs/phi nodes, memory accesses/versions, use-def queries.
- **Algorithm:** dominator-frontier SSA; memory partitions by address space/base;
  unknown accesses use conservative may edges; calls consume/produce summary
  tokens.
- **Invariants:** every read has a reaching def or initial/boundary token;
  uncertain memory is never marked must-not-alias.
- **Complexity:** near-linear for values; memory edges capped with conservative
  fallback.
- **Edge cases:** overlapping registers, stores through computed pointers,
  volatile/atomic/unknown calls.
- **Interactions:** roles, regions, observations, summaries.
- **Tests:** pointer alias, global, loop-carried, branch-join fixtures.
- **Experiment:** E2.

### 32.4 Role and interface inference

- **Purpose:** create probeable external-state hypotheses.
- **Inputs:** SSA/memory/effect graph and ABI priors.
- **Outputs:** capped weighted `InterfaceHypothesis` set.
- **Algorithm:** collect operation-use evidence, assign role likelihoods, map
  pointers to abstract objects, enumerate only high-impact alias alternatives.
- **Invariants:** all omitted reads become explicit context inputs or unknown;
  role confidence never hard-excludes without a declared rule.
- **Complexity:** O(values + accesses) plus capped hypothesis branching.
- **Edge cases:** pointer/integer ambiguity, packed structs, multiple values for
  one logical argument, hidden flags.
- **Interactions:** probes, database retrieval, region scoring.
- **Tests:** role fixture labels and false-hard-exclusion checks.
- **Experiment:** E4.

### 32.5 Region discovery

- **Purpose:** generate a compact, conservative candidate pool.
- **Inputs:** CFG/SSA/memory/effect graph and database feature index.
- **Outputs:** `RegionHypothesis` beam including whole function.
- **Algorithm:** seed calls/loops/outputs/idioms, enumerate bounded SESE regions,
  close dependencies, frontier-expand ambiguous candidates, deduplicate and score.
- **Invariants:** whole function retained; each candidate carries omitted context;
  pruning reason and budget are recorded.
- **Complexity:** O(analysis + K * frontier expansions), not O(N^2) intervals.
- **Edge cases:** split/merged inline code, multi-entry slices, nested loops,
  overlapping semantic regions.
- **Interactions:** all later stages.
- **Tests:** labeled inline/partial/fused corpus and interval-count comparison.
- **Experiment:** E3.

### 32.6 Probe planner

- **Purpose:** generate safe, informative concrete inputs.
- **Inputs:** interface hypotheses, database contracts, current candidate evidence.
- **Outputs:** reproducible `Probe` objects and predicted partitions.
- **Algorithm:** baseline schema probes first, then alias/boundary/metamorphic,
  then cost-aware candidate discriminators; include canaries and limits.
- **Invariants:** every pointer is mapped to allocated memory; alias constraints
  are explicit; probe seed and purpose are recorded.
- **Complexity:** generator cost plus O(candidates * probes) prediction; cap pool.
- **Edge cases:** invalid pointers, no-terminator strings, overlap, huge lengths,
  NaN/FP exceptional values, context mutation.
- **Interactions:** emulator, matching, database builder.
- **Tests:** memory safety and expected relation tests.
- **Experiment:** E6/E7.

### 32.7 Emulator and execution cache

- **Purpose:** execute actual P-Code under a probe and call policy.
- **Inputs:** region, initial state, policy, resource limits.
- **Outputs:** raw event trace and status-bearing execution result.
- **Algorithm:** snapshot state, execute through existing emulator adapter, route
  calls by policy, stop at region exits/limits, retain partial trace.
- **Invariants:** timeout is not mismatch; unsupported semantics are explicit;
  cache keys include all semantic configuration.
- **Complexity:** proportional to executed ops/bytes/calls per run.
- **Edge cases:** recursion, nontermination, external events, memory faults,
  branch target ambiguity.
- **Interactions:** observation normalization and adaptive matching.
- **Tests:** deterministic replay, timeout classification, policy differences.
- **Experiment:** E0/E8/E10.

### 32.8 Observation normalization

- **Purpose:** compare semantic behavior independent of raw addresses/registers.
- **Inputs:** raw trace, initial probe frame, region interface, API specs.
- **Outputs:** canonical `Observation` and channel-level diffs.
- **Algorithm:** map objects/tokens, filter dead state, compute memory/effect
  relations, canonicalize calls/branches, retain raw provenance.
- **Invariants:** normalization is deterministic and does not erase declared
  contract-relevant effects.
- **Complexity:** O(trace events + touched memory), with range compression.
- **Edge cases:** overlapping writes, symbolic pointers, allocation reuse,
  partial register writes, unknown globals.
- **Interactions:** database and matcher.
- **Tests:** equivalent variants normalize alike; confusers remain distinct.
- **Experiment:** E5.

### 32.9 Database builder

- **Purpose:** create scalable contract entries from known implementations.
- **Inputs:** reference, role/prototype schema, API docs/metadata, probe policy.
- **Outputs:** versioned `FunctionEntry`, expected observations, discriminators.
- **Algorithm:** generate probes, execute reference, infer relations/effects,
  collect confusers, require review for ambiguous semantics.
- **Invariants:** provenance and environment policy are complete; no probe output
  is treated as a universal law without a contract relation.
- **Complexity:** reference executions dominate; parallelizable by probe.
- **Edge cases:** nondeterministic references, undefined behavior, API host effects,
  compiler-specific outputs.
- **Interactions:** matcher and experiments.
- **Tests:** regenerate entries and compare artifact hashes/relations.
- **Experiment:** E11.

### 32.10 Matcher and evidence report

- **Purpose:** rank, falsify, and make calibrated accept/unknown decisions.
- **Inputs:** candidates, entries, observations, configuration.
- **Outputs:** explainable recognition records.
- **Algorithm:** cascade filters, hard contradictions, adaptive probes,
  counterexample search, margin/threshold gate, calibration.
- **Invariants:** accepted result has mandatory evidence; unknown/inconclusive
  reasons are preserved; no score-only acceptance.
- **Complexity:** cheap index operations plus bounded executions; worst case is
  explicitly budgeted.
- **Edge cases:** tied candidates, unavailable discriminators, class imbalance,
  all candidates inconclusive.
- **Interactions:** report/database/recursive refinement.
- **Tests:** known positives, confusers, forced abstention, calibration fixtures.
- **Experiment:** E7/E9/E12.

## 33. Pseudocode Contracts

```text
build_analysis(function):
    normalized = normalize(function)
    cfg = build_cfg(normalized)
    values = build_ssa(cfg)
    memory = build_memory_versions(cfg, values)
    effects = summarize_effects(normalized, memory)
    return Analysis(normalized, cfg, values, memory, effects)
```

```text
discover_regions(analysis, database, budget):
    result = {whole_function(analysis)}
    seeds = find_seeds(analysis, database)
    for seed in seeds:
        result += bounded_sese(seed, analysis)
        result += dependency_closure(seed, analysis)
    result = frontier_expand_ambiguous(result, analysis, budget)
    return beam_deduplicate(result, budget)
```

```text
infer_external_inputs(region, analysis):
    values = reads_before_def(region)
    memory = reaching_memory_inputs(region)
    flags = incoming_live_flags(region)
    context = globals_tls_calls_and_unknown_edges(region)
    return beam_role_alias_memory_hypotheses(values, memory, flags, context)
```

```text
generate_probe(interface, candidates, evidence):
    if not evidence.has_baseline:
        return next_schema_boundary_probe(interface)
    return argmax(pool(interface, candidates),
                  expected_partition_reduction
                  + contradiction_gain
                  + coverage_gain
                  - execution_cost
                  - safety_risk)
```

```text
execute(region, probe, policy, limits):
    state = snapshot_initial_state(probe)
    trace = emulator.run(region, state, policy, limits)
    return normalize_state(trace, probe, region.interface)
```

```text
compare(entry, observation):
    if hard_effect_contradiction(entry, observation):
        return CONTRADICTED
    return channel_evidence(entry.contract, observation)
```

```text
select_next_probe(evidence):
    if mandatory_probes_remain(evidence):
        return next_mandatory_probe(evidence)
    return cost_aware_max_discriminator(evidence)
```

## 34. Implementation Order and Scope Control

### Build first

1. Serializable P-Code model and normalizer.
2. CFG and operation-level trace mapping.
3. SSA/value boundaries and conservative memory accesses.
4. Whole-function plus SESE/dependence region candidates.
5. Structured memory-frame probes and normalized observations.
6. Five deterministic contract families: `strlen`, `memcpy`, `memmove`,
   `strcmp`, and arithmetic.
7. Cascaded matcher, evidence records, counterexample probe, and `UNKNOWN`.
8. Unit/integration/adversarial experiments E0-E9.

### Postpone until the baseline is measured

- learned ranking or embeddings in the final decision;
- complete source type recovery;
- whole-program symbolic execution;
- exact floating-point library identity;
- concurrency and real OS emulation;
- exhaustive indirect-call target recovery;
- production-scale parallel/distributed infrastructure.

### What may be mocked

- external API results, using event contracts and symbolic handles;
- unresolved callee bodies, using opaque summaries;
- optional solver, using concrete boundary probes;
- GTA5 labels during early unit experiments, using generated source ground truth.

### What must be real

- P-Code parsing/width/control semantics;
- memory alias and canary behavior in probe execution;
- region boundary/interface accounting;
- normalized observation comparison;
- negative/confuser testing;
- resource limits and explicit abstention.

## 35. Acceptance Criteria

The research prototype is ready for a GTA5-oriented pilot when:

1. It consumes serialized low P-Code without requiring ProgramDB at recognition
   time.
2. It reproduces the existing concrete emulator baseline and reports unsupported
   operations instead of silently continuing.
3. It creates CFG, SSA, conservative memory/effect views with tested invariants.
4. It includes the whole function and a bounded, measured region candidate set.
5. It infers explicit interface, role, alias, context, and callee-policy
   hypotheses.
6. It records normalized returns, memory effects, calls, path/termination, and
   inconclusive statuses.
7. It recognizes ordinary, inline, partially inline, wrapper, and nested fixtures
   where the declared capability envelope supports them.
8. It rejects the mandatory finite-probe confusers through an actual discriminator
   or abstains when no discriminator is available.
9. It supports execute, summary, and opaque callee policies with recursion limits.
10. It reports precision, recall, abstention, candidate reduction, probes,
    executions, and per-family/resource metrics.
11. Every accepted decision contains evidence and every rejection/abstention has a
    reason.
12. Falsification experiments are run and their outcomes affect the declared
    architecture or capability envelope.

The first successful milestone is not "recognizes every GTA5 function." It is a
reproducible demonstration that semantic-region hypotheses plus contract-directed
behavioral evidence achieve substantially lower false-positive rates than raw
fingerprints or structural similarity alone on a held-out transformed corpus.

## 36. Future Extensions

After the baseline is empirically justified, investigate:

- solver-generated relational summaries and loop invariants;
- graph neural or learned candidate ranking with calibration and adversarial
  auditing;
- probabilistic API/callee identity resolution;
- richer SIMD and floating-point abstract domains;
- interprocedural context summaries and global-state versioning;
- concurrency/event partial orders;
- scalable database indexing over semantic contracts;
- active database maintenance from newly discovered confusers;
- cross-architecture normalization and ABI-specific adapters;
- human-in-the-loop review queues for high-value `REVIEW` results;
- production implementation only after the Python experiments establish which
  analyses and policies actually improve precision/coverage.

## 37. Final Research Position

The initial behavioral-identification idea is valuable but too weak in its raw
form. A finite vector of outputs cannot solve unknown interfaces, aliasing,
context, calls, loops, or semantic collisions. The strongest practical version is
a conservative hybrid in which:

- P-Code remains the semantic execution source;
- analysis IR makes data/control/memory dependencies explicit;
- a region is a bounded semantic subgraph with an inferred external-state
  boundary, not an arbitrary interval;
- calls and effects are first-class observations;
- database entries combine contracts, executable references, probes, and
  implementation variants;
- probes are typed, memory-aware, alias-aware, metamorphic, and adaptive;
- concrete execution falsifies static hypotheses, with selective symbolic help;
- recursive callee summaries are policy-controlled and budgeted;
- uncertainty is preserved rather than prematurely collapsed;
- high precision is protected by counterexamples, calibration, and abstention.

This architecture directly addresses ordinary functions and inline regions with one
unified model. It also states where recognition is impossible or unsafe. The
prototype should therefore optimize first for measured scientific truth: expose
every assumption, attack every major hypothesis, and allow `UNKNOWN` whenever the
available P-Code, context, database, or execution budget does not justify a label.
