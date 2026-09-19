You are working inside the Ghidra source tree, and you have direct access to the original Ghidra Java implementation of its P-Code emulator. Use those Java sources as a reference for the semantics and architecture, but **do not simply wrap or expose Ghidra's existing emulator**. The goal is to implement our own extensible P-Code emulator in Python, using PyGhidra as the integration layer and Ghidra's Sleigh/P-Code infrastructure as the source of P-Code and reference semantics.

## Goal

Implement a **custom, extensible P-Code emulator** in:

```text
{workdir}/FUNC_RECOGNIZER_RESEARCH
```

The emulator must execute a function **P-Code instruction by P-Code instruction** and maintain a detailed execution state after every single P-Code instruction.

The primary purpose is research into function recognition / semantic P-Code analysis, so the emulator must be designed as a reusable execution core that can later be extended with additional functionality.

A major consumer of this emulator will be an execution-trace visualizer which produces detailed Markdown reports.

The important architectural idea is:

```text
                 PyGhidra / Ghidra
                         │
                         │ obtain Language / Program / P-Code
                         ▼
                 ┌─────────────────┐
                 │ Custom Emulator │
                 │      Core       │
                 └────────┬────────┘
                          │
              execute one P-Code op
                          │
                  ┌───────┴───────┐
                  │               │
                  ▼               ▼
               State          Extension hooks
            registers          / observers
            memory             / tracers
            unique             / analyzers
                  │               │
                  └───────┬───────┘
                          ▼
                  Execution Trace
                          │
                          ▼
                     Markdown
                       report
```

The custom emulator itself must be independent from the Markdown visualizer. The visualizer should be implemented as an extension/observer of the emulator rather than hard-coded into its execution loop.

---

# 1. First: study the original Ghidra implementation

Before implementing the emulator, inspect the relevant original Java sources in the Ghidra repository.

At minimum, study the implementation around:

```text
Ghidra/Framework/Emulation/src/main/java/ghidra/pcode/exec/
Ghidra/Framework/Emulation/src/main/java/ghidra/pcode/emu/
Ghidra/Framework/Emulation/src/main/java/ghidra/pcode/opbehavior/
```

Pay particular attention to:

```text
PcodeExecutor
PcodeExecutorState
DefaultPcodeExecutorState
BytesPcodeExecutorState
BytesPcodeExecutorStatePiece
PcodeArithmetic
BytesPcodeArithmetic
PcodeUseropLibrary
PcodeEmulator
PcodeThread
DefaultPcodeThread
SleighInstructionDecoder
```

Also inspect the relevant `OpBehavior*` implementations.

The objective is to understand the actual semantics of P-Code operations, state handling, memory/register access, branching, temporaries/unique space, loads/stores, etc.

Do not blindly port Ghidra's class hierarchy. Extract the semantics and architectural ideas that are useful for our own implementation.

Document important design decisions in the project README or an architecture/design document.

---

# 2. Core requirement: our own emulator

Implement a custom Python execution engine.

The emulator must execute a sequence of `PcodeOp` instructions one by one.

The core abstraction should look conceptually like:

```text
PCodeEmulator
    │
    ├── execution state
    │     ├── registers
    │     ├── memory
    │     ├── unique/temporary values
    │     └── control-flow state
    │
    ├── execute(op)
    ├── execute_program(ops)
    └── extension/observer hooks
```

The exact class names and decomposition are up to you after studying the Ghidra implementation, but keep the architecture clean and extensible.

Do NOT make the visualizer part of the emulator's core.

The emulator must provide hooks such as:

```text
before_op(...)
after_op(...)
on_memory_read(...)
on_memory_write(...)
on_register_read(...)
on_register_write(...)
on_branch(...)
on_call(...)
on_return(...)
```

or an equivalent observer/event interface.

The important property is that another component must be able to subscribe to execution and inspect the state without modifying the emulator itself.

---

# 3. P-Code semantics

The emulator must correctly implement the ordinary P-Code operations required by the test suite.

At minimum, cover the operations that naturally occur in normal compiler-generated x86/x86-64 code, including:

```text
COPY
LOAD
STORE

INT_EQUAL
INT_NOTEQUAL
INT_SLESS
INT_SLESSEQUAL
INT_LESS
INT_LESSEQUAL

INT_ZEXT
INT_SEXT

INT_ADD
INT_SUB
INT_CARRY
INT_SCARRY
INT_SBORROW

INT_2COMP
INT_NEGATE

INT_XOR
INT_AND
INT_OR

INT_LEFT
INT_RIGHT
INT_SRIGHT

INT_MULT
INT_DIV
INT_SDIV
INT_REM
INT_SREM

BOOL_NEGATE
BOOL_XOR
BOOL_AND
BOOL_OR

PIECE
SUBPIECE

BRANCH
CBRANCH
CALL
RETURN

CALLOTHER
```

