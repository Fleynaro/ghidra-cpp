You are an architecture/research agent for our C++23 Ghidra reimplementation project.

Your task is **ONLY to study the existing project and design the final architecture**.
**Do NOT modify, create, delete, rename, move, or refactor any source files. Do NOT implement anything.**

Your ONLY deliverable is:

```text
{workdir}/NEW/ARCHITECTURE.md
```

The document must be extremely detailed and implementation-oriented. A relatively inexperienced implementation agent should be able to read `ARCHITECTURE.md` and implement the architecture with minimal additional architectural decisions.

---

# 1. Main goal

We are gradually reimplementing Ghidra functionality in modern C++23.

The final system must:

* preserve the original Ghidra functionality as faithfully as possible;
* be modular and autonomous;
* use explicit contracts between components;
* avoid a giant shared mutable Ghidra-like database as the fundamental architecture;
* support asynchronous execution for expensive operations;
* support synchronous execution for cheap operations;
* use a shared runtime infrastructure;
* support projects, event sourcing, projections and replay;
* eventually support Python, JavaScript and Go bindings;
* use C++ as the primary native API;
* be suitable for a future GUI;
* remain practical for analyzing a huge real binary such as GTA5.exe.

We have already conceptually designed most of the architecture. Your job is to **map that design onto the actual current codebase** and determine the concrete final architecture.

The key question is:

> Does our theoretical architecture actually fit the current NEW implementation, and exactly how should the final architecture look?

Do not blindly preserve the current structure. Analyze it critically and propose corrections where necessary.

---

# 2. Existing conceptual architecture

Use this structure as the starting point:

```text
NEW/
├── core/
│   ├── domain/
│   ├── contracts/
│   └── events/
│
├── services/
│   ├── pe_loader/
│   ├── sleigh/
│   ├── function_id/
│   ├── decompiler/
│   └── analyzers/
│
├── runtime/
│   ├── dispatcher/
│   ├── workers/
│   ├── event_bus/
│   ├── event_store/
│   ├── projections/
│   ├── storage/
│   └── project/
│
├── bindings/
│   ├── cpp/
│   ├── python/
│   ├── javascript/
│   └── go/
│
└── apps/
    ├── gui/
    └── cli/
```

This is the intended direction, but you must verify it against the actual repository.

Do not assume every directory must exist exactly as shown. If inspection proves that something should be moved, merged, split or renamed, explain why.

---

# 3. Read the CURRENT implementation first

Thoroughly inspect:

```text
{workdir}/NEW
```

including all currently implemented modules.

At minimum inspect:

```text
NEW/features/pe_loader
NEW/features/sleigh_runtime
NEW/features/decompiler
NEW/features/function_id
NEW/analyzers
NEW/framework
```

and any other relevant directories.

Also inspect:

* all public headers / `.hpp`;
* all `.cpp`;
* all `.cppm` / C++ modules;
* CMake files;
* tests;
* fixtures;
* existing documentation;
* existing architecture/design documents;
* interfaces;
* duplicated types;
* duplicated utility code;
* dependency relationships.

Do not merely list files.

Understand what the code actually does.

---

# 4. Inspect the original Ghidra sources

For every important component, identify the corresponding original Ghidra implementation.

Especially inspect the original sources related to:

* Sleigh runtime / P-code;
* Decompiler;
* Function ID;
* PE loader;
* analyzers;
* Program / Function / Instruction / Address / Symbol / datatype concepts;
* relevant shared decompiler/Sleigh data structures.

We specifically suspect that our current Sleigh decoder and Decompiler were extracted from the same original Ghidra C++ subsystem and therefore currently contain duplicated concepts/types.

Investigate this carefully.

For every important proposed domain type, provide the original Ghidra source reference, for example:

```text
Ghidra/Framework/...
Ghidra/Features/Decompiler/...
Ghidra/Processors/...
```

Use exact source paths whenever possible.

The final architecture must preserve these references so future implementers can understand the origin of each abstraction.

---

# 5. CORE / DOMAIN must be designed explicitly

One of the most important tasks is designing:

```text
NEW/core/domain/
```

Do not just say "Address, Function, Instruction, etc."

Determine the actual domain model required by the current Sleigh, Decompiler, Function ID and future analysis pipeline.

For each proposed domain type specify:

* exact filename;
* preferably exact `.cppm` module name;
* class/struct name;
* purpose;
* important fields;
* ownership semantics;
* value/reference semantics;
* immutability/mutability;
* dependencies;
* which services use it;
* why it belongs in `core/domain`;
* original Ghidra source reference;
* whether it should be a value type or interface;
* whether it should be serializable;
* whether it should appear in public API.

