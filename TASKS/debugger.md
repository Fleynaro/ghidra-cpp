# Task: Implement a Fully Functional Cross-Platform Debugger Contract + Windows DbgEng Service

Implement a complete debugger service in our C++23 architecture.

## Goal

Add a **generic, implementation-independent debugger abstraction** to:

```text
NEW/core/contracts
NEW/core/domain
```

and implement the first concrete backend:

```text
NEW/services/debugger/win_dbg_eng
```

Also add the C++ binding under:

```text
NEW/bindings/cpp
```

Do **not** touch:

```text
NEW/runtime
```

The debugger contracts must be reusable by **any future debugger implementation**, including but not limited to:

* Windows DbgEng
* x64dbg
* GDB
* LLDB
* remote debugging engines
* other local/remote debugger backends

The WinDbg/DbgEng implementation must be only an implementation of those generic contracts.

---

# 1. First: Study the Existing Architecture

Before modifying anything:

1. Thoroughly inspect the current `NEW` architecture.
2. Read:

   * `NEW/ARCHITECTURE.md`
   * existing `core/contracts`
   * existing `core/domain`
   * existing services
   * existing C++23 module conventions
   * existing GoogleTest conventions
   * an existing service that already uses asynchronous `Task`-based APIs
   * especially:

     ```text
     NEW/services/analyzers/subroutine_references/tests/data
     ```

     to understand how test executables and `build.bat` are structured.
3. Inspect the existing `NEW/bindings/cpp` conventions.
4. Preserve the architecture already established by the repository.

Do not invent a parallel architecture if the repository already provides the required patterns.

---

# 2. C++23 Modules Only

This service MUST use C++23 modules.

Do NOT create:

```text
.h
.hpp
.cpp
```

for the new implementation.

Use `.cppm` files consistently, following the conventions already used in `NEW`.

The implementation should use modern C++23 facilities where appropriate:

* modules
* concepts where useful
* `std::expected` if compatible with existing project conventions
* `std::span`
* strong types
* RAII
* `std::jthread`
* C++23 coroutines
* existing project `Task` abstraction

Do not introduce unnecessary dependencies.

---

# 3. Generic Debugger Contracts

Design the debugger contracts FIRST.

They must live under:

```text
NEW/core/contracts
```

and, where appropriate, value/domain types should live under:

```text
NEW/core/domain
```

The contracts must contain **zero knowledge of DbgEng, WinDbg, Windows-specific APIs, x64dbg, GDB, LLDB, etc.**

Absolutely do NOT expose types such as:

```cpp
IDebugClient*
IDebugControl*
DEBUG_VALUE
DEBUG_STACK_FRAME
HRESULT
HANDLE
DWORD
ULONG
```

from generic contracts.

The generic API should express debugger concepts, not a particular engine's API.

---

# 4. Design for Local AND Remote Debugging

Do not accidentally design the contract around:

```text
"debugging a local Windows process"
```

The abstraction must also be capable of representing future remote debugging.

Think in terms of:

```text
DebugSession
Process
Thread
Module
Address
Register
RegisterValue
Memory
StackFrame
Breakpoint
Watchpoint
Exception
ExecutionState
DebugEvent
```

A future implementation might represent:

```text
local Windows process
remote Windows process
Linux process
GDB remote target
LLDB target
x64dbg process
```

without changing the generic contracts.

---

# 5. Asynchronous Execution Is Mandatory

Debugger execution is inherently asynchronous.

For example:

```text
step_over()
    ↓
target executes
    ↓
instruction/function executes
    ↓
debugger receives stop event
    ↓
operation completes
```

Likewise:

```text
continue()
    ↓
wait
    ↓
breakpoint / exception / thread event / process exit
```

Therefore, operations which wait for target execution MUST use the project's C++23 coroutine `Task` abstraction.

Do NOT expose blocking APIs such as:

```cpp
void step_over();
void continue_execution();
```

if the operation semantically waits for execution to stop.

Use the repository's existing `Task<T>` convention, for example conceptually:

```cpp
Task<DebugStopReason> step_over(...);
Task<DebugStopReason> step_into(...);
Task<DebugStopReason> step_out(...);
Task<DebugStopReason> continue_execution(...);
```

