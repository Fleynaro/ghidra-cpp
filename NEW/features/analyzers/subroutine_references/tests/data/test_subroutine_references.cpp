// MSVC x64 integration fixture for Ghidra's "Subroutine References" analyzer.
//
// This deliberately uses ordinary machine-code constructs rather than a mock
// reference table. The harness disassembles the PE first, then enables only
// FunctionAnalyzer's "Subroutine References" option. Expected function, body,
// and reference state is copied from the generated Ghidra Delta into the C++
// tests; the tests never parse that report at runtime.
//
// Original behavior under test:
// * FunctionAnalyzer.added() gathers every reference whose ReferenceType.isCall()
//   is true, deduplicates targets with AddressSet, ignores fall-through calls,
//   and removes already-existing function entries.
// * CreateFunctionCmd.getFunctionBody() uses FollowFlow while excluding call
//   destinations, so a caller retains its fall-through path but never absorbs
//   the callee body.
// * CreateFunctionCmd.subtractBodyFromExisting() reconciles overlapping and
//   shared code, while CreateThunkFunctionCmd handles simple jump thunks.
// * Repeated analysis must be idempotent: ReferenceManager and FunctionManager
//   do not gain duplicate references or duplicate function entries.

// The volatile sink prevents the optimizer from deleting calls or folding the
// control-flow cases into constants. /OPT:NOREF and /OPT:NOICF keep all cases
// as separate, inspectable code in the generated PE.
extern "C" volatile unsigned int fixture_sink = 0U;

// Three callers share this target. FunctionAnalyzer must create exactly one
// function even though it sees multiple independent call references.
extern "C" __declspec(noinline) void target_shared() {
    fixture_sink = 0x11U;
}

// These unique targets distinguish independent discovery from target fan-in.
extern "C" __declspec(noinline) void target_alpha() {
    fixture_sink = 0x22U;
}

extern "C" __declspec(noinline) void target_beta() {
    fixture_sink = 0x33U;
}

extern "C" __declspec(noinline) void target_conditional() {
    fixture_sink = 0x44U;
}

extern "C" __declspec(noinline) void target_recursive_leaf() {
    fixture_sink ^= 0x55U;
}

// This target is reached through the wrapper below. On x64 MSVC a wrapper with
// no post-call side effect commonly becomes a tail jump, which exercises the
// native CreateThunkFunctionCmd-style simple jump recognition.
extern "C" __declspec(noinline) void target_thunked() {
    fixture_sink ^= 0x66U;
}

// An exported function is the existing-function negative case. Depending on
// the PE importer it may already have a function at analysis start; in either
// case the call-driven analyzer must never create a duplicate entry.
extern "C" __declspec(dllexport) __declspec(noinline) void known_existing() {
    fixture_sink ^= 0x77U;
}

// The first caller supplies a shared target, a unique target, a conditional
// branch around a call, and a repeated call to the same target. FollowFlow must
// retain both branch paths in the caller body while FunctionAnalyzer creates
// only the call targets.
extern "C" __declspec(noinline) void caller_one() {
    target_shared();
    fixture_sink += 1U;
    target_alpha();
    fixture_sink += 1U;
    if ((fixture_sink & 1U) != 0U) {
        target_conditional();
    }
    target_shared();
}

// The second caller contributes another shared target and a different unique
// target, then calls the already-known exported function.
extern "C" __declspec(noinline) void caller_two() {
    target_shared();
    fixture_sink += 2U;
    target_beta();
    fixture_sink += 2U;
    known_existing();
}

// A third caller creates another duplicate target and calls it twice. AddressSet
// target collection must collapse all shared calls.
extern "C" __declspec(noinline) void caller_three() {
    target_shared();
    fixture_sink += 3U;
    target_shared();
}

// This nested chain makes the discovered target also a caller. It verifies
// transitive scheduling without relying on a separate body analyzer.
extern "C" __declspec(noinline) void nested_inner() {
    target_recursive_leaf();
}

extern "C" __declspec(noinline) void nested_middle() {
    nested_inner();
    target_shared();
}

extern "C" __declspec(noinline) void nested_outer() {
    nested_middle();
    target_alpha();
}

// The recursive call is conditional, so this function contains a loop and a
// call reference to itself. Body traversal must terminate after one visit per
// instruction.
extern "C" __declspec(noinline) void recursive_case(unsigned int depth) {
    if (depth != 0U) {
        recursive_case(depth - 1U);
    }
    fixture_sink += depth;
}

// Mutually recursive functions exercise duplicate discovery through a cycle;
// neither function may be recreated when the other is analyzed later.
extern "C" __declspec(noinline) void mutual_b(unsigned int depth);

extern "C" __declspec(noinline) void mutual_a(unsigned int depth) {
    if (depth != 0U) {
        mutual_b(depth - 1U);
    }
    fixture_sink += 0x80U;
}

extern "C" __declspec(noinline) void mutual_b(unsigned int depth) {
    if (depth != 0U) {
        mutual_a(depth - 1U);
    }
    fixture_sink += 0x81U;
}

// A wrapper with no post-call side effect is a thunk candidate. The target is
// intentionally not called from fixture_entry directly, so it is discovered
// through the wrapper's call/jump relationship.
extern "C" __declspec(noinline) void thunk_like() {
    target_thunked();
}

// Alignment makes the terminal-flow boundary visible in the body report.
__declspec(align(32)) extern "C" __declspec(noinline) void padded_terminal() {
    fixture_sink ^= 0x90U;
}

// This function mixes a call, conditional jumps, and a final target call. It
// gives BasicBlockModel and SimpleBlockModel distinct leaders and successors
// without relying on compiler-generated jump-table data.
extern "C" __declspec(noinline) void mixed_flow(unsigned int selector) {
    target_beta();
    if ((selector & 1U) != 0U) {
        fixture_sink += 1U;
    } else if ((selector & 2U) != 0U) {
        fixture_sink += 2U;
    } else {
        fixture_sink += 3U;
    }
    padded_terminal();
}

// The indirect call has no statically resolved target reference. It must not
// cause the analyzer to invent a function at an arbitrary register value.
using FixtureFunction = void (*)();
volatile FixtureFunction indirect_slot = target_conditional;

extern "C" __declspec(noinline) void unresolved_indirect_call() {
    indirect_slot();
    fixture_sink += 4U;
}

// The linker entry reaches every caller family so fan-in, nested discovery,
// existing functions, recursive cycles, thunk-like code, terminal flow, and
// unresolved indirect flow all occur in one deterministic PE fixture.
extern "C" __declspec(noinline) void fixture_entry() {
    caller_one();
    caller_two();
    caller_three();
    nested_outer();
    recursive_case(2U);
    mutual_a(2U);
    thunk_like();
    mixed_flow(1U);
    unresolved_indirect_call();
}