Do not unnecessarily implement the entire P-Code universe up front. Implement what is needed for the test corpus, but structure the operation dispatch so new operations can be added cleanly.

Use the original Ghidra implementation to verify the exact semantics, especially integer width, masking, signedness, truncation, carry/borrow behavior, endian handling, and address-space semantics.

Do not silently use host-language integer semantics when they differ from P-Code semantics.

---

# 4. Execution state

The emulator must maintain explicit state.

At minimum:

```text
Registers
Memory
Unique / temporary variables
Program counter / control-flow position
```

Register values must preserve the bit width implied by the corresponding P-Code varnode.

Memory must support arbitrary address ranges rather than assuming that every memory object is a single scalar.

Represent memory in a way that makes it possible to later inspect contiguous regions.

For example, if a test function operates on:

```cpp
int array[4]
```

the visualizer should be able to recognize that the relevant memory region is:

```text
0x....:
    [10, 20, 30, 40]
```

rather than producing four unrelated low-level byte dumps.

The emulator does not have to perform advanced C/C++ type recovery. However, the trace layer should provide reasonable grouping of contiguous memory regions and should make arrays/strings/blocks of bytes readable.

---

# 5. Important: P-Code step granularity

A **step is exactly one P-Code instruction (`PcodeOp`)**.

Do not define one CPU instruction as one step.

For example:

```text
x86 instruction
    ↓
P-Code #1
P-Code #2
P-Code #3
P-Code #4
```

must generate four execution steps.

The trace must therefore be able to show:

```text
Step 0
  P-Code: ...
  State before
  State after

Step 1
  P-Code: ...
  State before
  State after

Step 2
  P-Code: ...
  State before
  State after
```

This is the central requirement of the project.

---

# 6. P-Code source

The test pipeline must start from a real compiled executable rather than fabricated P-Code alone.

Create:

```text
{workdir}/FUNC_RECOGNIZER_RESEARCH
```

with a structure similar to:

```text
FUNC_RECOGNIZER_RESEARCH/
    src/
    tests/
    output/
    scripts/
    build.bat
    README.md
    ...
```

The exact layout may be adjusted if you have a better design, but keep the emulator core, tests, generated executable, and generated reports clearly separated.

---

# 7. Create a C++ test corpus

Create a C++ source file containing exactly **10 simple test functions**.

The functions should exercise different forms of P-Code behavior.

At minimum include examples such as:

```cpp
int add(int x, int y);

int sum_array(const int* values, int count);

void add_to_list(int* values, int count, int value);
```

Add several more functions covering things like:

* local variables
* loops
* conditional branches
* array indexing
* simple pointer/memory writes
* multiplication
* min/max or conditional logic
* accumulation/reduction
* multiple return paths

Keep the functions simple enough that their execution can be understood manually.

Avoid optimization transforming the functions into something trivial or eliminating them entirely. Choose suitable MSVC compilation settings so all 10 functions remain available as recognizable functions in the resulting executable.

The functions should also use deterministic test inputs.

---

# 8. Build script

Create:

```text
build.bat
```

inside:

```text
FUNC_RECOGNIZER_RESEARCH
```

Use MSVC.

Inspect and follow the approach used in:

```text
FUNC_VECTOR_RESEARCH/build_bsim.bat
```

where useful.

The build script must produce a Windows `.exe` containing the 10 test functions.

The exact compiler flags are your responsibility, but make sure the resulting executable is suitable for PyGhidra analysis and that the functions are not optimized away.

---

# 9. PyGhidra integration

Run the Python side through:

```text
TEST\run_ghidra_python.bat
```

Do not invent a separate Python/Ghidra environment if the existing project already provides the correct PyGhidra setup.

Create a PyGhidra script which:

1. opens the generated `.exe`
2. obtains the relevant Ghidra `Program`
3. identifies the 10 test functions
4. obtains the Sleigh language / architecture information
5. obtains the P-Code for each function
6. converts it into the input representation used by our custom emulator
7. executes the function using our emulator
8. generates a Markdown trace
9. writes the trace into:

```text
FUNC_RECOGNIZER_RESEARCH/output/
```

---

# 10. Input state and function arguments

The emulator needs a deterministic initial state for every test function.

For each test function, define explicit test inputs.