Adapt the exact signatures to the existing architecture.

Fast state queries such as reading already-available state may remain synchronous if that matches the project's architecture.

The important distinction is:

```text
command that waits for execution
        -> Task<T>

immediate state/value query
        -> synchronous API where appropriate
```

Do NOT invent a second async abstraction if the project already has `Task`.

---

# 6. Important DbgEng Constraint: Dedicated Debugger Thread

DbgEng has important thread-affinity rules.

Do NOT casually call DbgEng interfaces from arbitrary application threads.

The implementation should have a dedicated debugger/engine thread responsible for interacting with DbgEng.

Conceptually:

```text
Application
    |
    | debugger commands
    v
Command Queue
    |
    v
+-----------------------------+
| DbgEng Debugger Thread      |
|                             |
| IDebugClient                |
| IDebugControl               |
| IDebugRegisters             |
| IDebugDataSpaces            |
| IDebugSymbols               |
| IDebugSystemObjects         |
| callbacks                   |
|                             |
| WaitForEvent()              |
+-----------------------------+
    |
    v
DbgEng / dbgeng.dll
```

External callers must communicate with this engine thread through the service's asynchronous command mechanism.

Do not create a design where random threads directly invoke DbgEng interfaces.

---

# 7. Critical DbgEng Execution Model

DbgEng is event-driven.

Study and correctly implement the relationship between:

```text
SetExecutionStatus(...)
WaitForEvent(...)
IDebugEventCallbacks
```

For example, conceptually:

```text
continue
    ↓
SetExecutionStatus(DEBUG_STATUS_GO)
    ↓
WaitForEvent()
    ↓
target executes
    ↓
breakpoint / exception / step completion
    ↓
DbgEng callback
    ↓
target becomes stopped
    ↓
Task completes
```

Do not assume that calling `SetExecutionStatus()` alone means the asynchronous debugger operation has completed.

Pay particular attention to the documented restrictions around `WaitForEvent()` and callback reentrancy.

---

# 8. DbgEng Implementation

Implement the concrete service under:

```text
NEW/services/debugger/win_dbg_eng
```

The implementation should wrap the native DbgEng API behind the generic debugger contracts.

At minimum investigate and use the appropriate interfaces for:

```text
IDebugClient
IDebugControl
IDebugRegisters
IDebugDataSpaces
IDebugSymbols
IDebugSystemObjects
IDebugEventCallbacks
IDebugOutputCallbacks
IDebugBreakpoint
```

Use newer interface versions where they provide functionality required by the implementation.

Do not blindly use every interface in `dbgeng.h`.

Use the smallest correct set of interfaces needed for the service.

---

# 9. DbgEng Lifetime / RAII

DbgEng uses COM-like reference-counted interfaces.

Use proper RAII wrappers.

Do not scatter manual:

```cpp
AddRef()
Release()
```

throughout the implementation.

Use an appropriate existing project wrapper if one exists, otherwise use a safe RAII approach such as a suitable COM smart pointer.

Every native DbgEng resource must have clearly defined ownership and lifetime.

---

# 10. Generic Debugger Capabilities

The generic debugger API and WinDbg/DbgEng implementation should support, where the underlying engine permits it:

### Session

* create session
* launch process
* attach to process
* detach
* terminate

### Execution

* continue
* pause/break
* step into
* step over
* step out

### Processes

* process identity
* process state
* exit state/code

### Threads

* enumerate threads
* current thread
* select thread
* thread state
* thread identity

### Registers

* enumerate/read registers
* obtain instruction pointer
* obtain stack pointer
* obtain frame/base pointer where available
* architecture-independent register representation

For x64 specifically, verify access to:

```text
RAX-R15
RIP
RSP
RBP
RFLAGS
```

and investigate SIMD/FPU register support for future extensibility.

### Memory

* read memory
* write memory
* inspect memory regions where appropriate

### Stack

* obtain stack frames
* frame instruction address
* stack/frame information
* symbol information when available

### Modules

* enumerate loaded modules
* module base
* module size
* module path/name

### Breakpoints

* instruction/code breakpoint
* enable/disable
* remove
* breakpoint hit event

### Memory Breakpoints / Watchpoints

Support the generic concept of a data breakpoint/watchpoint.