Examples to investigate:

```text
Address
AddressRange
AddressSpace
Bytes
Instruction
InstructionOperand
PCodeOp
Varnode
Register
Scalar
Function
FunctionId
Symbol
DataType
MemoryRegion / MemoryBlock
Reference
CallingConvention
Architecture
CompilerSpec
Language
...
```

Do NOT automatically create all of these.

Determine which concepts are actually required.

Very important:

> Avoid creating a second giant Ghidra ProgramDB inside `core`.

`core/domain` should contain stable domain primitives and concepts shared across services, not business logic belonging to a particular service.

---

# 6. Identify and eliminate duplicated domain concepts

Explicitly search for duplicated types between:

```text
sleigh_runtime
decompiler
function_id
pe_loader
analyzers
framework
```

For example, if multiple modules independently define:

```text
Address
Instruction
Register
PCode
Varnode
Architecture
Memory
Function
```

analyze whether these should become shared `core/domain` concepts.

Create a section:

```text
## Current Duplication and Consolidation Plan
```

with a table:

| Current location | Duplicated concept | Proposed canonical type | Why |
| ---------------- | ------------------ | ----------------------- | --- |

Do not implement the consolidation. Only design it.

---

# 7. CORE / CONTRACTS

Design the complete service/provider contract layer.

This is extremely important.

Do not merely write:

```text
IDecompiler
ISleigh
IPELoader
```

Instead define the actual intended interfaces/classes and their responsibilities.

For every contract specify:

* exact class/interface name;
* method signatures;
* input types;
* output types;
* synchronous/asynchronous semantics;
* error model;
* ownership;
* thread-safety;
* whether it is a Service or Provider;
* dependencies;
* original Ghidra references where applicable.

Examples to investigate:

```cpp
class IPCodeDecoder;
class IDecompiler;
class IPELoader;
class IFunctionIdDatabase;
class IFunctionAnalyzer;
class IMemoryProvider;
class IFunctionProvider;
class IInstructionProvider;
class IEventStore;
...
```

But do not blindly create these names. Derive the final set from the actual architecture.

Clearly distinguish:

### Service

An active capability / operation.

### Provider

A passive dependency supplying data/capabilities to another service.

Avoid interface explosion. Do not create five interfaces representing the same conceptual capability without a concrete architectural reason.

---

# 8. SYNC vs ASYNC API

This distinction must be explicit.

For example:

### Sleigh

Small operation:

```cpp
Instruction decode(BytesView bytes);
PCode translate(const Instruction&);
```

can be synchronous.

But large-scale decoding may need asynchronous execution.

Therefore investigate a design such as:

```cpp
ISleighDecoder
    ├── synchronous API
    └── asynchronous/batched API
```

or another architecture if your analysis finds something better.

We want the ability to do both:

```text
decode one instruction
    -> synchronous

decode thousands/millions of instructions
    -> asynchronous worker pool
```

### Decompiler

Decompiler should primarily be treated as an expensive asynchronous operation:

```text
CommandRequest
    DecompileFunction
          ↓
Dispatcher
          ↓
Shared Worker Pool
          ↓
Decompiler
          ↓
CommandResponse / Task<Decompilation>
```

Design the exact API.

Show how C++23 coroutines / `Task<T>` or another abstraction should be used conceptually.

Do not implement it.

---

# 9. SHARED WORKER POOL

We specifically want the worker pool to be a reusable runtime infrastructure component.

Do NOT create an independent worker pool for every service unless there is a demonstrated reason.

The intended concept is:

```text
                    Runtime Worker Pool
                           │
             ┌─────────────┼─────────────┐
             ▼             ▼             ▼
        Decompiler    Function ID    Bulk Sleigh
```

Investigate whether this should be:

```text
runtime/workers/
```

and how services submit work to it.

Explain:

* worker abstraction;
* task abstraction;
* queue design;
* thread independence;
* scheduling;
* cancellation;
* priorities, if useful;
* CPU-bound workloads;
* maximum concurrency;
* interaction with coroutines;
* whether there should be one global pool per runtime/project or multiple pools;
* how expensive services avoid starving each other.

Do not overengineer. We want a practical C++23 architecture.

---

# 10. DISPATCHER

Design:

```text
runtime/dispatcher/
```

Explain how:

```text
CommandRequest
      ↓
Dispatcher
      ↓
Service
      ↓
CommandResponse
```

works.

Specify the command abstraction.

For example:

```text
CommandRequest
CommandResponse
Command
CommandContext
CommandId
CorrelationId
```

