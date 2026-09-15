The new full analyzer integration test currently takes approximately **150 seconds (~2.5 minutes)** to complete.

We need to investigate and fix this as a **pure performance task**.

## 1. Profile first — DO NOT change behavior yet

Profile the full integration test and determine exactly where the time is spent.

Measure separately, where possible:

* test/fixture startup,
* PE loading,
* Sleigh decoding,
* analyzer manager/scheduling,
* each individual analyzer,
* decompiler operations,
* FunctionID,
* Ghidra/reference generation if relevant,
* test assertions/finalization.

Do not guess.

Use appropriate profiling/timing instrumentation and/or a profiler. If necessary, temporarily add high-resolution timing around analyzer execution and important shared-infrastructure operations.

Identify the actual top performance bottlenecks.

---

## 2. Create `REVIEW_REPORT.md`

Create/update:

`NEW/features/analyzers/tests/REVIEW_REPORT.md`

Add a **CRITICAL** section documenting the performance problem.

Include:

* current total runtime,
* exact environment/build configuration,
* measured bottlenecks,
* time spent by the slowest analyzers/operations,
* call paths or operations responsible where identifiable,
* estimated percentage of total runtime,
* why the operation is unnecessarily expensive,
* whether the problem is in an analyzer, shared infrastructure, dependency, or test harness.

Do NOT label something Critical merely because it sounds suspicious. Measure it.

---

## 3. Fix performance — WITHOUT REDUCING FUNCTIONALITY

After identifying the bottleneck, implement the smallest correct performance improvements.

**ABSOLUTE REQUIREMENT: preserve behavior 1:1.**

Do NOT:

* remove analyzer functionality,
* skip difficult cases,
* reduce analysis scope,
* disable analyzers,
* reduce test coverage,
* lower iteration limits,
* remove branches,
* replace real algorithms with approximations,
* add fixture-specific shortcuts,
* weaken assertions,
* make the integration test artificially smaller,
* cache results in a way that changes semantics.

The goal is to make the **same analysis produce the same results faster**.

Prefer genuine optimizations such as:

* eliminating repeated work,
* avoiding unnecessary full-memory scans,
* avoiding duplicate decoding,
* caching immutable/intermediate results where semantically safe,
* improving lookup structures,
* reducing redundant allocations/copies,
* avoiding repeated graph traversal,
* improving event/scheduling deduplication,
* batching operations,
* preventing accidental O(N²)/O(N³) behavior,
* reusing already-computed analysis state.

If the bottleneck is shared infrastructure, fix it there rather than adding an analyzer-specific workaround.

---

## 4. Prove that behavior did not change

After every significant optimization:

1. rebuild,
2. run the complete integration test,
3. run the affected analyzer's focused tests,
4. run the full analyzer test suite,
5. compare important final analysis results with the previous baseline.

A performance optimization is acceptable only if functionality and test expectations remain intact.

If an optimization changes behavior, revert it and find a semantically equivalent optimization.

---

## 5. Re-profile

After fixes, profile again.

Update `REVIEW_REPORT.md` with:

* before runtime,
* after runtime,
* improvement percentage,
* before/after timings of the major bottlenecks,
* remaining bottlenecks,
* exact changes responsible for the improvement.

If runtime is still excessive, continue optimizing the measured bottlenecks.

Do not stop after making an arbitrary small improvement.

### Final principle

**This task is PERFORMANCE ONLY.**

Do not redesign analyzer semantics or reduce functionality.

The target is:

> **exactly the same analysis and test coverage, substantially faster execution.**