The contract MUST NOT assume Windows hardware-debug-register semantics.

The WinDbg/DbgEng implementation should map it to the appropriate DbgEng mechanism.

Support, where DbgEng permits:

* execute
* read
* write
* read/write

### Exceptions

Capture:

* exception code
* exception address
* first/second chance information where available

### Events

Represent debugger events through generic domain types.

At minimum account for:

```text
process created
process exited
thread created
thread exited
module loaded
module unloaded
breakpoint hit
exception
execution stopped
```

---

# 11. Do NOT Overfit the Generic Contract to DbgEng

This is extremely important.

DbgEng supports many things which may not exist identically in GDB, LLDB, x64dbg, or remote debugging.

Do NOT blindly expose DbgEng concepts as generic concepts.

For example, do not create a generic API whose meaning is essentially:

```cpp
set_dbgeng_execution_status(...)
```

Instead expose the semantic operation:

```cpp
continue_execution()
```

Similarly, avoid leaking:

```text
DbgEng thread indexes
DbgEng engine IDs
Windows HANDLEs
HRESULT
DEBUG_* constants
```

The generic contract should represent the semantic concept.

The concrete implementation performs the mapping.

---

# 12. Error Handling

Define proper generic debugger errors.

Do not make callers understand raw HRESULT values.

The implementation should translate native errors into the project's generic error/result model.

Errors should preserve enough information for diagnosis, for example:

```text
operation
backend
native error information
human-readable description
```

But do not expose Windows-specific error types through the generic contract unless the project architecture explicitly requires an opaque backend error payload.

---

# 13. Debugger State Machine

Define and enforce meaningful debugger states.

At minimum investigate states similar to:

```text
Created
Launching
Attaching
Running
Stopped
Exiting
Exited
Detached
Failed
```

The exact model should follow existing project conventions.

Operations that are invalid for the current state should fail deterministically rather than producing undefined behavior.

---

# 14. Events and Callbacks

Implement DbgEng callbacks and translate them into generic debugger events.

Callbacks should remain lightweight.

Do NOT perform expensive application-level work directly from a DbgEng callback.

Conceptually:

```text
DbgEng callback
    ↓
capture relevant information
    ↓
translate to generic DebugEvent
    ↓
enqueue/publish event
    ↓
return to DbgEng
```

Do not call the decompiler, analyzer, PE loader, or unrelated services directly from DbgEng callbacks.

---

# 15. Test Debuggee

Create a realistic debugger test program under the service's test data directory, following the existing repository convention.

Use:

```text
tests/data
```

and inspect:

```text
NEW/services/analyzers/subroutine_references/tests/data
```

before implementing it.

The test debuggee MUST be compiled with MSVC using a `build.bat`.

The resulting executable must be a real runnable Windows executable.

Do NOT use a trivial:

```cpp
int main() {
    return 42;
}
```

program.

The debuggee must deliberately exercise debugger functionality.

---

# 16. Test Debuggee Requirements

The test executable MUST contain meaningful code and multiple execution paths.

Include:

### Multiple functions

For example:

```text
main
worker
compute
process_data
recursive/helper function
```

with actual data flow between them.

### Multiple threads

Create several threads.

Each thread should execute distinguishable work so the debugger tests can:

* enumerate threads
* identify threads
* switch current thread
* inspect different thread contexts
* stop on different threads

At least one worker thread should remain active long enough for deterministic debugger tests.

### Stack usage

Create functions with several nested calls and local variables so stack inspection is meaningful.

### Heap usage

Allocate data dynamically and manipulate it.

This allows tests to inspect:

```text
heap-backed objects
pointers
memory contents
```

### Globals/static data

Have deterministic global/static values that can be located and inspected.

### Loops

Include deterministic loops suitable for:

```text
step over
step into
continue
breakpoint
```

### Function calls

Use multiple levels of function calls so stack frames and stepping can be demonstrated.

### Memory accesses

Include deterministic reads/writes to a known variable or buffer so memory breakpoints/watchpoints can be tested.

### Exceptions

Include a controlled exception path if it can be tested reliably without making the entire integration suite flaky.

---

# 17. Comments in the Test Debuggee