if actually needed.

Explain synchronous vs asynchronous command execution.

The dispatcher must not become business logic.

---

# 11. EVENTS

We have established the following semantic rule:

> Command = request to perform an operation.
>
> CommandResponse = transient result of that operation.
>
> Domain Event = significant persistent fact/change in system history.

Use this distinction throughout the architecture.

Examples:

```text
BinaryLoaded
SectionMapped
FunctionDiscovered
FunctionIdMatched
FunctionRenamed
FunctionTypeChanged
AnalysisCompleted
HypothesisCreated
...
```

Do NOT event-source every tiny operation.

For example, individual P-code operations should NOT automatically become persistent domain events.

Design:

```text
core/events/
runtime/event_bus/
runtime/event_store/
```

separately.

Explain exactly what each layer owns.

---

# 12. EVENT STORE

Design the initial event store.

We currently prefer a simple append-only sequential log:

```text
project/
    events.log
```

because:

* append is cheap;
* sequential reads are fast;
* replay is simple;
* event history is naturally ordered.

Design the event record format conceptually.

Investigate whether events need:

```text
event_id
event_type
project_id
entity/aggregate id
sequence number
timestamp
payload
schema version
correlation id
causation id
source service
```

Do not blindly include everything. Explain what is actually necessary.

Explain replay.

Example:

```text
events.log
    ↓
Event Replay
    ↓
Projection
    ↓
Current Project State
```

Also explain how the event store differs from projection storage.

---

# 13. PROJECTIONS

For now, DO NOT design the RDF/hypothesis system as the primary architecture.

We want a conventional projection first.

For example:

```text
Event Store
     ↓
Projection
     ↓
Current Software Model
```

Design what the initial projection should contain:

```text
Functions
Instructions
Symbols
References
Memory regions
Data types
...
```

Explain where it is stored.

It can use different physical storage from the event log.

For example:

```text
project/
    events.log
    projection/
        ...
```

Determine whether SQLite, custom files, or another storage format makes the most sense architecturally, but do not implement it.

Important:

The projection should provide the familiar Ghidra-like model needed by users and analyzers, but it must NOT become a giant mutable central dependency like Ghidra's ProgramDB.

The projection is a materialized current view of the event history.

---

# 14. PROJECT SYSTEM

The current repository does not properly have the Project abstraction yet.

Design it.

We currently believe:

```text
Project
```

belongs primarily to `runtime`, not `core/domain`.

Evaluate this assumption critically.

A project represents one analyzed executable/binary:

```text
Project GTA5
    ├── input artifact(s)
    ├── configuration
    ├── event store
    ├── projections
    └── runtime context
```

For now assume:

> one Project = one primary `.exe`

unless analysis reveals a strong reason otherwise.

Define:

* `Project`
* project lifecycle;
* project creation/open/close;
* project directory;
* project configuration;
* event store ownership;
* projection ownership;
* service lifecycle;
* loaded services;
* caches;
* resource ownership.

Explain what belongs in `runtime/project` versus `core`.

---

# 15. SERVICE INITIALIZATION / PRELOADING

Investigate how the runtime should preload expensive resources.

Example:

Function ID may need:

```text
.fidb
.fidb
.fidb
...
```

Opening and indexing many FID databases may be expensive.

The runtime should potentially preload them when a Project is initialized.

Likewise:

* Sleigh `.sla` files;
* compiler specifications;
* architecture descriptions;
* Function ID databases;
* other expensive immutable resources.

Design a lifecycle such as:

```text
Project Open
    ↓
Runtime initialization
    ↓
Load required resources
    ↓
Create service instances
    ↓
Warm caches
    ↓
Project Ready
```

But distinguish:

* mandatory initialization;
* lazy loading;
* preloading;
* caching;
* eviction.

For Function ID specifically, investigate whether searching multiple databases should use the shared worker pool.

Example:

```text
Function
   │
   ├── FID DB #1 ── Worker
   ├── FID DB #2 ── Worker
   ├── FID DB #3 ── Worker
   └── FID DB #4 ── Worker
```

Then combine results.

Do not assume this exact scheduling is optimal; analyze it.

---

# 16. ANALYZERS

Very carefully inspect:

```text
NEW/analyzers
```

There is already an analyzer manager/orchestration mechanism.

Determine:

* what it currently does;
* whether it belongs in `services/analyzers`;
* whether its orchestration responsibilities should move into `runtime`;
* what should remain as an actual analyzer service;
* how analyzers depend on other services;
* how analyzers are ordered;
* whether dependencies form a DAG;
* how asynchronous operations work;
* how analysis progress is represented;
* how analyzers emit events;
* how they query projections.

