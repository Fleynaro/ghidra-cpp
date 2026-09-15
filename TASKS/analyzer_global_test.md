We now need to build a **single large end-to-end integration test covering ALL currently implemented analyzers together**.

This is NOT a replacement for individual analyzer tests. Keep all existing unit/feature tests. This is an additional integration-level test proving that all analyzers can operate together without conflicts.

## Current architecture

Our implementation is under:

`NEW/features/analyzers`

Shared infrastructure:

`NEW/features/analyzers/shared`

All builtin analyzers can be registered through:

`register_builtin_analyzers_impl`

Existing analyzers include the already implemented ports such as:

* Constant Propagation
* Data Reference
* Scalar Operand References
* Stack
* Function Start Search
* Reference
* Non-Returning Functions
* Subroutine References
* Disassemble Entry Points

and the newer analyzer ports such as:

* Aggressive Instruction Finder
* Apply Data Archives
* ASCII Strings
* Call Convention ID
* Call-Fixup Installer
* Condense Filler Bytes
* Create Address Tables
* Decompiler Parameter ID
* Decompiler Switch Analysis
* Demangler Microsoft
* Embedded Media
* External Entry References
* Function ID
* PDB MSDIA
* PDB Universal
* Shared Return Calls
* Variadic Function Signature Override
* WindowsPE x86 Propagate External Parameters
* WindowsResourceReference
* x86 Constant Reference

**Do not assume this list is complete. Inspect the actual repository and registered builtin analyzers and include ALL analyzers currently implemented/registered.**

---

# 1. Create the integration-test directory

Create:

`NEW/features/analyzers/tests`

It should contain at minimum:

* one large C++ fixture,
* one large Google Test,
* one `run_ghidra.py`,
* build/run scripts as appropriate.

The integration test should register/run **all builtin analyzers together**, preferably through:

`register_builtin_analyzers_impl`

Do not manually select a convenient subset.

---

# 2. First study the existing tests

Before writing the fixture:

* inspect every existing analyzer test,
* inspect every existing `run_ghidra.py`,
* inspect their `.md` reports,
* understand what each analyzer actually tests,
* inspect the original Ghidra Java implementation of each analyzer where necessary.

Reuse and combine the important behavioral cases from the existing tests.

Do NOT blindly concatenate source files.

The new fixture must intentionally exercise interactions between analyzers.

---

# 3. Build ONE realistic, difficult C++ fixture

Create one substantial C++ program that combines the important cases from the existing analyzer fixtures.

Make it intentionally rich but still reasonable to compile and analyze.

The fixture should resemble a tiny non-graphical "GTA-like" engine.

For example, use concepts such as:

* `Entity`
* `Vehicle : Entity`
* `Ped : Entity`
* `Weapon : Entity`
* `Player : Ped`
* managers/world/game-state objects
* resource objects
* handles/IDs
* callbacks
* utility systems

No graphics are required. Use `printf`/console output.

The exact design is up to you, but it must provide substantial static-analysis complexity.

## REQUIRED language constructs

The fixture MUST contain meaningful examples of:

* multiple class hierarchies,
* virtual methods,
* non-virtual methods,
* constructors/destructors,
* overridden methods,
* standalone functions,
* overloaded functions where practical,
* recursive functions,
* mutually recursive functions where practical,
* global variables,
* static/global state,
* pointers and references,
* arrays,
* structures/classes,
* strings,
* string references,
* function pointers/callbacks,
* indirect calls where practical,
* direct calls,
* nested calls,
* conditional branches,
* loops,
* switch statements,
* jump tables where the compiler generates them,
* arithmetic/constants,
* stack variables,
* different argument types,
* return values,
* calls to system/runtime/library functions,
* resource-like data,
* embedded media/resource signatures where practical,
* intentionally unused/dead code,
* functions with different calling patterns,
* code/data adjacency,
* alignment/filler bytes,
* data referenced from code,
* code referenced from data where practical.

Include both normal and adversarial cases.

Do not artificially manufacture assembly. The C++ source should naturally compile into the required patterns.

---

# 4. Combine existing test cases intelligently

For every existing analyzer, identify the important behavior currently tested by its individual fixture.

The integration fixture should contain corresponding scenarios.

Examples:

* function discovery cases,
* stack-analysis cases,
* scalar/reference cases,
* string cases,
* data-reference cases,
* non-returning functions,
* shared-return cases,
* function-ID candidates,
* call-fixup/calling-convention cases,
* decompiler parameter/switch cases,
* PE/external-reference cases,
* resource/media cases,
* demangling cases,
* pointer/reference cases,
* filler/alignment cases,
* address-table cases.