The test `.cpp` source itself MUST contain comments explaining why each piece exists.

For example:

```cpp
// Used by debugger integration tests to verify reading/writing process memory.
volatile std::uint64_t g_debug_value = ...;

// This worker intentionally performs nested calls so stack-frame inspection
// and thread switching can be tested.
void worker(...)
{
    ...
}
```

The test program is part of the debugger specification and should therefore be understandable to future developers.

---

# 18. Deterministic Test Design

Debugger tests are notoriously easy to make flaky.

Design the debuggee and tests to be deterministic.

Avoid relying on:

```text
sleep(10)
hope the breakpoint happens
```

where possible.

Use explicit synchronization primitives:

```text
mutex
condition_variable
atomics
events
```

to create known execution points.

The debugger should be able to attach/launch and deterministically reach states required by tests.

---

# 19. Required Integration Tests

Create comprehensive GoogleTest integration tests using the **generic debugger contract**, NOT the concrete DbgEng implementation.

This requirement is critical.

Tests should conceptually look like:

```text
test
 ↓
generic Debugger interface
 ↓
WinDbgEng service
 ↓
DbgEng
 ↓
test debuggee.exe
```

NOT:

```text
test
 ↓
IDebugClient
 ↓
DbgEng
```

Tests must never directly call DbgEng APIs.

The concrete implementation is an implementation detail.

---

# 20. Mandatory Test Coverage

At minimum implement integration tests for:

### Launch

* launch the debuggee
* verify process/session state

### Initial stop

* verify the debugger reaches a deterministic stopped state

### Process information

* obtain process information
* verify PID/state

### Threads

* enumerate all expected threads
* identify the current thread
* switch/select another thread
* inspect its context

### Registers

While stopped:

* read RIP
* read RSP
* read general-purpose registers
* verify values are plausible/non-empty
* compare contexts between different threads where appropriate

### Memory

* locate a deterministic test variable
* read its memory through the generic API
* verify expected value
* write memory where appropriate
* verify the changed value

### Stack

* obtain stack frames
* verify multiple frames exist
* verify frame instruction addresses are meaningful

### Modules

* enumerate loaded modules
* verify the debuggee module exists
* verify its base address and size

### Code breakpoint

Set a breakpoint on a deterministic function/instruction.

Verify:

```text
breakpoint installed
    ↓
continue
    ↓
breakpoint hit
    ↓
execution stopped
    ↓
current instruction address is correct
```

### Step Into

From a deterministic function call:

```text
caller
  ↓
callee
```

execute `step_into()` and verify that execution enters the callee.

### Step Over

From a deterministic function call:

```text
caller
  ↓
callee
```

execute `step_over()` and verify that execution advances past the call without stopping inside the callee.

### Step Out

Enter a function and execute `step_out()`.

Verify that execution returns to the caller.

### Memory breakpoint/watchpoint

Set a memory breakpoint on a deterministic variable.

Continue execution.

Verify that the debugger stops because of the memory access.

Test at least the relevant write case.

If DbgEng supports additional modes cleanly through the generic abstraction, cover them as well.

### Exception

Trigger the controlled test exception and verify that a generic exception event contains:

* exception code
* address
* first-chance information when available

### Thread switching

Stop execution while multiple threads exist.

Select different threads and verify their contexts can be queried independently.

### Continue/resume

Verify that a stopped process can resume and eventually reach the expected next stop condition.

### Termination / detach

Verify correct cleanup and session state transitions.

---

# 21. Tests Must Use the Generic Contract

This deserves repeating.

The test should depend on something like:

```text
core/contracts/debugger
```

and receive/use the debugger through that abstraction.

It must NOT include:

```cpp
dbgeng.h
```

and must NOT mention:

```cpp
IDebugClient
IDebugControl
DEBUG_STATUS_*
DEBUG_BREAKPOINT_*
```

etc.

This ensures that later we can run the same contract-level test suite against:

```text
WinDbgEng
x64dbg
GDB
LLDB
remote debugger
```

where the semantics are supported.

Backend-specific tests may exist only when testing behavior unique to that backend, and they must be clearly separated from generic contract tests.

---

# 22. GoogleTest Quality

Use the repository's existing GoogleTest style.

Tests must be:

* deterministic
* readable
* isolated where possible
* properly cleaned up
* descriptive
* sufficiently granular

Do not create one enormous:

```text
DebuggerEverythingTest
```

with 300 lines and ten unrelated assertions.

Prefer logically separated tests or fixtures.

Use comments where they explain debugger synchronization/state transitions.

---

# 23. Build the Test Debuggee with MSVC

Create:

```text
tests/data/build.bat
```

following the existing project's test-data conventions.

It must invoke the MSVC compiler/toolchain correctly.

The build must work from a normal developer environment with Visual Studio/MSVC installed.

The resulting `.exe` must be used by integration tests.

Do not check in generated `.exe` files unless repository conventions explicitly require it.

The test system should build the fixture when appropriate.

---

# 24. C++ Binding

Implement the appropriate binding under:

```text
NEW/bindings/cpp
```

using the project's existing binding conventions.

The binding should expose the generic debugger service/API, not raw DbgEng.

Do not duplicate debugger logic inside the binding.

The architecture should remain:

```text
core/contracts
       ↑
services/debugger/win_dbg_eng
       ↑
bindings/cpp
```

The binding is an adapter, not another debugger implementation.

---

# 25. Source Provenance

As with the other NEW modules, source comments must include references to the original Ghidra implementation wherever the implementation is conceptually/functionally derived from Ghidra.

Investigate and reference relevant Ghidra sources, especially:

```text
Ghidra/Debug/Debugger-agent-dbgeng
```

and relevant debugger API/agent code.

Use comments such as:

```cpp
// Origin/reference:
// Ghidra/Debug/Debugger-agent-dbgeng/...
//
// Ghidra uses DbgEng/DbgModel for Windows debugger integration.
// This implementation intentionally exposes the functionality through
// our backend-independent debugger contract.
```

Do NOT blindly copy Ghidra architecture.

We are implementing our own C++23 architecture while preserving useful behavioral knowledge from the original implementation.

Also reference Microsoft DbgEng documentation in comments where appropriate for non-obvious behavior, especially:

* client/thread affinity
* `WaitForEvent`
* execution status
* callback semantics
* breakpoint behavior

---

# 26. Documentation

Create/update appropriate documentation for the debugger service.

Document at least:

```text
architecture
threading model
execution model
DbgEng lifetime
event flow
state transitions
generic contract vs backend implementation
limitations imposed by DbgEng
test strategy
```

Clearly explain that:

```text
core/contracts
```

is backend-independent while:

```text
services/debugger/win_dbg_eng
```

contains Windows/DbgEng-specific code.

---

# 27. Do Not Touch Existing Services Unnecessarily

Do not refactor unrelated services.

Do not break existing tests.

Before changing anything, establish the current baseline.

After implementation:

1. Build the new service.
2. Run all new debugger tests.
3. Run affected existing tests.
4. Run the full relevant NEW test suite.
5. Fix regressions rather than weakening/removing tests.

**Never delete or disable an existing test simply because the new architecture makes it inconvenient.**

If an existing test must change because of an architectural migration, preserve its behavioral coverage and expand it where necessary.

---

# 28. No Functionality Cuts

Do not implement a "minimal fake debugger" merely to make tests pass.

The goal is a genuinely usable backend.

In particular, do NOT omit:

```text
step into
step over
step out
register context
stack
heap/process memory
code breakpoints
memory breakpoints/watchpoints
multiple threads
thread switching
modules
exceptions
execution events
```

All of these are required.

---

# 29. Suggested Implementation Order

Use this order unless the existing architecture strongly suggests a better one:

### Phase 1 — Architecture discovery

Study the repository and document the required integration points.

### Phase 2 — Generic domain/contracts

Implement:

```text
Address
ProcessId
ThreadId
Register
RegisterValue
ExecutionState
Process
Thread
Module
StackFrame
Breakpoint
Watchpoint
DebugException
DebugEvent
DebugSession
```

Only create the types that are actually needed.

Do not over-engineer speculative APIs.

### Phase 3 — Task integration

Integrate execution operations with the existing C++23 coroutine/task infrastructure.

### Phase 4 — DbgEng foundation

Implement:

```text
DebugCreate
client lifetime
DbgEng engine thread
callbacks
WaitForEvent
```

