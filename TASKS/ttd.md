You are working on the C++23 Ghidra reimplementation project under `NEW/`.

## Goal

Implement a complete **TTD Replay Debugger** service and a separate **trace recorder** abstraction, while refactoring the existing debugger contract architecture so that live debugging and replay debugging share a common base contract without duplicating functionality.

This task must be completed end-to-end. **Do not stop after analysis or scaffolding. Implement the contracts, service(s), integration, and comprehensive tests.**

---

# 1. Existing architecture — understand this first

The project uses a service-oriented architecture similar to a backend/web application:

```text
NEW/
├── core/
│   ├── contracts/     # service interfaces / contracts
│   └── domain/        # OS/architecture/debugger-independent domain types
├── services/          # concrete service implementations
└── engine/            # runtime/orchestration
```

Contracts are intentionally independent from concrete implementations, operating systems, CPU architectures, and debugger engines.

There is currently:

```text
NEW/services/debugger/win_dbg_eng/
```

which integrates Microsoft's **DbgEng** for live Windows debugging.

It already has an independent debugger contract:

```text
IDebugSession
```

This contract currently represents a **live debugger session**, including concepts such as launch/attach.

Important: do NOT blindly extend this contract with TTD-specific functionality. We now need to properly separate the common debugger functionality from live/replay-specific functionality.

Study the existing implementation and tests carefully before changing anything.

---

# 2. Desired debugger contract architecture

Refactor the current `IDebugSession` hierarchy into a shared base + specialized contracts.

Conceptually:

```text
              IBaseDebugSession
                    │
          ┌─────────┴─────────┐
          │                   │
   ILiveDebugSession    IReplayDebugSession
          │                   │
       DbgEng                TTD
```

Use the actual naming conventions already present in the project if they are better.

The important architectural requirement is:

### Base contract

Contain only functionality that makes sense for BOTH live and replay debugging.

Examples may include:

* current debug position/state
* process/thread inspection where applicable
* register access
* memory reading
* module/thread information
* breakpoint/watchpoint-like common concepts where appropriate
* other genuinely shared operations

Do NOT force live-only or replay-only concepts into the base.

### Live debugger contract

The live-specific contract should contain functionality such as:

* launch
* attach
* detach
* continue/resume
* live process lifecycle operations
* other functionality that fundamentally requires a live target

The existing DbgEng service should implement this contract.

### Replay debugger contract

The replay-specific contract should contain TTD/replay-specific functionality such as:

* open/load `.run` trace
* close trace
* forward stepping
* backward stepping
* replay positioning / seeking
* trace position
* replay-specific timeline operations
* other capabilities that are fundamentally specific to recorded execution

Do not artificially force replay semantics into the live debugger contract.

### Important

Use **inheritance/composition properly** so common functionality is not duplicated.

Preserve source compatibility where reasonably possible, but prioritize a clean architecture consistent with the existing project.

Before modifying the contracts, inspect all current users of `IDebugSession` and all existing tests.

---

# 3. Separate contract for recording traces

We also need a completely separate, OS/debugger-independent abstraction for **recording execution traces**.

Do NOT put recording into the replay debugger contract.

Conceptually:

```text
ITraceRecorder
    │
    └── WindowsTTDRecorder
```

The contract must be independent of:

* Windows
* `ttd.exe`
* Microsoft TTD
* x86/x64
* ARM
* any particular recorder implementation

It should describe the abstract operation of recording an application execution into a replayable trace.

For example, conceptually it may support:

* start recording an executable
* command line / arguments
* output trace path
* working directory
* environment if appropriate
* recording options
* wait for completion / process result
* stop/cancel if appropriate

Do not blindly copy these names — inspect the existing domain/contract style and design an appropriate abstraction.

The future intention is that another implementation could eventually exist for:

```text
Linux
Android
ARM
another trace technology
another TTD-like recorder
```

Therefore the contract must contain **no TTD-specific concepts**.

Then implement:

```text
Windows TTD recorder
```

which invokes the already-installed:

```text
ttd.exe
```

on the user's Windows machine.

Do not reimplement the recorder. Use the Microsoft command-line recorder.

---

# 4. Research the provided Microsoft TTD sources FIRST

Before implementing the TTD service, thoroughly inspect:

```text
TEST/debugger/TTD
```

This directory contains the Microsoft WinDbg-Samples TTD material, including things such as:

* Replay API
* `LiveRecorderApiSample`
* official TTD examples
* headers/documentation/samples

Treat this as the primary technical reference for the Microsoft Replay API.

Understand:

* how `IReplayEngineView` is created/accessed
* cursor APIs
* replay positions
* forward/backward execution
* register access
* memory access
* modules
* threads
* replay callbacks/events
* watchpoints
* replay segments
* trace lifetime
* error handling
* initialization/shutdown
* required runtime DLLs
* how the standalone/native API is expected to be used