Do not require every analyzer to create something artificially if its behavior depends on a specific binary condition. Instead construct the binary so the analyzer naturally encounters the condition.

---

# 5. Unified Ghidra reference generation

Create:

`NEW/features/analyzers/tests/run_ghidra.py`

This script must generate the reference data for the SAME binary.

Do not simply call the existing scripts one after another blindly.

Study all existing `run_ghidra.py` scripts and build one unified workflow that runs the relevant Ghidra analysis and collects information needed to validate the complete integration behavior.

The generated `.md` report must be detailed but MUST have a reasonable size limit.

It must NOT become tens of thousands of lines.

Implement explicit report limits such as:

* maximum total output size,
* maximum functions,
* maximum instructions per function,
* maximum references per function,
* maximum strings/data entries,
* maximum lines per section.

When limits are reached, clearly indicate truncation in the report.

The report should cover, where available:

* discovered functions,
* function names,
* addresses,
* signatures,
* return types,
* parameter types,
* calling conventions,
* stack frame information,
* stack variables,
* local variables,
* instructions,
* p-code where useful,
* references,
* callers/callees,
* symbols,
* strings,
* data,
* data types,
* globals,
* external references,
* imported/external functions,
* non-returning functions,
* switch/jump-table information,
* resources/media,
* comments/bookmarks where relevant,
* analyzer-created artifacts.

The `.md` file is a **development oracle only**.

The C++ test MUST NOT read or parse this `.md`.

---

# 6. Unified Google Test

Create one substantial integration GTest source file.

It should execute the C++ analyzer pipeline with **all builtin analyzers registered**.

The test must verify concrete results, not merely:

`EXPECT_TRUE(analyzer.run())`

Check meaningful final program state, including as applicable:

* expected functions exist,
* unexpected bogus functions do not exist,
* function boundaries are reasonable,
* signatures/parameters are recovered,
* stack frames/variables are correct,
* references exist,
* strings/data are discovered,
* global references are correct,
* call relationships are correct,
* non-returning behavior is recognized,
* switch structures are recognized,
* external references are handled,
* symbols/types are created where expected,
* analyzers do not corrupt previously discovered state.

Use the Ghidra reference report to determine the expected behavior, then encode those expectations directly into C++ assertions.

Never parse the `.md` at runtime.

---

# 7. Most important requirement: analyzer interaction

This test exists primarily to prove that analyzers work **together**.

Do not test only isolated final outputs.

Include assertions demonstrating that:

* one analyzer's output becomes another analyzer's input,
* analyzer scheduling/order works,
* re-analysis/events work,
* functions created by one analyzer are correctly processed by others,
* references discovered by one analyzer are consumed by others,
* analyzers do not overwrite/corrupt each other's state,
* repeated analysis is stable,
* running the complete pipeline does not produce duplicate or contradictory objects.

Where possible, test invariants such as:

* no duplicate functions,
* no overlapping invalid function bodies,
* no contradictory references,
* no corrupted stack information,
* no invalid symbols,
* deterministic final state.

The test should expose conflicts that isolated analyzer tests cannot detect.

---

# 8. Test quality

This must be a **real integration fixture**, not a giant toy program with superficial assertions.

The source should be complex enough that a broken analyzer implementation is likely to produce observable differences.

At the same time, keep the fixture maintainable and deterministic.

Avoid:

* random behavior,
* timing-dependent behavior,
* unnecessary templates,
* enormous generated code,
* external dependencies,
* network access,
* graphics,
* huge libraries.

Prefer deterministic native C++ compiled into a normal Windows executable.

---

# 9. Important restrictions

Do NOT weaken existing analyzer tests.

Do NOT delete existing tests.

Do NOT make the integration test depend on `.md`.

Do NOT hardcode arbitrary addresses unless they are stable and intentionally controlled.

Do NOT add fixture-specific hacks to production analyzers.

If the integration test exposes a real bug:

1. fix the production implementation,
2. add a focused regression test to the affected analyzer/module,
3. keep the integration test.

If the integration test exposes a shared infrastructure bug, fix the infrastructure instead.

---

# 10. Final validation

Run:

1. all existing analyzer tests,
2. the new full integration test,
3. the unified Ghidra reference generator.

Investigate every failure.

Do not simply change expectations until everything passes.

The final goal is:

**one realistic executable exercising the behavior of all analyzers together, with a detailed bounded Ghidra oracle and a large deterministic Google Test validating the resulting combined analysis state.**

At the end, report:

* which analyzers were included,
* which existing test cases were incorporated,
* what constructs the fixture contains,
* what the unified Ghidra report covers,
* how many GTests/assertions were added,
* any implementation bugs discovered,
* any shared-infrastructure bugs discovered,
* final test results.