For example:

```text
add(10, 32)
sum_array([1, 2, 3, 4], 4)
add_to_list([10, 20, 30], 3, 5)
```

Initialize the appropriate registers and memory so that the function starts from a meaningful state.

Do not fake the P-Code itself.

The P-Code must come from the compiled executable through Ghidra/Sleigh.

The test harness may construct the initial machine state.

Clearly document how function arguments are placed into registers/stack/memory for the x86-64 calling convention used by the compiled executable.

---

# 11. Execution trace visualizer

Build a separate trace/visualization layer on top of the emulator.

Its responsibility is to consume emulator events/state snapshots and generate a beautiful, readable Markdown report.

Each function should get its own report, for example:

```text
output/
    add.md
    sum_array.md
    add_to_list.md
    ...
```

The Markdown must be designed for humans, not machines.

The report should clearly show:

```text
Function
Arguments
Initial state
Execution steps
Final state
Validation result
```

For every P-Code step, show at minimum:

```text
Step N
P-Code operation

Registers changed
Memory changed
Relevant temporary/unique values

State before
State after
```

Do NOT dump the entire register file and entire memory on every step if that would make the report unreadable.

Prefer a useful "diff" representation:

```text
Before:
    RAX = 10
    RBX = 32

P-Code:
    INT_ADD RAX, RBX -> RAX

After:
    RAX = 42
    RBX = 32
```

For memory:

```text
Memory before:
    0x1000: [10, 20, 30, 40]

P-Code:
    STORE ...

Memory after:
    0x1000: [10, 25, 30, 40]
```

Where possible, group related contiguous memory into a human-readable block.

---

# 12. Arrays and memory visualization

This is particularly important.

If a region of memory represents something that looks like an array, render it as an array/block rather than as a meaningless sequence of individual bytes.

For example:

```text
Memory @ 0x000001F0

int[4]
┌─────┬─────┬─────┬─────┐
│  10 │  20 │  30 │  40 │
└─────┴─────┴─────┴─────┘
```

In Markdown, use tables or code blocks when they make the structure clearer.

Also show the raw address/range information so that the visualization remains technically useful.

Do not pretend to know a high-level type when that information is unavailable. Clearly distinguish:

```text
inferred representation
```

from:

```text
actual known metadata
```

---

# 13. Execution visualization should be extensible

The trace generation must be implemented as a consumer of emulator events/state, not by inserting Markdown-specific code throughout the opcode implementations.

For example, architect something conceptually similar to:

```text
PcodeEmulator
      │
      ├── PcodeOp execution
      │
      ├── state management
      │
      └── Event/Observer interface
               │
               ├── MarkdownTraceRecorder
               ├── Debug logger
               ├── Future semantic analyzer
               └── Future function-recognition logic
```

The intention is to later be able to add things such as:

```text
execution statistics
value provenance
data-flow analysis
semantic pattern detection
branch/path tracking
P-Code rewrite validation
function recognition features
```

without rewriting the emulator.

---

# 14. Branches and control flow

The emulator must not blindly execute P-Code as a flat list if the P-Code contains control flow.

It must correctly handle:

```text
BRANCH
CBRANCH
CALL
RETURN
```

according to P-Code semantics.

For a conditional branch, the actual runtime value of the condition must determine the executed path.

The execution trace should make control flow visible.

For example:

```text
Step 14
CBRANCH 0x1234, condition=1

→ branch taken
```

or:

```text
Step 14
CBRANCH 0x1234, condition=0

→ fall-through
```

Do not attempt exhaustive path exploration at this stage.

The requirement is a deterministic concrete execution trace for the selected test input.

---

# 15. Validation is mandatory

Do not stop when the emulator "runs without exceptions".

You must verify that it produces the correct semantic result.

For every test function:

1. execute the corresponding C++ function normally
2. execute the same function through the extracted Ghidra P-Code using our emulator
3. compare the final relevant result/state
4. report whether they agree

The generated Markdown must contain a validation section such as:

```text
## Validation

Expected result:
    42

Emulated result:
    42

Status:
    PASS
```

For array/memory-mutating functions:

```text
Expected memory:
    [10, 25, 30, 40]

Emulated memory:
    [10, 25, 30, 40]

Status:
    PASS
```

You should also inspect the generated Markdown reports manually after running the complete test suite.

Look for:

* wrong register values
* incorrect signed arithmetic
* incorrect masking/truncation
* endian mistakes
* broken memory accesses
* incorrect branch behavior
* incorrect CALL/RETURN behavior
* missing P-Code operations
* wrong function argument initialization
* incorrect array/memory visualization