Do not make assumptions based only on high-level documentation.

---

# 5. Also research windbg-tool

Inspect:

```text
TEST/debugger/windbg-tool
```

This is the Devolutions `windbg-tool` project.

Pay particular attention to its native TTD bridge.

Understand how they connect:

```text
C++ native bridge
        ↓
Microsoft.TimeTravelDebugging.Apis
        ↓
TTDReplay.dll / TTDReplayCPU.dll
```

Use this as an additional implementation reference.

However:

**Do not copy their architecture wholesale.**

Our project is C++23 and already has its own:

```text
core/domain
core/contracts
services
engine
```

architecture.

Extract only useful technical information about using Microsoft's TTD Replay API.

---

# 6. Implement the TTD replay service

Create:

```text
NEW/services/debugger/win_ttd/
```

following the project's existing service conventions.

The service should implement the replay-specific debugger contract.

It must use Microsoft's TTD Replay API rather than shelling out to WinDbg for replay operations.

The service should be a real implementation, not a mock.

At minimum, investigate and implement the replay equivalents of the capabilities already tested by the existing debugger contract where they make semantic sense.

Additionally implement replay-specific capabilities such as:

* opening `.run`
* closing `.run`
* current replay position
* forward stepping
* backward stepping
* seeking
* reading registers
* reading memory
* thread enumeration
* module enumeration
* relevant trace/replay metadata

Use existing project domain types wherever possible.

Do not leak Microsoft TTD types through the public contract.

The Microsoft TTD API belongs behind the service boundary.

---

# 7. Trace recorder service

Create an appropriate service for the Windows TTD recorder, for example under the debugger/trace-recording service hierarchy according to the project's existing naming conventions.

It should invoke:

```text
ttd.exe
```

which is already installed on the machine.

Do not assume a hardcoded path if the existing project has a configuration/environment mechanism that is more appropriate.

The service must:

1. start recording a test executable;
2. produce a `.run`;
3. expose useful process/recording result information through the independent recorder contract;
4. report failures clearly;
5. support the options that are actually needed by the tests.

Keep the recorder abstraction independent from Microsoft TTD.

---

# 8. Tests are CRITICAL

This task is NOT complete without extensive tests.

Study:

```text
NEW/services/debugger/win_dbg_eng/tests
```

especially:

```text
NEW/services/debugger/win_dbg_eng/tests/debugger_contract_tests.cppm
```

and the existing generated test executable/source files.

The current DbgEng tests are intentionally non-trivial, including multiple threads and different debugging scenarios.

Use those tests as the baseline for the replay implementation.

You may reuse/copy the existing test executable sources from:

```text
NEW/services/debugger/win_dbg_eng/tests
```

when appropriate.

Modify or extend them if TTD requires additional scenarios.

The test target should be deliberately complex enough to validate real replay behavior:

* multiple threads
* synchronization
* function calls
* stack changes
* register changes
* memory writes
* memory reads
* breakpoints/watchpoints where applicable
* different execution paths
* thread creation/termination
* exceptions if practical
* deterministic observable state changes

Do NOT replace this with a trivial:

```cpp
int main() { return 42; }
```

test.

---

# 9. Recording and replay must be tested together

The integration tests should follow this real workflow:

```text
test executable
      │
      ▼
ITraceRecorder
      │
      ▼
ttd.exe
      │
      ▼
.run
      │
      ▼
IReplayDebugSession
      │
      ▼
Replay / assertions
```

The tests should use the **contracts**, not concrete implementation classes, wherever possible.

Do not make the test directly depend on internal TTD classes.

---

# 10. Do NOT create one giant integration test

Split the test suite into several focused tests that all reuse the same recorded `.run` where possible.

For example:

```text
record test executable
        ↓
shared fixture / generated .run
        │
        ├── open/close tests
        ├── module tests
        ├── thread tests
        ├── register tests
        ├── memory tests
        ├── forward stepping tests
        ├── backward stepping tests
        ├── seek/position tests
        ├── breakpoint/watchpoint tests
        ├── multi-thread replay tests
        └── deterministic replay tests
```

The exact test decomposition should follow the existing test framework and conventions.

Avoid recording a new `.run` for every test unless technically necessary. Recording is expensive.

Use a shared fixture/lifecycle that records once and allows multiple tests to consume the same trace.

Make sure tests are isolated and deterministic despite sharing the trace.

---

# 11. Compare against DbgEng semantics

Where both implementations support the same concept, the tests should ideally use the same contract-level expectations.

For example:

```text
Live:
    ILiveDebugSession

Replay:
    IReplayDebugSession
```

with shared expectations defined against the base contract where appropriate.