We currently suspect that:

```text
Analyzer implementation
    -> services/analyzers

Analyzer scheduling/orchestration
    -> runtime
```

But this is only a hypothesis. Validate it against the current code.

Design the final separation.

---

# 17. REALISTIC END-TO-END PIPELINE

Include a detailed end-to-end architecture example for this exact workflow:

1. User opens `GTA5.exe`.
2. Runtime creates a Project.
3. PE Loader loads the image.
4. Events are emitted.
5. Projection is updated.
6. Runtime initializes required architecture resources.
7. Sleigh resources are loaded.
8. Function ID databases are loaded/warmed.
9. Automatic analysis starts.
10. Function ID searches functions.
11. Function discovery creates new functions.
12. Reference analysis discovers relationships.
13. Analyzer requests decompilation.
14. Decompiler requests P-code.
15. Expensive decompilations execute through the shared worker pool.
16. Cheap Sleigh operations can execute synchronously.
17. Significant changes generate domain events.
18. Projection updates.
19. User browses functions.
20. User renames a function.
21. User assigns a data type.
22. User creates a new function in an unexplored region.
23. Those changes generate events and update the projection.
24. Project can be closed.
25. Project can later be reopened.
26. Projection can be loaded directly.
27. If necessary, projection can be reconstructed by replaying the event store.

Show this as one or more Mermaid diagrams.

---

# 18. PUBLIC C++ API AND LANGUAGE BINDINGS

We want one stable native C++ API.

Conceptually:

```text
                Native C++ API
                      │
            ┌─────────┼─────────┐
            ▼         ▼         ▼
         Python      JS        Go
```

The C++ API is special because it is both:

1. the public native API;
2. the API used internally by the runtime/services where appropriate.

However, avoid creating a dependency cycle where:

```text
runtime -> bindings/cpp -> runtime
```

Instead determine the correct layering.

Clearly distinguish:

```text
core contracts
internal runtime/services
public C++ facade
language bindings
```

Design how Python/JS/Go can eventually be generated or wrapped around the C++ API.

---

# 19. DEPENDENCY GRAPH

Create a clear dependency graph.

For example:

```text
                 core/domain
                      ↑
                 core/contracts
                      ↑
          ┌───────────┼───────────┐
          │           │           │
       services    runtime     events
          │           │           │
          └───────────┼───────────┘
                      ↓
                 C++ Public API
                      ↓
              Python / JS / Go
```

But derive the actual final graph from your investigation.

Explicitly identify forbidden dependencies.

For example:

```text
core MUST NOT depend on services
core MUST NOT depend on runtime
domain MUST NOT depend on GUI
services MUST NOT depend on concrete projection storage
```

etc.

---

# 20. FINAL FOLDER TREE

Provide the complete proposed final directory tree.

Not just:

```text
core/
services/
runtime/
```

but down to the meaningful source/module level.

For example:

```text
NEW/
├── core/
│   ├── domain/
│   │   ├── address.cppm
│   │   ├── instruction.cppm
│   │   ├── pcode.cppm
│   │   └── ...
│   ├── contracts/
│   │   ├── pcode_decoder.cppm
│   │   ├── decompiler.cppm
│   │   └── ...
│   └── events/
│       ├── binary_events.cppm
│       └── function_events.cppm
│
├── services/
...
```

The exact structure must be based on your investigation.

For every important file explain its purpose.

---

# 21. CLASS / MODULE CATALOG

Create a detailed catalog of the final architecture.

For each class/interface/module:

```text
Name
Location
Responsibility
Public API
Dependencies
Thread safety
Sync/Async
Persistence
Original Ghidra source
Current NEW implementation(s)
Migration notes
```

This is one of the most important parts of the document.

The eventual implementer should be able to use this as a blueprint.

---

# 22. Ghidra → NEW MAPPING

Create a mapping table:

| Ghidra original | Current NEW | Final NEW | Notes |
| --------------- | ----------- | --------- | ----- |

Cover the important classes and modules.

Explicitly identify:

* code that should move;
* code that should be merged;
* code that should become domain types;
* code that should become contracts;
* code that should remain service-specific;
* code that should move into runtime;
* duplicated code that should disappear conceptually.

Again: DO NOT perform any changes.

---

# 23. CONCURRENCY MODEL

Design the concurrency model explicitly.

Explain:

* which operations are synchronous;
* which operations are asynchronous;
* what uses the worker pool;
* what must remain thread-safe;
* what data is immutable;
* what data is project-scoped;
* how multiple concurrent commands interact;
* how events are ordered;
* how projection updates are serialized/concurrent;
* whether services are stateless;
* how cancellation should work.

Do not create an unnecessarily complicated distributed-systems architecture.

This is an in-process C++23 application inspired by service architecture, not actual network microservices.

---

# 24. IMPORTANT ARCHITECTURAL PRINCIPLE

The architecture should feel like an in-process service platform:

```text
Commands
Services
Providers
Runtime
Worker Pool
Events
Event Store
Projections
Public API
```

but NOT actual HTTP microservices.

There should be:

* no unnecessary serialization;
* no localhost RPC;
* no network overhead;
* no artificial process boundaries.

The service abstraction is primarily for modularity, contracts and lifecycle.

---

# 25. FUTURE RDF / FACT / HYPOTHESIS LAYER

Do not implement this.

Only explain how the architecture leaves room for it.

Eventually we want:

```text
Event Store
      │
      ├── Normal Projection
      │
      └── Knowledge Projection
              ├── Facts
              ├── Evidence
              ├── Hypotheses
              └── Provenance
```

The current conventional projection should therefore not make the future RDF/fact model impossible.

---

# 26. DIAGRAMS

Make the Markdown visually useful.

Use Mermaid diagrams for at least:

1. Final architecture.
2. Folder/dependency architecture.
3. Open Project lifecycle.
4. Binary loading pipeline.
5. Automatic analysis pipeline.
6. Command → Dispatcher → Worker Pool → Response.
7. Event → Event Store → Projection.
8. Sync Sleigh path.
9. Async Decompiler path.
10. Shared Worker Pool.
11. Public C++ API → Python/JS/Go.
12. Project lifecycle.
13. Replay process.

Make diagrams readable and not absurdly gigantic.

---

# 27. DECISION LOG

At the end include:

```text
## Architectural Decisions
```

For each important decision:

```text
Decision
Reason
Alternatives considered
Why rejected
Consequences
```

Especially document decisions around:

* `core` vs `runtime`;
* Service vs Provider;
* sync vs async;
* shared worker pool;
* Command vs Event;
* Event Store vs Projection;
* Project location;
* Analyzer Manager location;
* C++ public API;
* duplicated Sleigh/Decompiler domain types;
* storage.

---

# 28. OPEN QUESTIONS / RISKS

Do not pretend everything is certain.

Create:

```text
## Open Questions
## Risks
## Assumptions
```

If the repository reveals conflicts with our theoretical architecture, explicitly document them.

The goal is not to blindly confirm our ideas.

The goal is to determine whether the architecture survives contact with the real codebase.

---

# 29. CRITICAL RULES

1. **DO NOT MODIFY THE CODE.**
2. **DO NOT REFACTOR ANYTHING.**
3. **DO NOT CREATE IMPLEMENTATION FILES.**
4. Only create/update:

   ```text
   NEW/ARCHITECTURE.md
   ```
5. Inspect the actual current implementation deeply.
6. Inspect the original Ghidra sources wherever necessary.
7. Preserve original Ghidra source references.
8. Do not simplify away functionality.
9. Do not design toy abstractions that cannot support the current Decompiler/Sleigh/FunctionID functionality.
10. Do not create a giant `core` framework that becomes another ProgramDB.
11. Do not introduce unnecessary interfaces.
12. Do not turn everything into asynchronous operations.
13. Do not create separate worker pools per service without justification.
14. Do not treat Event Bus as RPC.
15. Do not persist every tiny operation as an event.
16. Do not make the architecture dependent on RDF/hypotheses yet.
17. Do not assume the current folder structure is already correct.
18. The final architecture must be implementable by another agent without repeatedly asking architectural questions.

---

# 30. FINAL QUALITY BAR

Before finishing `ARCHITECTURE.md`, mentally perform this test:

> "Could I give this document to a coding agent that has never seen our architectural discussions and tell it: implement the entire NEW architecture exactly according to this document?"

If the answer is no, the document is not detailed enough.

The document should be a **real architecture specification**, not a high-level overview.

It should explain not only WHAT exists, but:

* WHY it exists;
* WHERE it lives;
* WHAT it depends on;
* WHAT depends on it;
* WHAT its API looks like;
* HOW it behaves synchronously/asynchronously;
* HOW it interacts with runtime;
* HOW it interacts with events;
* HOW it interacts with persistence;
* HOW it maps to the original Ghidra implementation;
* HOW current duplicated code should conceptually be consolidated.

Again: **research and document only. Do not implement the architecture.**