Do not claim the emulator is correct merely because the Python script exited successfully.

---

# 16. Compare against Ghidra where useful

Because PyGhidra gives access to Ghidra itself, use the real Ghidra execution/semantic machinery as a reference where practical.

The goal is not to delegate all execution back to Ghidra.

Instead:

```text
Ghidra:
    source of P-Code
    reference for semantics

Our implementation:
    actual execution engine
    state tracking
    instrumentation
    tracing
    future extensibility
```

For selected operations or complete test functions, compare our emulator's state/results with Ghidra's own execution facilities where practical.

If there are differences, investigate them rather than papering over them.

---

# 17. Error handling

When an unsupported P-Code operation is encountered, fail loudly and diagnostically.

Include:

```text
opcode
mnemonic
inputs
output
current instruction
current function
current step
```

and enough state information to reproduce the problem.

Do NOT silently skip unsupported operations.

If `CALLOTHER` / user-defined P-Code appears, determine whether the test corpus actually requires it.

For unsupported userops, report them clearly and only add support when needed.

---

# 18. Performance

Performance is not the primary goal yet.

Prioritize:

1. semantic correctness
2. deterministic execution
3. complete state tracking
4. good trace output
5. extensibility
6. only then optimization

However, avoid obviously pathological full-state copies on every step if the architecture can support efficient diffs/snapshots.

A practical design is welcome:

```text
state before
    ↓
execute one op
    ↓
state after
    ↓
compute relevant diff
```

rather than cloning enormous memory structures unnecessarily.

---

# 19. Tests

Create automated tests for the emulator core itself.

At minimum, include tests for:

```text
COPY
INT_ADD
INT_SUB
INT_AND
INT_OR
INT_XOR
INT_LEFT
INT_RIGHT
PIECE
SUBPIECE
LOAD
STORE
CBRANCH
BRANCH
```

Use small hand-written P-Code snippets where possible.

Then add integration tests which execute P-Code extracted from the real `.exe`.

The 10 C++ functions are the main end-to-end integration test corpus.

---

# 20. Final expected result

At the end of the task, the following should work from a clean checkout:

```text
build.bat
```

produces the test executable.

Then:

```text
TEST\run_ghidra_python.bat
```

runs the PyGhidra analysis/emulation pipeline.

The pipeline extracts real P-Code from the executable, executes it with our custom emulator, validates the result, and produces:

```text
FUNC_RECOGNIZER_RESEARCH/output/*.md
```

with a detailed and readable execution trace for all 10 functions.

The reports must make it possible for a human to answer questions such as:

```text
What was in RAX before this P-Code operation?
What changed after the operation?
What memory address was written?
What bytes/integers are currently stored there?
Which branch was taken?
What is the function's final result?
```

---

# 21. Deliverables

Do not consider the task complete until all of the following exist and work:

```text
FUNC_RECOGNIZER_RESEARCH/
    build.bat

    C++ test source
        10 deterministic test functions

    custom P-Code emulator
        reusable core
        explicit state
        opcode execution
        control flow
        extension hooks / observers

    PyGhidra integration
        executable loading
        function discovery
        P-Code extraction
        initial-state setup
        emulator execution

    trace visualizer
        MarkdownTraceRecorder or equivalent
        readable register diffs
        readable memory diffs
        array/block visualization
        control-flow information

    output/
        10 Markdown execution reports

    tests/
        emulator unit tests
        integration tests

    README.md
        how to build
        how to run
        architecture
        limitations
        validation results
```

---

# 22. Completion criterion

The task is complete only when:

```text
C++ source
    ↓
MSVC build
    ↓
.exe
    ↓
PyGhidra
    ↓
real Ghidra/Sleigh P-Code
    ↓
OUR custom emulator
    ↓
step-by-step execution
    ↓
register + memory state tracking
    ↓
Markdown visualization
    ↓
semantic validation
```

works end-to-end for all 10 test functions.

Do not stop at creating the emulator classes.

Do not stop at successfully extracting P-Code.

Do not stop at generating partial Markdown.

Run the complete pipeline, inspect the generated reports, identify and fix any incorrect results, and leave the repository in a working state.

At the end, provide a concise summary containing:

1. the final architecture
2. the important source files
3. which P-Code operations are implemented
4. how the PyGhidra integration works
5. how state and trace hooks work
6. where the generated reports are located
7. the validation results for all 10 functions
8. any known limitations or unsupported operations

The implementation should be clean enough that this emulator can later become the execution/semantic core for more advanced function-recognition research.