### Phase 5 — Process/session

Implement:

```text
launch
attach
detach
terminate
```

### Phase 6 — Runtime state

Implement:

```text
threads
registers
memory
stack
modules
```

### Phase 7 — Execution

Implement:

```text
continue
pause
step into
step over
step out
```

### Phase 8 — Breakpoints

Implement:

```text
code breakpoints
memory breakpoints/watchpoints
```

### Phase 9 — Events/exceptions

Implement event translation and exception handling.

### Phase 10 — Test debuggee

Build the realistic multithreaded MSVC debuggee.

### Phase 11 — Integration tests

Implement the complete GoogleTest suite through the generic contracts.

### Phase 12 — C++ binding

Add the binding.

### Phase 13 — Full validation

Run all relevant tests and fix regressions.

---

# 30. Important Research References

Before implementation, consult the official Microsoft DbgEng documentation, especially:

* `DebugCreate`
* Client Objects and the Engine
* Using Client Objects
* Using Callback Objects
* Debugging Session and Execution Model
* `IDebugControl`
* `IDebugRegisters`
* `IDebugDataSpaces`
* `IDebugSymbols`
* `IDebugSystemObjects`
* `IDebugEventCallbacks`
* `IDebugBreakpoint`

Also inspect the Ghidra implementation:

```text
Ghidra/Debug/Debugger-agent-dbgeng
```

and relevant Binary Ninja DbgEng integration/documentation as an additional practical reference.

Do not assume behavior from memory. Verify important DbgEng semantics against the documentation/source.

---

# 31. Final Acceptance Criteria

The task is complete only when ALL of the following are true:

* [ ] Generic debugger contracts exist in `core/contracts`.
* [ ] Any genuinely reusable debugger value/domain types are placed in `core/domain`.
* [ ] No generic contract depends on Windows/DbgEng/x64dbg/GDB/LLDB.
* [ ] All new code uses C++23 modules (`.cppm`), with no new `.h/.hpp/.cpp`.
* [ ] DbgEng implementation exists under:

  ```text
  services/debugger/win_dbg_eng
  ```
* [ ] DbgEng calls are isolated behind the generic contracts.
* [ ] DbgEng thread affinity is respected.
* [ ] Dedicated debugger engine thread is implemented correctly.
* [ ] `WaitForEvent()` / callback execution model is implemented correctly.
* [ ] Launch works.
* [ ] Attach works.
* [ ] Detach works.
* [ ] Termination works.
* [ ] Continue works.
* [ ] Pause/break works.
* [ ] Step into works.
* [ ] Step over works.
* [ ] Step out works.
* [ ] Register inspection works.
* [ ] Memory read works.
* [ ] Memory write works.
* [ ] Stack inspection works.
* [ ] Module enumeration works.
* [ ] Thread enumeration works.
* [ ] Thread switching works.
* [ ] Code breakpoints work.
* [ ] Memory breakpoints/watchpoints work.
* [ ] Exception events work.
* [ ] Debugger events are translated into generic events.
* [ ] A realistic multithreaded MSVC test debuggee exists under `tests/data`.
* [ ] The test debuggee is built by `build.bat`.
* [ ] The debuggee contains meaningful functions, nested calls, heap usage, globals, loops, memory accesses, multiple threads and controlled debugger scenarios.
* [ ] The test debuggee source contains explanatory comments.
* [ ] Comprehensive GoogleTest integration tests exist.
* [ ] Integration tests use ONLY the generic debugger contract.
* [ ] Tests do not include `dbgeng.h`.
* [ ] C++ binding exists under `NEW/bindings/cpp`.
* [ ] No `NEW/runtime` changes were made.
* [ ] Existing tests remain intact and passing.
* [ ] New tests are passing.
* [ ] Relevant full-suite validation passes.
* [ ] Source provenance comments reference relevant Ghidra sources and Microsoft documentation where useful.
* [ ] Documentation explains the architecture and threading/event model.

If something cannot be implemented exactly because DbgEng has a limitation, **do not silently remove the functionality**. Document the limitation, explain the semantic mismatch, and design the generic contract so another backend can support the capability later.

The final implementation should be production-quality C++23, not a proof-of-concept.
