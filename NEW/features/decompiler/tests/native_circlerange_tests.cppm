module;

#include <gtest/gtest.h>

export module native_circlerange_tests;

import ghidra.decompiler;
import std;

// Port provenance: the scenario vectors and behavioral contracts originate in
// Ghidra/Features/Decompiler/src/decompile/unittests/testcirclerange.cc.
// This test uses the exported CircleRange implementation from
// NEW/features/decompiler/src/rangeutil.cppm and the exported OpBehavior
// implementations from NEW/features/decompiler/src/opbehavior.cppm directly.

namespace newghidra::decompiler::tests {

using ghidra::CircleRange;
using ghidra::int4;
using ghidra::OpBehavior;
using ghidra::OpCode;
using ghidra::uintb;

/// Describes one CircleRange constructor invocation from the original native scenarios.
/// Boundaries use the original half-open circular representation, and `size` is measured in bytes.
struct RangeSpec {
    uintb left;
    uintb right;
    int4 size;
    int4 step;
};

/// Describes the range representation expected after a successful native operation.
/// An exact range may also be empty; a non-exact expectation is used when the native API
/// correctly reports that a set cannot be represented by one CircleRange.
struct ExpectedRange {
    bool exact;
    bool empty;
    uintb minimum;
    uintb end;
    int4 step;
};

/// Describes an intersection or union case, including the native result code contract.
struct SetScenario {
    const char* name;
    RangeSpec first;
    RangeSpec second;
    int4 expected_code;
    ExpectedRange expected_range;
};

/// Describes a unary pull-back case through one real p-code operation behavior.
struct UnaryPullbackScenario {
    const char* name;
    RangeSpec output_range;
    OpCode opcode;
    int4 input_size;
    bool expected_valid;
    ExpectedRange expected_range;
};

/// Describes a binary pull-back case where one input is a known constant.
struct BinaryPullbackScenario {
    const char* name;
    RangeSpec output_range;
    OpCode opcode;
    int4 slot;
    uintb constant_value;
    int4 input_size;
    bool expected_valid;
    ExpectedRange expected_range;
};

/// Describes a unary push-forward case through one real p-code operation behavior.
struct UnaryPushScenario {
    const char* name;
    RangeSpec input_range;
    OpCode opcode;
    int4 output_size;
    bool expected_valid;
    ExpectedRange expected_range;
};

/// Describes a binary push-forward case and preserves the original 32-step bound.
struct BinaryPushScenario {
    const char* name;
    RangeSpec first;
    RangeSpec second;
    OpCode opcode;
    int4 output_size;
    bool expected_valid;
    ExpectedRange expected_range;
};

/// Constructs a native range from a table entry without changing its circular boundaries.
CircleRange make_range(const RangeSpec& spec) {
    return CircleRange(spec.left, spec.right, spec.size, spec.step);
}

/// Checks the observable CircleRange representation, including empty ranges and stride.
void expect_range(const CircleRange& actual, const ExpectedRange& expected) {
    // The original testEqual() only requires a non-empty conservative result
    // when its independently enumerated input cannot be represented exactly.
    if (!expected.exact) {
        EXPECT_FALSE(actual.isEmpty());
        // The original testcirclerange.cc oracle observes the conservative
        // one-piece approximation left behind when a push reports failure.
        EXPECT_EQ(actual.getMin(), expected.minimum);
        EXPECT_EQ(actual.getEnd(), expected.end);
        EXPECT_EQ(actual.getStep(), expected.step);
        return;
    }
    if (expected.empty) {
        EXPECT_TRUE(actual.isEmpty());
        return;
    }
    ASSERT_FALSE(actual.isEmpty());
    EXPECT_EQ(actual.getMin(), expected.minimum);
    EXPECT_EQ(actual.getEnd(), expected.end);
    EXPECT_EQ(actual.getStep(), expected.step);
}

/// Owns the native operation behavior objects used by all table-driven cases.
/// The registry follows OpBehavior::registerInstructions without constructing an Architecture or XML harness.
class NativeCircleRangeTest : public ::testing::Test {
protected:
    /// Registers every exported native p-code behavior once for this fixture family.
    static void SetUpTestSuite() {
        OpBehavior::registerInstructions(behaviors_, nullptr);
    }

    /// Releases the behavior objects allocated by the native registration routine.
    static void TearDownTestSuite() {
        for (OpBehavior* behavior : behaviors_)
            delete behavior;
        behaviors_.clear();
    }

    /// Looks up the registered native behavior for an opcode without assuming all enum slots are populated.
    static OpBehavior* behavior_for(OpCode opcode) {
        const std::size_t index = static_cast<std::size_t>(opcode);
        if (index >= behaviors_.size())
            return nullptr;
        return behaviors_[index];
    }