This should let us verify that the refactoring did not accidentally break the existing DbgEng implementation.

**Existing DbgEng tests must continue passing.**

Do not sacrifice existing functionality.

---

# 12. Important architectural constraints

### Do NOT:

* leak `DbgEng` types into contracts
* leak Microsoft TTD types into contracts
* make contracts Windows-specific
* make the recorder contract TTD-specific
* make the base debugger contract live-only
* duplicate common debugger methods in both live/replay contracts
* shell out to WinDbg for replay functionality
* replace the real Replay API with a mock implementation
* delete existing DbgEng tests
* weaken existing tests just to make the refactor pass
* remove functionality because it is difficult to port to TTD

### DO:

* preserve the existing architecture
* use C++23 modules consistently with the surrounding codebase
* follow existing naming/style conventions
* keep platform-specific code inside the Windows services
* keep contracts/domain platform-independent
* hide Microsoft APIs behind service boundaries
* add focused tests for every new capability
* keep existing tests passing
* document important TTD-specific limitations where live/replay semantics differ

---

# 13. Pay special attention to semantic differences

Live debugging and replay debugging are NOT identical.

Do not pretend they are.

For every existing debugger operation, explicitly determine:

```text
Supported identically
Supported with different semantics
Replay-specific
Live-only
Not supported
```

For example:

```text
launch       → live-only
attach       → live-only
open trace   → replay-only
step back    → replay-specific
continue     → different semantics
read memory  → potentially common
read regs    → potentially common
threads      → potentially common
modules      → potentially common
```

Reflect this in the contract hierarchy rather than adding meaningless methods.

---

# 14. TTD-specific investigation: instruction-level replay

As part of the implementation/research, determine how the Microsoft Replay API exposes:

* individual execution steps
* `Execute` events
* replay callbacks
* replay segments

We eventually want to build a separate execution profiler capable of deriving:

```text
executed instruction
    ↓
basic block
    ↓
edge
    ↓
function
```

Do NOT implement the full BB profiler in this task unless it naturally fits.

However, the replay service architecture should not make this future functionality impossible.

Document what you discover about the API and the appropriate extension point for future instruction/BB tracing.

---

# 15. Build and test quality

Before declaring the task complete:

1. Build all affected targets.
2. Run the existing DbgEng test suite.
3. Run all new TTD tests.
4. Verify that recording actually invokes the installed `ttd.exe`.
5. Verify that a real `.run` is produced.
6. Verify that the Replay API opens the real `.run`.
7. Verify forward and backward replay against known state transitions.
8. Verify multi-thread behavior.
9. Verify memory/register assertions.
10. Fix all failures rather than documenting them as expected unless they are genuine Microsoft TTD limitations.
11. Do not leave TODO stubs or fake implementations.

If something cannot be implemented because of an actual Replay API limitation, document the exact limitation and keep the contract clean rather than introducing a fake behavior.

---

# 16. Final deliverables

At the end, the repository should contain:

```text
NEW/core/contracts/
    updated debugger contracts
    new trace recorder contract

NEW/core/domain/
    any required platform-independent replay/recording domain types

NEW/services/debugger/win_dbg_eng/
    refactored existing live debugger
    existing tests preserved and passing

NEW/services/debugger/win_ttd/
    complete Microsoft TTD Replay implementation
    tests

appropriate trace-recorder service/
    Windows TTD recorder using ttd.exe
    tests

tests/
    reusable recorded-trace fixture
    comprehensive replay tests
```

Also provide a concise implementation report describing:

* the final contract hierarchy;
* what was moved from the old `IDebugSession`;
* what is shared vs live-specific vs replay-specific;
* the trace recorder abstraction;
* how Microsoft TTD Replay API is integrated;
* how `windbg-tool` informed the implementation;
* test architecture;
* exact capabilities currently supported;
* known TTD limitations;
* recommended future extension point for instruction/BB-level profiling.

## Execution strategy

**Do not immediately start coding.**

First inspect:

```text
NEW/core/contracts
NEW/core/domain
NEW/services/debugger/win_dbg_eng
NEW/services/debugger/win_dbg_eng/tests
TEST/debugger/TTD
TEST/debugger/windbg-tool
```

Build a concrete implementation plan based on the actual repository.

Then implement the plan completely.

Do not ask me to manually perform intermediate implementation steps. You have enough information to investigate the repository and make the necessary architectural decisions yourself.

**The definition of done is a working, tested end-to-end flow:**

```text
C++ test executable
        ↓
independent trace recorder contract
        ↓
Windows TTD recorder
        ↓
ttd.exe
        ↓
real .run
        ↓
independent replay debugger contract
        ↓
Microsoft TTD Replay API
        ↓
real replay operations
        ↓
comprehensive automated assertions
```

Preserve all existing functionality and tests throughout the refactor.