    /// Verifies that a scenario is exercising the real behavior registered for its exported OpCode.
    static void expect_behavior(OpCode opcode, bool unary) {
        OpBehavior* behavior = behavior_for(opcode);
        ASSERT_NE(behavior, nullptr);
        if (behavior == nullptr)
            return;
        EXPECT_EQ(behavior->getOpcode(), opcode);
        EXPECT_EQ(behavior->isUnary(), unary);
    }

private:
    inline static std::vector<OpBehavior*> behaviors_;
};

/// Verifies that the native operation registry exposes the unary and binary behaviors used below.
TEST_F(NativeCircleRangeTest, RegistersNativeOperationBehaviors) {
    const std::array<std::pair<OpCode, bool>, 18> operations{{
        {ghidra::CPUI_INT_NEGATE, true},
        {ghidra::CPUI_INT_2COMP, true},
        {ghidra::CPUI_INT_ZEXT, true},
        {ghidra::CPUI_INT_SEXT, true},
        {ghidra::CPUI_INT_ADD, false},
        {ghidra::CPUI_INT_SUB, false},
        {ghidra::CPUI_INT_RIGHT, false},
        {ghidra::CPUI_INT_SRIGHT, false},
        {ghidra::CPUI_INT_EQUAL, false},
        {ghidra::CPUI_INT_NOTEQUAL, false},
        {ghidra::CPUI_INT_CARRY, false},
        {ghidra::CPUI_INT_LESS, false},
        {ghidra::CPUI_INT_LESSEQUAL, false},
        {ghidra::CPUI_INT_SLESS, false},
        {ghidra::CPUI_INT_SLESSEQUAL, false},
        {ghidra::CPUI_INT_MULT, false},
        {ghidra::CPUI_INT_LEFT, false},
        {ghidra::CPUI_SUBPIECE, false},
    }};

    for (const auto& [opcode, unary] : operations)
        expect_behavior(opcode, unary);
}

/// Verifies representative values through the registered native operation behaviors rather than a test-only oracle.
TEST_F(NativeCircleRangeTest, EvaluatesNativeOperationBehaviors) {
    using BehaviorCase = std::tuple<OpCode, bool, int4, int4, uintb, uintb, uintb>;
    const std::array<BehaviorCase, 18> cases{{
        {ghidra::CPUI_INT_NEGATE, true, 1, 1, 0x12, 0, 0xED},
        {ghidra::CPUI_INT_2COMP, true, 1, 1, 0x12, 0, 0xEE},
        {ghidra::CPUI_INT_ZEXT, true, 2, 1, 0x12, 0, 0x12},
        {ghidra::CPUI_INT_SEXT, true, 2, 1, 0x80, 0, 0xFF80},
        {ghidra::CPUI_INT_ADD, false, 1, 1, 1, 2, 3},
        {ghidra::CPUI_INT_SUB, false, 1, 1, 1, 2, 0xFF},
        {ghidra::CPUI_INT_RIGHT, false, 1, 1, 0x80, 1, 0x40},
        {ghidra::CPUI_INT_SRIGHT, false, 1, 1, 0x80, 1, 0xC0},
        {ghidra::CPUI_INT_EQUAL, false, 1, 1, 3, 3, 1},
        {ghidra::CPUI_INT_NOTEQUAL, false, 1, 1, 3, 4, 1},
        {ghidra::CPUI_INT_CARRY, false, 1, 1, 0xFF, 1, 1},
        {ghidra::CPUI_INT_LESS, false, 1, 1, 1, 2, 1},
        {ghidra::CPUI_INT_LESSEQUAL, false, 1, 1, 2, 2, 1},
        {ghidra::CPUI_INT_SLESS, false, 1, 1, 0xFF, 1, 1},
        {ghidra::CPUI_INT_SLESSEQUAL, false, 1, 1, 0xFF, 0xFF, 1},
        {ghidra::CPUI_INT_MULT, false, 1, 1, 3, 4, 12},
        {ghidra::CPUI_INT_LEFT, false, 1, 1, 1, 2, 4},
        {ghidra::CPUI_SUBPIECE, false, 1, 4, 0x1234, 1, 0x12},
    }};

    for (const auto& [opcode, unary, output_size, input_size, first, second, expected] : cases) {
        SCOPED_TRACE(static_cast<int>(opcode));
        expect_behavior(opcode, unary);
        OpBehavior* behavior = behavior_for(opcode);
        ASSERT_NE(behavior, nullptr);
        if (behavior == nullptr)
            continue;
        const uintb actual = unary ? behavior->evaluateUnary(output_size, input_size, first)
                                   : behavior->evaluateBinary(output_size, input_size, first, second);
        EXPECT_EQ(actual, expected);
    }
}

/// Verifies every original intersection vector and checks that non-representable intersections leave the receiver
/// unchanged.
TEST_F(NativeCircleRangeTest, IntersectScenarios) {
    const std::vector<SetScenario> scenarios{
        {"circlerange_intersect1", {1, 20, 4, 1}, {10, 30, 4, 1}, 0, {true, false, 10, 20, 1}},
        {"circlerange_intersect2", {200, 10, 1, 1}, {250, 5, 1, 1}, 0, {true, false, 250, 5, 1}},
        {"circlerange_intersect3", {1, 250, 1, 1}, {240, 5, 1, 1}, 2, {false, false, 0, 0, 1}},
        {"circlerange_intersect4", {4, 100, 1, 4}, {248, 52, 1, 4}, 0, {true, false, 4, 52, 4}},
        {"circlerange_intersect5",
         {0x100000, 0x1000FE, 8, 2},
         {0xFFFFFFFFFFFFFFF0ULL, 0xFFFFFFFFFFFFFFFEULL, 8, 2},
         0,
         {true, true, 0, 0, 1}},
        {"circlerange_intersect6", {0x100, 0x110, 2, 4}, {0x110, 0x130, 2, 4}, 0, {true, true, 0, 0, 1}},
        {"circlerange_intersect7", {0xFFE0, 0x20, 2, 2}, {0, 0x20, 2, 2}, 0, {true, false, 0, 0x20, 2}},
        {"circlerange_intersect8", {0x80, 0x8, 1, 1}, {0xD0, 0x80, 1, 1}, 0, {true, false, 0xD0, 0x8, 1}},
    };

    for (const SetScenario& scenario : scenarios) {
        SCOPED_TRACE(scenario.name);
        CircleRange first = make_range(scenario.first);
        const CircleRange original = first;
        const CircleRange second = make_range(scenario.second);
        const int4 result_code = first.intersect(second);
        EXPECT_EQ(result_code, scenario.expected_code);
        if (scenario.expected_code != 0) {
            EXPECT_EQ(first, original);
        } else {
            expect_range(first, scenario.expected_range);
        }
    }
}

/// Verifies every original union vector and the two-piece result contract.
TEST_F(NativeCircleRangeTest, UnionScenarios) {
    const std::vector<SetScenario> scenarios{
        {"circlerange_union1", {1, 20, 4, 1}, {10, 30, 4, 1}, 0, {true, false, 1, 30, 1}},
        {"circlerange_union2", {200, 10, 1, 1}, {250, 5, 1, 1}, 0, {true, false, 200, 10, 1}},
        {"circlerange_union3", {1, 250, 1, 1}, {240, 5, 1, 1}, 0, {true, false, 0, 0, 1}},
        {"circlerange_union4", {4, 100, 1, 4}, {248, 52, 1, 4}, 0, {true, false, 248, 100, 4}},
        {"circlerange_union5",
         {0x100000, 0x1000FE, 8, 2},
         {0xFFFFFFFFFFFFFFF0ULL, 0xFFFFFFFFFFFFFFFEULL, 8, 2},
         2,
         {false, false, 0, 0, 1}},
        {"circlerange_union6", {0x100, 0x110, 2, 4}, {0x110, 0x130, 2, 4}, 0, {true, false, 0x100, 0x130, 4}},
        {"circlerange_union7", {0xFFE0, 0x20, 2, 2}, {0, 0x20, 2, 2}, 0, {true, false, 0xFFE0, 0x20, 2}},
        {"circlerange_union8", {0x80, 0x8, 1, 1}, {0xD0, 0x80, 1, 1}, 0, {true, false, 0, 0, 1}},
    };

    for (const SetScenario& scenario : scenarios) {
        SCOPED_TRACE(scenario.name);
        CircleRange first = make_range(scenario.first);
        const CircleRange original = first;
        const CircleRange second = make_range(scenario.second);
        const int4 result_code = first.circleUnion(second);
        EXPECT_EQ(result_code, scenario.expected_code);
        if (scenario.expected_code != 0) {
            EXPECT_EQ(first, original);
        } else {
            expect_range(first, scenario.expected_range);
        }
    }
}

/// Verifies all unary pull-back vectors, including extension truncation and empty preimages.
TEST_F(NativeCircleRangeTest, PullbackUnaryScenarios) {
    const std::vector<UnaryPullbackScenario> scenarios{
        {"circlerange_pullbacknegate1",
         {1, 20, 4, 1},
         ghidra::CPUI_INT_NEGATE,
         4,
         true,
         {true, false, 0xFFFFFFECULL, 0xFFFFFFFFULL, 1}},
        {"circlerange_pullbacknegate2",
         {0xF0, 0x10, 1, 1},
         ghidra::CPUI_INT_NEGATE,
         1,
         true,
         {true, false, 0xF0, 0x10, 1}},
        {"circlerange_pullbacknegate3",
         {0x10, 0x30, 4, 4},
         ghidra::CPUI_INT_NEGATE,
         4,
         true,
         {true, false, 0xFFFFFFD3ULL, 0xFFFFFFF3ULL, 4}},
        {"circlerange_pullbacknegate4", {0xFFF0, 0, 2, 4}, ghidra::CPUI_INT_NEGATE, 2, true, {true, false, 3, 0x13, 4}},
        {"circlerange_pullbacknegate5",
         {0xD1, 0x11, 1, 4},
         ghidra::CPUI_INT_NEGATE,
         1,
         true,
         {true, false, 0xF2, 0x32, 4}},
        {"circlerange_pullbacknegate6", {0, 0x30, 1, 4}, ghidra::CPUI_INT_NEGATE, 1, true, {true, false, 0xD3, 3, 4}},
        {"circlerange_pullbackminus1",
         {1, 20, 4, 1},
         ghidra::CPUI_INT_2COMP,
         4,
         true,
         {true, false, 0xFFFFFFEDULL, 0, 1}},
        {"circlerange_pullbackminus2",
         {0xF0, 0x10, 1, 1},
         ghidra::CPUI_INT_2COMP,
         1,
         true,
         {true, false, 0xF1, 0x11, 1}},
        {"circlerange_pullbackminus3",
         {0x10, 0x30, 4, 4},
         ghidra::CPUI_INT_2COMP,
         4,
         true,
         {true, false, 0xFFFFFFD4ULL, 0xFFFFFFF4ULL, 4}},
        {"circlerange_pullbackminus4", {0xFFF0, 0, 2, 4}, ghidra::CPUI_INT_2COMP, 2, true, {true, false, 4, 0x14, 4}},
        {"circlerange_pullbackminus5",
         {0xD1, 0x11, 1, 4},
         ghidra::CPUI_INT_2COMP,
         1,
         true,
         {true, false, 0xF3, 0x33, 4}},
        {"circlerange_pullbackminus6", {0, 0x30, 1, 4}, ghidra::CPUI_INT_2COMP, 1, true, {true, false, 0xD4, 4, 4}},
        {"circlerange_pullbackzext1", {1, 20, 4, 1}, ghidra::CPUI_INT_ZEXT, 2, true, {true, false, 1, 20, 1}},
        {"circlerange_pullbackzext2", {0xFFF0, 0xFF10, 2, 1}, ghidra::CPUI_INT_ZEXT, 1, true, {true, false, 0, 0, 1}},
        {"circlerange_pullbackzext3", {0x10, 0x30, 4, 4}, ghidra::CPUI_INT_ZEXT, 1, true, {true, false, 0x10, 0x30, 4}},
        {"circlerange_pullbackzext4", {0xFFF0, 0, 2, 4}, ghidra::CPUI_INT_ZEXT, 1, true, {true, true, 0, 0, 1}},
        {"circlerange_pullbackzext5", {0xFFD1, 0x11, 2, 4}, ghidra::CPUI_INT_ZEXT, 1, true, {true, false, 1, 0x11, 4}},
        {"circlerange_pullbackzext6", {0, 0x30, 4, 4}, ghidra::CPUI_INT_ZEXT, 2, true, {true, false, 0, 0x30, 4}},
        {"circlerange_pullbacksext1", {1, 20, 4, 1}, ghidra::CPUI_INT_SEXT, 2, true, {true, false, 1, 20, 1}},
        {"circlerange_pullbacksext2",
         {0xFFF0, 0x10, 2, 1},
         ghidra::CPUI_INT_SEXT,
         1,
         true,
         {true, false, 0xF0, 0x10, 1}},
        {"circlerange_pullbacksext3", {0x10, 0x30, 4, 4}, ghidra::CPUI_INT_SEXT, 2, true, {true, false, 0x10, 0x30, 4}},
        {"circlerange_pullbacksext4", {0xFFF0, 0, 2, 4}, ghidra::CPUI_INT_SEXT, 1, true, {true, false, 0xF0, 0, 4}},
        {"circlerange_pullbacksext5",
         {0xFFD1, 0x11, 2, 4},
         ghidra::CPUI_INT_SEXT,
         1,
         true,
         {true, false, 0xD1, 0x11, 4}},
        {"circlerange_pullbacksext6", {0, 0x30, 2, 4}, ghidra::CPUI_INT_SEXT, 1, true, {true, false, 0, 0x30, 4}},
    };

    for (const UnaryPullbackScenario& scenario : scenarios) {
        SCOPED_TRACE(scenario.name);
        expect_behavior(scenario.opcode, true);
        CircleRange actual = make_range(scenario.output_range);
        const bool valid = actual.pullBackUnary(scenario.opcode, scenario.input_size, scenario.output_range.size);
        EXPECT_EQ(valid, scenario.expected_valid);
        expect_range(actual, scenario.expected_range);
    }
}

/// Verifies the complete add and subtract pull-back vectors with modular arithmetic at each tested width.
TEST_F(NativeCircleRangeTest, PullbackArithmeticScenarios) {
    const std::vector<BinaryPullbackScenario> scenarios{
        {"circlerange_pullbackadd1",
         {1, 20, 4, 1},
         ghidra::CPUI_INT_ADD,
         0,
         0xFFFFFFFDULL,
         4,
         true,
         {true, false, 4, 0x17, 1}},
        {"circlerange_pullbackadd2",
         {0xF0, 0x10, 1, 1},
         ghidra::CPUI_INT_ADD,
         0,
         0xFFFFFFFDULL,
         1,
         true,
         {true, false, 0xF3, 0x13, 1}},
        {"circlerange_pullbackadd3",
         {0x10, 0x30, 4, 4},
         ghidra::CPUI_INT_ADD,
         0,
         0xFFFFFFFDULL,
         4,
         true,
         {true, false, 0x13, 0x33, 4}},
        {"circlerange_pullbackadd4",
         {0xFFF0, 0, 2, 4},
         ghidra::CPUI_INT_ADD,
         0,
         0xFFFFFFFDULL,
         2,
         true,
         {true, false, 0xFFF3, 3, 4}},
        {"circlerange_pullbackadd5",
         {0xD1, 0x11, 1, 4},
         ghidra::CPUI_INT_ADD,
         0,
         0xFFFFFFFDULL,
         1,
         true,
         {true, false, 0xD4, 0x14, 4}},
        {"circlerange_pullbackadd6",
         {0, 0x30, 1, 4},
         ghidra::CPUI_INT_ADD,
         0,
         0xFFFFFFFDULL,
         1,
         true,
         {true, false, 3, 0x33, 4}},
        {"circlerange_pullbacksub1",
         {1, 20, 4, 1},
         ghidra::CPUI_INT_SUB,
         0,
         0xFFFFFFFDULL,
         4,
         true,
         {true, false, 0xFFFFFFFEULL, 0x11, 1}},
        {"circlerange_pullbacksub2",
         {0xF0, 0x10, 1, 1},
         ghidra::CPUI_INT_SUB,
         0,
         0xFFFFFFFDULL,
         1,
         true,
         {true, false, 0xED, 0xD, 1}},
        {"circlerange_pullbacksub3",
         {0x10, 0x30, 4, 4},
         ghidra::CPUI_INT_SUB,
         0,
         0xFFFFFFFDULL,
         4,
         true,
         {true, false, 0xD, 0x2D, 4}},
        {"circlerange_pullbacksub4",
         {0xFFF0, 0, 2, 4},
         ghidra::CPUI_INT_SUB,
         0,
         0xFFFFFFFDULL,
         2,
         true,
         {true, false, 0xFFED, 0xFFFD, 4}},
        {"circlerange_pullbacksub5",
         {0xD1, 0x11, 1, 4},
         ghidra::CPUI_INT_SUB,
         0,
         0xFFFFFFFDULL,
         1,
         true,
         {true, false, 0xCE, 0xE, 4}},
        {"circlerange_pullbacksub6",
         {0, 0x30, 1, 4},
         ghidra::CPUI_INT_SUB,
         0,
         0xFFFFFFFDULL,
         1,
         true,
         {true, false, 0xFD, 0x2D, 4}},
    };

    for (const BinaryPullbackScenario& scenario : scenarios) {
        SCOPED_TRACE(scenario.name);
        expect_behavior(scenario.opcode, false);
        CircleRange actual = make_range(scenario.output_range);
        const bool valid = actual.pullBackBinary(scenario.opcode, scenario.constant_value, scenario.slot,
                                                 scenario.input_size, scenario.output_range.size);
        EXPECT_EQ(valid, scenario.expected_valid);
        expect_range(actual, scenario.expected_range);
    }
}

/// Verifies logical and arithmetic right-shift pull-back, including the documented stride rejection.
TEST_F(NativeCircleRangeTest, PullbackShiftScenarios) {
    const std::vector<BinaryPullbackScenario> scenarios{
        {"circlerange_pullbackright1",
         {1, 0xF, 2, 1},
         ghidra::CPUI_INT_RIGHT,
         0,
         8,
         2,
         true,
         {true, false, 0x100, 0xF00, 1}},
        {"circlerange_pullbackright2",
         {0xF0, 0x10, 2, 1},
         ghidra::CPUI_INT_RIGHT,
         0,
         8,
         2,
         true,
         {true, false, 0xF000, 0x1000, 1}},
        {"circlerange_pullbackright3",
         {0xF0, 0x10, 1, 1},
         ghidra::CPUI_INT_RIGHT,
         0,
         1,
         1,
         true,
         {true, false, 0, 0x20, 1}},
        {"circlerange_pullbackright4", {1, 0xF, 2, 2}, ghidra::CPUI_INT_RIGHT, 0, 8, 2, false, {false, false, 0, 0, 1}},
        {"circlerange_pullbacksright1",
         {1, 0xF, 2, 1},
         ghidra::CPUI_INT_SRIGHT,
         0,
         8,
         2,
         true,
         {true, false, 0x100, 0xF00, 1}},
        {"circlerange_pullbacksright2",
         {0xF0, 0x10, 1, 1},
         ghidra::CPUI_INT_SRIGHT,
         0,
         2,
         1,
         true,
         {true, false, 0xC0, 0x40, 1}},
        {"circlerange_pullbacksright3",
         {0x10, 0x30, 1, 1},
         ghidra::CPUI_INT_SRIGHT,
         0,
         2,
         1,
         true,
         {true, false, 0x40, 0x80, 1}},
        {"circlerange_pullbacksright4",
         {1, 0xF, 2, 2},
         ghidra::CPUI_INT_SRIGHT,
         0,
         8,
         2,
         false,
         {false, false, 0, 0, 1}},
    };

    for (const BinaryPullbackScenario& scenario : scenarios) {
        SCOPED_TRACE(scenario.name);
        expect_behavior(scenario.opcode, false);
        CircleRange actual = make_range(scenario.output_range);
        const CircleRange original = actual;
        const bool valid = actual.pullBackBinary(scenario.opcode, scenario.constant_value, scenario.slot,
                                                 scenario.input_size, scenario.output_range.size);
        EXPECT_EQ(valid, scenario.expected_valid);
        if (!scenario.expected_valid)
            EXPECT_EQ(actual, original);
        else
            expect_range(actual, scenario.expected_range);
    }
}

/// Verifies equality, inequality, carry, and unsigned/signed comparison pull-back vectors.
TEST_F(NativeCircleRangeTest, PullbackPredicateScenarios) {
    const std::vector<BinaryPullbackScenario> scenarios{
        {"circlerange_pullbackequal1",
         {1, 2, 1, 1},
         ghidra::CPUI_INT_EQUAL,
         0,
         0x1234,
         4,
         true,
         {true, false, 0x1234, 0x1235, 1}},
        {"circlerange_pullbackequal2",
         {0, 1, 1, 1},
         ghidra::CPUI_INT_EQUAL,
         0,
         0x1234,
         2,
         true,
         {true, false, 0x1235, 0x1234, 1}},
        {"circlerange_pullbacknotequal1",
         {0, 1, 1, 1},
         ghidra::CPUI_INT_NOTEQUAL,
         0,
         0x1234,
         4,
         true,
         {true, false, 0x1234, 0x1235, 1}},
        {"circlerange_pullbacknotequal2",
         {1, 2, 1, 1},
         ghidra::CPUI_INT_NOTEQUAL,
         0,
         0x1234,
         2,
         true,
         {true, false, 0x1235, 0x1234, 1}},
        {"circlerange_pullbackcarry1",
         {1, 2, 1, 1},
         ghidra::CPUI_INT_CARRY,
         0,
         0x1234,
         2,
         true,
         {true, false, 0xEDCC, 0, 1}},
        {"circlerange_pullbackcarry2",
         {0, 1, 1, 1},
         ghidra::CPUI_INT_CARRY,
         0,
         0x1234,
         2,
         true,
         {true, false, 0, 0xEDCC, 1}},
        {"circlerange_pullbackless1",
         {0, 1, 1, 1},
         ghidra::CPUI_INT_LESS,
         0,
         0x1234,
         4,
         true,
         {true, false, 0x1234, 0, 1}},
        {"circlerange_pullbackless2",
         {1, 2, 1, 1},
         ghidra::CPUI_INT_LESS,
         0,
         0x1234,
         2,
         true,
         {true, false, 0, 0x1234, 1}},
        {"circlerange_pullbacklessequal1",
         {0, 1, 1, 1},
         ghidra::CPUI_INT_LESSEQUAL,
         0,
         0x1234,
         4,
         true,
         {true, false, 0x1235, 0, 1}},
        {"circlerange_pullbacklessequal2",
         {1, 2, 1, 1},
         ghidra::CPUI_INT_LESSEQUAL,
         0,
         0x1234,
         2,
         true,
         {true, false, 0, 0x1235, 1}},
        {"circlerange_pullbacksless1",
         {0, 1, 1, 1},
         ghidra::CPUI_INT_SLESS,
         0,
         0x1234,
         4,
         true,
         {true, false, 0x1234, 0x80000000ULL, 1}},
        {"circlerange_pullbacksless2",
         {1, 2, 1, 1},
         ghidra::CPUI_INT_SLESS,
         0,
         0x1234,
         2,
         true,
         {true, false, 0x8000, 0x1234, 1}},
        {"circlerange_pullbackslessequal1",
         {0, 1, 1, 1},
         ghidra::CPUI_INT_SLESSEQUAL,
         0,
         0x1234,
         4,
         true,
         {true, false, 0x1235, 0x80000000ULL, 1}},
        {"circlerange_pullbackslessequal2",
         {1, 2, 1, 1},
         ghidra::CPUI_INT_SLESSEQUAL,
         0,
         0x1234,
         2,
         true,
         {true, false, 0x8000, 0x1235, 1}},
    };

    for (const BinaryPullbackScenario& scenario : scenarios) {
        SCOPED_TRACE(scenario.name);
        expect_behavior(scenario.opcode, false);
        // Match the original CircleRange(true/false) target using the
        // serialized right/size/step fields, not an assumption about left.
        CircleRange actual(scenario.output_range.right - scenario.output_range.step, scenario.output_range.right,
                           scenario.output_range.size, scenario.output_range.step);
        const bool valid = actual.pullBackBinary(scenario.opcode, scenario.constant_value, scenario.slot,
                                                 scenario.input_size, scenario.output_range.size);
        EXPECT_EQ(valid, scenario.expected_valid);
        expect_range(actual, scenario.expected_range);
    }
}

/// Verifies every original unary push-forward vector, including failed two-piece extensions.
TEST_F(NativeCircleRangeTest, PushUnaryScenarios) {
    const std::vector<UnaryPushScenario> scenarios{
        {"circlerange_pushnegate1",
         {1, 20, 4, 1},
         ghidra::CPUI_INT_NEGATE,
         4,
         true,
         {true, false, 0xFFFFFFECULL, 0xFFFFFFFFULL, 1}},
        {"circlerange_pushnegate2", {0xF0, 0x10, 1, 1}, ghidra::CPUI_INT_NEGATE, 1, true, {true, false, 0xF0, 0x10, 1}},
        {"circlerange_pushnegate3",
         {0x10, 0x30, 4, 4},
         ghidra::CPUI_INT_NEGATE,
         4,
         true,
         {true, false, 0xFFFFFFD3ULL, 0xFFFFFFF3ULL, 4}},
        {"circlerange_pushnegate4", {0xFFF0, 0, 2, 4}, ghidra::CPUI_INT_NEGATE, 2, true, {true, false, 3, 0x13, 4}},
        {"circlerange_pushnegate5", {0xD1, 0x11, 1, 4}, ghidra::CPUI_INT_NEGATE, 1, true, {true, false, 0xF2, 0x32, 4}},
        {"circlerange_pushnegate6", {0, 0x30, 1, 4}, ghidra::CPUI_INT_NEGATE, 1, true, {true, false, 0xD3, 3, 4}},
        {"circlerange_pushminus1", {1, 20, 4, 1}, ghidra::CPUI_INT_2COMP, 4, true, {true, false, 0xFFFFFFEDULL, 0, 1}},
        {"circlerange_pushminus2", {0xF0, 0x10, 1, 1}, ghidra::CPUI_INT_2COMP, 1, true, {true, false, 0xF1, 0x11, 1}},
        {"circlerange_pushminus3",
         {0x10, 0x30, 4, 4},
         ghidra::CPUI_INT_2COMP,
         4,
         true,
         {true, false, 0xFFFFFFD4ULL, 0xFFFFFFF4ULL, 4}},
        {"circlerange_pushminus4", {0xFFF0, 0, 2, 4}, ghidra::CPUI_INT_2COMP, 2, true, {true, false, 4, 0x14, 4}},
        {"circlerange_pushminus5", {0xD1, 0x11, 1, 4}, ghidra::CPUI_INT_2COMP, 1, true, {true, false, 0xF3, 0x33, 4}},
        {"circlerange_pushminus6", {0, 0x30, 1, 4}, ghidra::CPUI_INT_2COMP, 1, true, {true, false, 0xD4, 4, 4}},
        {"circlerange_pushzext1", {1, 20, 2, 1}, ghidra::CPUI_INT_ZEXT, 4, true, {true, false, 1, 20, 1}},
        {"circlerange_pushzext2",
         {0xFFF0, 0xFF10, 2, 1},
         ghidra::CPUI_INT_ZEXT,
         4,
         true,
         {false, false, 0xFFF0, 0xFF0F, 1}},
        {"circlerange_pushzext3", {0x10, 0x30, 2, 4}, ghidra::CPUI_INT_ZEXT, 4, true, {true, false, 0x10, 0x30, 4}},
        {"circlerange_pushzext4", {0xFFF0, 0, 2, 4}, ghidra::CPUI_INT_ZEXT, 4, true, {true, false, 0xFFF0, 0x10000, 4}},
        {"circlerange_pushzext5",
         {0xFFD1, 0xFFF1, 2, 4},
         ghidra::CPUI_INT_ZEXT,
         4,
         true,
         {true, false, 0xFFD1, 0xFFF1, 4}},
        {"circlerange_pushzext6", {0, 0x30, 1, 4}, ghidra::CPUI_INT_ZEXT, 2, true, {true, false, 0, 0x30, 4}},
        {"circlerange_pushzext7", {0, 0, 1, 4}, ghidra::CPUI_INT_ZEXT, 2, true, {true, false, 0, 0x100, 4}},
        {"circlerange_pushsext1", {1, 20, 2, 1}, ghidra::CPUI_INT_SEXT, 4, true, {true, false, 1, 20, 1}},
        {"circlerange_pushsext2",
         {0xFFF0, 0xFF10, 2, 1},
         ghidra::CPUI_INT_SEXT,
         4,
         true,
         {false, false, 0xFFFFFFF0ULL, 0xFFFFFF0FULL, 1}},
        {"circlerange_pushsext3", {0x10, 0x30, 2, 4}, ghidra::CPUI_INT_SEXT, 4, true, {true, false, 0x10, 0x30, 4}},
        {"circlerange_pushsext4",
         {0xFFF0, 0, 2, 4},
         ghidra::CPUI_INT_SEXT,
         4,
         true,
         {true, false, 0xFFFFFFF0ULL, 0, 4}},
        {"circlerange_pushsext5",
         {0xFFD1, 0xFFF1, 2, 4},
         ghidra::CPUI_INT_SEXT,
         4,
         true,
         {true, false, 0xFFFFFFD1ULL, 0xFFFFFFF1ULL, 4}},
        {"circlerange_pushsext6", {0, 0x30, 1, 4}, ghidra::CPUI_INT_SEXT, 2, true, {true, false, 0, 0x30, 4}},
        {"circlerange_pushsext7", {0, 0, 1, 4}, ghidra::CPUI_INT_SEXT, 2, true, {true, false, 0xFF80, 0x80, 4}},
    };

    for (const UnaryPushScenario& scenario : scenarios) {
        SCOPED_TRACE(scenario.name);
        expect_behavior(scenario.opcode, true);
        const CircleRange input = make_range(scenario.input_range);
        CircleRange actual;
        const bool valid =
            actual.pushForwardUnary(scenario.opcode, input, scenario.input_range.size, scenario.output_size);
        if (scenario.expected_range.exact) {
            EXPECT_EQ(valid, scenario.expected_valid);
        } else {
            // In testcirclerange.cc, testEqual() returns true after verifying
            // an intentionally non-representable result, although the native
            // push operation itself must return false.
            EXPECT_TRUE(scenario.expected_valid);
            EXPECT_FALSE(valid);
        }
        // The original testEqual helper validates the output range even when
        // the operation reports that it cannot represent the result.
        expect_range(actual, scenario.expected_range);
    }
}

/// Verifies every original binary push-forward vector, including multiplication stride growth and truncation.
TEST_F(NativeCircleRangeTest, PushBinaryScenarios) {
    const std::vector<BinaryPushScenario> scenarios{
        {"circlerange_pushadd1",
         {10, 15, 1, 1},
         {30, 35, 1, 1},
         ghidra::CPUI_INT_ADD,
         1,
         true,
         {true, false, 40, 49, 1}},
        {"circlerange_pushadd2",
         {1, 10, 4, 1},
         {0xFFFFFFFEULL, 5, 4, 1},
         ghidra::CPUI_INT_ADD,
         4,
         true,
         {true, false, 0xFFFFFFFFULL, 0xE, 1}},
        {"circlerange_pushadd3",
         {0, 20, 2, 4},
         {0xFFF0, 6, 2, 2},
         ghidra::CPUI_INT_ADD,
         2,
         true,
         {true, false, 0xFFF0, 0x16, 2}},
        {"circlerange_pushadd4", {1, 250, 1, 1}, {20, 30, 1, 1}, ghidra::CPUI_INT_ADD, 1, true, {true, false, 0, 0, 1}},
        {"circlerange_pushmult1",
         {0x1000, 0x1010, 4, 1},
         {2, 3, 4, 1},
         ghidra::CPUI_INT_MULT,
         4,
         true,
         {true, false, 0x2000, 0x2020, 2}},
        {"circlerange_pushmult2",
         {0xFFFC, 8, 2, 2},
         {4, 5, 2, 1},
         ghidra::CPUI_INT_MULT,
         2,
         true,
         {true, false, 0xFFF0, 0x20, 8}},
        {"circlerange_pushmult3", {5, 133, 1, 1}, {2, 3, 1, 1}, ghidra::CPUI_INT_MULT, 1, true, {true, false, 0, 0, 2}},
        {"circlerange_pushleft1", {1, 5, 4, 1}, {1, 2, 4, 1}, ghidra::CPUI_INT_LEFT, 4, true, {true, false, 2, 10, 2}},
        {"circlerange_pushleft2", {8, 72, 1, 4}, {2, 3, 1, 1}, ghidra::CPUI_INT_LEFT, 1, true, {true, false, 0, 0, 16}},
        {"circlerange_pushsubpiece1",
         {0xFFFE, 0x10005, 4, 1},
         {0, 1, 4, 1},
         ghidra::CPUI_SUBPIECE,
         1,
         true,
         {true, false, 0xFE, 5, 1}},
        {"circlerange_pushsubpiece2",
         {0xFFFE, 0x10005, 4, 1},
         {1, 2, 4, 1},
         ghidra::CPUI_SUBPIECE,
         1,
         true,
         {true, false, 0xFF, 1, 1}},
        {"circlerange_pushsubpiece3",
         {0x10F0, 0x1200, 4, 1},
         {0, 1, 4, 1},
         ghidra::CPUI_SUBPIECE,
         1,
         true,
         {true, false, 0, 0, 1}},
        {"circlerange_pushright1",
         {0x30A6, 0x30C0, 2, 2},
         {4, 5, 2, 1},
         ghidra::CPUI_INT_RIGHT,
         2,
         true,
         {true, false, 778, 780, 1}},
        {"circlerange_pushright2",
         {0xFE00, 0xFFC0, 2, 0x20},
         {9, 10, 2, 1},
         ghidra::CPUI_INT_RIGHT,
         2,
         true,
         {true, false, 127, 128, 1}},
        {"circlerange_pushright3",
         {7, 10, 4, 1},
         {4, 5, 4, 1},
         ghidra::CPUI_INT_RIGHT,
         4,
         true,
         {true, false, 0, 1, 1}},
        {"circlerange_pushsright1",
         {0x3000, 0x3064, 2, 4},
         {3, 4, 2, 1},
         ghidra::CPUI_INT_SRIGHT,
         2,
         true,
         {true, false, 1536, 1549, 1}},
        {"circlerange_pushsright2",
         {0xFFF0, 0x24, 2, 4},
         {3, 4, 2, 1},
         ghidra::CPUI_INT_SRIGHT,
         2,
         true,
         {true, false, 0xFFFE, 5, 1}},
    };

    for (const BinaryPushScenario& scenario : scenarios) {
        SCOPED_TRACE(scenario.name);
        expect_behavior(scenario.opcode, false);
        const CircleRange first = make_range(scenario.first);
        const CircleRange second = make_range(scenario.second);
        CircleRange actual;
        const bool valid =
            actual.pushForwardBinary(scenario.opcode, first, second, scenario.first.size, scenario.output_size, 32);
        EXPECT_EQ(valid, scenario.expected_valid);
        // Preserve the original postcondition: a failed push may still leave
        // a documented conservative range that must remain observable.
        expect_range(actual, scenario.expected_range);
    }
}

/// Independently enumerates a small 8-bit ZEXT family in both directions.
/// This mirrors the finite-set oracle used by Ghidra's testcirclerange.cc
/// without replacing any of its table-driven scenarios.
TEST_F(NativeCircleRangeTest, ZextElementEnumerationOracle) {
    // The input contains 0xf8 through 0xff; ZEXT to 16 bits preserves each
    // element and therefore produces the exact half-open range [0xf8, 0x100).
    const CircleRange input(0xF8, 0, 1, 1);
    CircleRange pushed;
    ASSERT_TRUE(pushed.pushForwardUnary(ghidra::CPUI_INT_ZEXT, input, 1, 2));
    for (uintb value = 0; value < 0x100; ++value) {
        const bool expected = value >= 0xF8;
        EXPECT_EQ(pushed.contains(value), expected) << "push value=" << value;
    }

    // Pulling the same enumerated output back through ZEXT keeps exactly the
    // low-byte preimage.  The rangeutil.cppm implementation documents this
    // as intersection with the complete ZEXT image before masking to input.
    CircleRange pulled(0xF8, 0x100, 2, 1);
    ASSERT_TRUE(pulled.pullBackUnary(ghidra::CPUI_INT_ZEXT, 1, 2));
    for (uintb value = 0; value < 0x100; ++value) {
        const bool expected = value >= 0xF8;
        EXPECT_EQ(pulled.contains(value), expected) << "pullback value=" << value;
    }
}

} // namespace newghidra::decompiler::tests
