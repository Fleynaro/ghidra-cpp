module;

#include <gtest/gtest.h>

export module native_scalar_tests;

import recode.decompiler;
import std;

namespace ghidra::native_scalar_tests {

// Port provenance: these fixed-width operands and expected results are copied
// from Ghidra/Features/Decompiler/src/decompile/unittests/testmultiprec.cc.
static ghidra::uintb num1[2] = {0xffffffffffffffff, 0xffffffffffffffff};
static ghidra::uintb denom1[2] = {1, 0};
static ghidra::uintb num2[2] = {0x89a732a9fb157c4d, 0x4eada2039e48443e};
static ghidra::uintb denom2[2] = {0xbabf3b71, 0};
static ghidra::uintb num3[2] = {0xf7df0315d584ad8d, 0xb9d55c0d1d5cfbbd};
static ghidra::uintb denom3[2] = {0x8aa797dbccee6e96, 0x646be9};
static ghidra::uintb a[2] = {0x1a309a9df2ce836a, 0xd66f2248906d1bdf};
static ghidra::uintb b[2] = {0xf6c190704eb1763e, 0xa05c42212dfba7c6};

// Converts the low 32 bits of an encoded value to a host float without
// changing the IEEE 754 bit pattern, matching testfloatemu.cc's memcpy helper.
// Original source: Ghidra/Features/Decompiler/src/decompile/unittests/testfloatemu.cc.
static float float_from_raw_bits(ghidra::uintb encoding) {
    return std::bit_cast<float>(static_cast<std::uint32_t>(encoding));
}

// Converts a host float to its exact 32-bit IEEE 754 encoding for comparison
// with FloatFormat's target encoding.
// Original source: Ghidra/Features/Decompiler/src/decompile/unittests/testfloatemu.cc.
static ghidra::uintb float_to_raw_bits(float value) {
    return static_cast<ghidra::uintb>(std::bit_cast<std::uint32_t>(value));
}

// Converts a 64-bit IEEE 754 encoding to a host double without altering bits.
// Original source: Ghidra/Features/Decompiler/src/decompile/unittests/testfloatemu.cc.
static double double_from_raw_bits(ghidra::uintb encoding) {
    return std::bit_cast<double>(static_cast<std::uint64_t>(encoding));
}

// Converts a host double to its exact 64-bit IEEE 754 encoding for comparison
// with FloatFormat's target encoding.
// Original source: Ghidra/Features/Decompiler/src/decompile/unittests/testfloatemu.cc.
static ghidra::uintb double_to_raw_bits(double value) {
    return static_cast<ghidra::uintb>(std::bit_cast<std::uint64_t>(value));
}

// Replaces testfloatemu.cc's ASSERT_FLOAT_ENCODING macro with a reusable GTest
// helper while retaining its exact double-input and raw-encoding assertion.
// Original source: Ghidra/Features/Decompiler/src/decompile/unittests/testfloatemu.cc.
static void expect_float_encoding(double value) {
    ghidra::FloatFormat format(4);
    const ghidra::uintb true_encoding = float_to_raw_bits(static_cast<float>(value));
    const ghidra::uintb encoding = format.getEncoding(value);
    EXPECT_EQ(true_encoding, encoding);
}

// Replaces testfloatemu.cc's ASSERT_DOUBLE_ENCODING macro with a reusable GTest
// helper while retaining its exact raw-encoding assertion.
// Original source: Ghidra/Features/Decompiler/src/decompile/unittests/testfloatemu.cc.
static void expect_double_encoding(double value) {
    ghidra::FloatFormat format(8);
    const ghidra::uintb true_encoding = double_to_raw_bits(value);
    const ghidra::uintb encoding = format.getEncoding(value);
    EXPECT_EQ(true_encoding, encoding);
}

// These values reproduce testfloatemu.cc's generated operation input set,
// including signed zero, denormals, normal extrema, NaN, and infinities.
// Original source: Ghidra/Features/Decompiler/src/decompile/unittests/testfloatemu.cc.
static const std::vector<float> float_test_values{
    -0.0f,
    +0.0f,
    -1.0f,
    +1.0f,

    -1.234f,
    +1.234f,

    -std::numeric_limits<float>::denorm_min(),
    std::numeric_limits<float>::denorm_min(),

    std::numeric_limits<float>::min() - std::numeric_limits<float>::denorm_min(),
    std::numeric_limits<float>::min(),
    std::numeric_limits<float>::min() + std::numeric_limits<float>::denorm_min(),

    -std::numeric_limits<float>::min() + std::numeric_limits<float>::denorm_min(),
    -std::numeric_limits<float>::min(),
    -std::numeric_limits<float>::min() - std::numeric_limits<float>::denorm_min(),

    std::numeric_limits<float>::max(),

    std::numeric_limits<float>::quiet_NaN(),

    -std::numeric_limits<float>::infinity(),
    std::numeric_limits<float>::infinity()};

// This list reproduces every integer edge and representative value used by
// testfloatemu.cc's integer-to-float operation test.
// Original source: Ghidra/Features/Decompiler/src/decompile/unittests/testfloatemu.cc.
static const std::vector<int> int_test_values = {
    0, -1, 1, 1234, -1234, std::numeric_limits<int>::min(), std::numeric_limits<int>::max()};

// Verifies all four original 128-bit division paths: one-word division,
// multi-word division, a numerator smaller than its denominator, and a
// quotient with a multi-word remainder.
// Original source: Ghidra/Features/Decompiler/src/decompile/unittests/testmultiprec.cc::multiprec_udiv.
TEST(Multiprecision, Udiv128) {
    ghidra::uintb q[2];
    ghidra::uintb r[2];

    ghidra::udiv128(num1, denom1, q, r);
    ASSERT_EQ(q[0], 0xffffffffffffffff);
    ASSERT_EQ(q[1], 0xffffffffffffffff);
    ASSERT_EQ(r[0], 0);
    ASSERT_EQ(r[1], 0);

    ghidra::udiv128(num2, denom2, q, r);
    ASSERT_EQ(q[0], 0x2a21eef2058d7e9a);
    ASSERT_EQ(q[1], 0x6bdaed99);
    ASSERT_EQ(r[0], 0x928d1c53);
    ASSERT_EQ(r[1], 0);

    ghidra::udiv128(num2, num1, q, r);
    ASSERT_EQ(q[0], 0);
    ASSERT_EQ(q[1], 0);
    ASSERT_EQ(r[0], num2[0]);
    ASSERT_EQ(r[1], num2[1]);

    ghidra::udiv128(num3, denom3, q, r);
    ASSERT_EQ(q[0], 0x1d9bc949e24);
    ASSERT_EQ(q[1], 0);
    ASSERT_EQ(r[0], 0x2e78197dc5048c75);
    ASSERT_EQ(r[1], 0x24d9cc);
}

// Verifies the 128-bit addition carry propagation and expected low-word
// wraparound from the original native multiprecision test.
// Original source: Ghidra/Features/Decompiler/src/decompile/unittests/testmultiprec.cc::multiprec_add.
TEST(Multiprecision, Add128) {
    ghidra::uintb res[2];
    ghidra::add128(a, b, res);
    ASSERT_EQ(res[0], 0x10f22b0e417ff9a8);
    ASSERT_EQ(res[1], 0x76cb6469be68c3a6);
}

// Verifies the 128-bit subtraction borrow propagation and wrapped unsigned
// result used by the original native multiprecision test.
// Original source: Ghidra/Features/Decompiler/src/decompile/unittests/testmultiprec.cc::multiprec_sub.
TEST(Multiprecision, Subtract128) {
    ghidra::uintb res[2];
    ghidra::subtract128(a, b, res);
    ASSERT_EQ(res[0], 0x236f0a2da41d0d2c);
    ASSERT_EQ(res[1], 0x3612e02762717418);
}

// Verifies a cross-word 51-bit logical left shift using the original 128-bit
// operand and exact expected encoding.
// Original source: Ghidra/Features/Decompiler/src/decompile/unittests/testmultiprec.cc::multiprec_left.
TEST(Multiprecision, LeftShift128) {
    ghidra::uintb res[2];
    ghidra::leftshift128(num2, res, 51);
    ASSERT_EQ(res[0], 0xe268000000000000);
    ASSERT_EQ(res[1], 0x21f44d39954fd8ab);
}

// Verifies strict and non-strict unsigned comparisons for equal, ordered, and
// distinct 128-bit values.
// Original source: Ghidra/Features/Decompiler/src/decompile/unittests/testmultiprec.cc::multiprec_less.
TEST(Multiprecision, UnsignedComparison128) {
    ASSERT_FALSE(ghidra::uless128(a, a));
    ASSERT_TRUE(ghidra::uless128(num2, num3));
    ASSERT_TRUE(ghidra::uless128(denom1, denom2));
    ASSERT_TRUE(ghidra::ulessequal128(a, a));
    ASSERT_TRUE(ghidra::ulessequal128(num2, num3));
    ASSERT_TRUE(ghidra::ulessequal128(denom2, denom2));
}

// Verifies exact IEEE 754 single-precision encodings for ordinary positive and
// negative values through the exported FloatFormat class.
// Original source: Ghidra/Features/Decompiler/src/decompile/unittests/testfloatemu.cc::float_encoding_normal.
TEST(FloatFormat, FloatEncodingNormal) {
    expect_float_encoding(1.234);
    expect_float_encoding(-1.234);
}

// Verifies exact IEEE 754 double-precision encodings for ordinary positive and
// negative values through the exported FloatFormat class.
// Original source: Ghidra/Features/Decompiler/src/decompile/unittests/testfloatemu.cc::double_encoding_normal.
TEST(FloatFormat, DoubleEncodingNormal) {
    expect_double_encoding(1.234);
    expect_double_encoding(-1.234);
}

// Verifies that positive and negative quiet NaNs retain the native sign and
// canonical payload expected by the original encoding test.
// Original source: Ghidra/Features/Decompiler/src/decompile/unittests/testfloatemu.cc::float_encoding_nan.
TEST(FloatFormat, FloatEncodingNan) {
    expect_float_encoding(std::numeric_limits<float>::quiet_NaN());
    expect_float_encoding(-std::numeric_limits<float>::quiet_NaN());
}

// Verifies exact positive and negative quiet-NaN encodings for double format.
// Original source: Ghidra/Features/Decompiler/src/decompile/unittests/testfloatemu.cc::double_encoding_nan.
TEST(FloatFormat, DoubleEncodingNan) {
    expect_double_encoding(std::numeric_limits<double>::quiet_NaN());
    expect_double_encoding(-std::numeric_limits<double>::quiet_NaN());
}

// Verifies the smallest positive and negative single-precision subnormal
// encodings.
// Original source: Ghidra/Features/Decompiler/src/decompile/unittests/testfloatemu.cc::float_encoding_subnormal.
TEST(FloatFormat, FloatEncodingSubnormal) {
    expect_float_encoding(std::numeric_limits<float>::denorm_min());
    expect_float_encoding(-std::numeric_limits<float>::denorm_min());
}

// Verifies the smallest positive and negative double-precision subnormal
// encodings.
// Original source: Ghidra/Features/Decompiler/src/decompile/unittests/testfloatemu.cc::double_encoding_subnormal.
TEST(FloatFormat, DoubleEncodingSubnormal) {
    expect_double_encoding(std::numeric_limits<double>::denorm_min());
    expect_double_encoding(-std::numeric_limits<double>::denorm_min());
}

// Verifies the minimum positive and negative normal single-precision values.
// Original source: Ghidra/Features/Decompiler/src/decompile/unittests/testfloatemu.cc::float_encoding_min_normal.
TEST(FloatFormat, FloatEncodingMinNormal) {
    expect_float_encoding(std::numeric_limits<float>::min());
    expect_float_encoding(-std::numeric_limits<float>::min());
}

// Verifies the minimum positive and negative normal double-precision values.
// Original source: Ghidra/Features/Decompiler/src/decompile/unittests/testfloatemu.cc::double_encoding_min_normal.
TEST(FloatFormat, DoubleEncodingMinNormal) {
    expect_double_encoding(std::numeric_limits<double>::min());
    expect_double_encoding(-std::numeric_limits<double>::min());
}

// Verifies positive and negative infinity encodings in single precision.
// Original source: Ghidra/Features/Decompiler/src/decompile/unittests/testfloatemu.cc::float_encoding_infinity.
TEST(FloatFormat, FloatEncodingInfinity) {
    expect_float_encoding(std::numeric_limits<float>::infinity());
    expect_float_encoding(-std::numeric_limits<float>::infinity());
}

// Verifies positive and negative infinity encodings in double precision.
// Original source: Ghidra/Features/Decompiler/src/decompile/unittests/testfloatemu.cc::double_encoding_infinity.
TEST(FloatFormat, DoubleEncodingInfinity) {
    expect_double_encoding(std::numeric_limits<double>::infinity());
    expect_double_encoding(-std::numeric_limits<double>::infinity());
}

// Verifies the exact minimum decimal precision strings for representative
// single-precision raw encodings.
// Original source: Ghidra/Features/Decompiler/src/decompile/unittests/testfloatemu.cc::float_decimal_precision.
TEST(FloatFormat, FloatDecimalPrecision) {
    ghidra::FloatFormat format(4);
    const float f0 = float_from_raw_bits(0x34000001);
    ASSERT_EQ(format.printDecimal(f0, false), "1.192093e-07");
    const float f1 = float_from_raw_bits(0x34800000);
    ASSERT_EQ(format.printDecimal(f1, false), "2.3841858e-07");
    const float f2 = float_from_raw_bits(0x3eaaaaab);
    ASSERT_EQ(format.printDecimal(f2, false), "0.33333334");
    const float f3 = float_from_raw_bits(0x3e800000);
    ASSERT_EQ(format.printDecimal(f3, false), "0.25");
    const float f4 = float_from_raw_bits(0x3de3ee46);
    ASSERT_EQ(format.printDecimal(f4, false), "0.111294314");
}

// Verifies the exact minimum decimal precision strings and forced scientific
// notation for representative double-precision raw encodings.
// Original source: Ghidra/Features/Decompiler/src/decompile/unittests/testfloatemu.cc::double_decimal_precision.
TEST(FloatFormat, DoubleDecimalPrecision) {
    ghidra::FloatFormat format(8);
    const double f0 = double_from_raw_bits(0x3fc5555555555555);
    ASSERT_EQ(format.printDecimal(f0, false), "0.16666666666666666");
    const double f1 = double_from_raw_bits(0x7fefffffffffffff);
    ASSERT_EQ(format.printDecimal(f1, false), "1.79769313486232e+308");
    const double f2 = double_from_raw_bits(0x3fd555555c7dda4b);
    ASSERT_EQ(format.printDecimal(f2, false), "0.33333334");
    const double f3 = double_from_raw_bits(0x3fd0000000000000);
    ASSERT_EQ(format.printDecimal(f3, false), "0.25");
    const double f4 = double_from_raw_bits(0x3fb999999999999a);
    ASSERT_EQ(format.printDecimal(f4, false), "0.1");
    const double f5 = double_from_raw_bits(0x3fbf7ced916872b0);
    ASSERT_EQ(format.printDecimal(f5, true), "1.23000000000000e-01");
}

// Verifies all six nearest-even midpoint cases by comparing FloatFormat's
// conversions with the host float conversion and checking tie direction.
// Original source: Ghidra/Features/Decompiler/src/decompile/unittests/testfloatemu.cc::float_midpoint_rounding.
TEST(FloatFormat, FloatMidpointRounding) {
    ghidra::FloatFormat format(4);

    // IEEE 754 recommends nearest-even rounding for binary floating-point;
    // these values cover below midpoint, even midpoint, and above midpoint
    // for both even and odd retained significands.
    const double d0 = double_from_raw_bits(0x4010000000000000);
    const double d1 = double_from_raw_bits(0x4010000010000000);
    const double d2 = double_from_raw_bits(0x4010000010000001);
    const double d3 = double_from_raw_bits(0x4010000020000000);
    const double d4 = double_from_raw_bits(0x4010000030000000);
    const double d5 = double_from_raw_bits(0x4010000030000001);

    const float f0 = static_cast<float>(d0);
    const float f1 = static_cast<float>(d1);
    const float f2 = static_cast<float>(d2);
    const float f3 = static_cast<float>(d3);
    const float f4 = static_cast<float>(d4);
    const float f5 = static_cast<float>(d5);

    const ghidra::uintb e0 = format.getEncoding(d0);
    const ghidra::uintb e1 = format.getEncoding(d1);
    const ghidra::uintb e2 = format.getEncoding(d2);
    const ghidra::uintb e3 = format.getEncoding(d3);
    const ghidra::uintb e4 = format.getEncoding(d4);
    const ghidra::uintb e5 = format.getEncoding(d5);

    ASSERT_EQ(float_to_raw_bits(f0), e0);
    ASSERT_EQ(float_to_raw_bits(f1), e1);
    ASSERT_EQ(float_to_raw_bits(f2), e2);
    ASSERT_EQ(float_to_raw_bits(f3), e3);
    ASSERT_EQ(float_to_raw_bits(f4), e4);
    ASSERT_EQ(float_to_raw_bits(f5), e5);

    ASSERT_EQ(e0, e1);
    ASSERT_NE(e1, e2);
    ASSERT_NE(e3, e4);
    ASSERT_EQ(e4, e5);
}

// Verifies NaN classification for every value in the original generated
// single-precision operation set.
// Original source: Ghidra/Features/Decompiler/src/decompile/unittests/testfloatemu.cc::float_opNan.
TEST(FloatFormat, FloatOpNan) {
    ghidra::FloatFormat format(4);
    for (const float value : float_test_values) {
        const ghidra::uintb true_result = std::isnan(value);
        const ghidra::uintb encoding = format.getEncoding(value);
        const ghidra::uintb result = format.opNan(encoding);
        ASSERT_EQ(true_result, result);
    }
}

// Verifies sign inversion, including signed zero, infinities, NaN, subnormal,
// and normal values from the original generated operation set.
// Original source: Ghidra/Features/Decompiler/src/decompile/unittests/testfloatemu.cc::float_opNeg.
TEST(FloatFormat, FloatOpNeg) {
    ghidra::FloatFormat format(4);
    for (const float value : float_test_values) {
        const ghidra::uintb true_result = float_to_raw_bits(-value);
        const ghidra::uintb encoding = format.getEncoding(value);
        const ghidra::uintb result = format.opNeg(encoding);
        ASSERT_EQ(true_result, result);
    }
}

// Verifies absolute value over the complete original generated operation set.
// Original source: Ghidra/Features/Decompiler/src/decompile/unittests/testfloatemu.cc::float_opAbs.
TEST(FloatFormat, FloatOpAbs) {
    ghidra::FloatFormat format(4);
    for (const float value : float_test_values) {
        const ghidra::uintb true_result = float_to_raw_bits(std::abs(value));
        const ghidra::uintb encoding = format.getEncoding(value);
        const ghidra::uintb result = format.opAbs(encoding);
        ASSERT_EQ(true_result, result);
    }
}

// Verifies square-root emulation against host single-precision behavior for
// every original generated value, including invalid negative inputs.
// Original source: Ghidra/Features/Decompiler/src/decompile/unittests/testfloatemu.cc::float_opSqrt.
TEST(FloatFormat, FloatOpSqrt) {
    ghidra::FloatFormat format(4);
    for (const float value : float_test_values) {
        const ghidra::uintb true_result = float_to_raw_bits(std::sqrt(value));
        const ghidra::uintb encoding = format.getEncoding(value);
        const ghidra::uintb result = format.opSqrt(encoding);
        ASSERT_EQ(true_result, result);
    }
}

// Verifies ceiling emulation and exact encoded results over all original
// generated values, including signed zero and infinities.
// Original source: Ghidra/Features/Decompiler/src/decompile/unittests/testfloatemu.cc::float_opCeil.
TEST(FloatFormat, FloatOpCeil) {
    ghidra::FloatFormat format(4);
    for (const float value : float_test_values) {
        const ghidra::uintb true_result = float_to_raw_bits(std::ceil(value));
        const ghidra::uintb encoding = format.getEncoding(value);
        const ghidra::uintb result = format.opCeil(encoding);
        ASSERT_EQ(true_result, result);
    }
}

// Verifies floor emulation and exact encoded results over all original
// generated values, including signed zero and infinities.
// Original source: Ghidra/Features/Decompiler/src/decompile/unittests/testfloatemu.cc::float_opFloor.
TEST(FloatFormat, FloatOpFloor) {
    ghidra::FloatFormat format(4);
    for (const float value : float_test_values) {
        const ghidra::uintb true_result = float_to_raw_bits(std::floor(value));
        const ghidra::uintb encoding = format.getEncoding(value);
        const ghidra::uintb result = format.opFloor(encoding);
        ASSERT_EQ(true_result, result);
    }
}

// Verifies round-away-from-zero emulation against the host float operation for
// every original generated value.
// Original source: Ghidra/Features/Decompiler/src/decompile/unittests/testfloatemu.cc::float_opRound.
TEST(FloatFormat, FloatOpRound) {
    ghidra::FloatFormat format(4);
    for (const float value : float_test_values) {
        const ghidra::uintb true_result = float_to_raw_bits(std::round(value));
        const ghidra::uintb encoding = format.getEncoding(value);
        const ghidra::uintb result = format.opRound(encoding);
        ASSERT_EQ(true_result, result);
    }
}

// Verifies signed integer to four-byte floating-point conversion across all
// original representative and integer-limit inputs.
// Original source: Ghidra/Features/Decompiler/src/decompile/unittests/testfloatemu.cc::float_opInt2Float_size4.
TEST(FloatFormat, FloatOpInt2FloatSize4) {
    ghidra::FloatFormat format(4);
    for (const int value : int_test_values) {
        const ghidra::uintb true_result = float_to_raw_bits(static_cast<float>(value));
        const ghidra::uintb result = format.opInt2Float(value, 4);
        ASSERT_EQ(true_result, result);
    }
}

// Verifies conversion from every original single-precision value to an exact
// double-precision encoding.
// Original source: Ghidra/Features/Decompiler/src/decompile/unittests/testfloatemu.cc::float_to_double_opFloat2Float.
TEST(FloatFormat, FloatToDoubleOpFloat2Float) {
    ghidra::FloatFormat format(4);
    ghidra::FloatFormat format8(8);
    for (const float value : float_test_values) {
        const ghidra::uintb true_result = double_to_raw_bits(static_cast<double>(value));
        const ghidra::uintb encoding = format.getEncoding(value);
        const ghidra::uintb result = format.opFloat2Float(encoding, format8);
        ASSERT_EQ(true_result, result);
    }
}

// Verifies truncation to a four-byte signed integer while preserving the
// original test's guard against undefined behavior for out-of-range floats.
// Original source: Ghidra/Features/Decompiler/src/decompile/unittests/testfloatemu.cc::float_opTrunc_to_int.
TEST(FloatFormat, FloatOpTruncToInt) {
    ghidra::FloatFormat format(4);
    for (const float value : float_test_values) {
        // Avoid the undefined behavior that the original native test excludes.
        if (!std::isfinite(value) || static_cast<double>(value) > std::numeric_limits<int>::max() ||
            static_cast<double>(value) < std::numeric_limits<int>::min()) {
            continue;
        }
        const ghidra::uintb true_result = static_cast<ghidra::uintb>(static_cast<std::int32_t>(value)) & 0xffffffff;
        const ghidra::uintb encoding = format.getEncoding(value);
        const ghidra::uintb result = format.opTrunc(encoding, 4);
        ASSERT_EQ(true_result, result);
    }
}

// Verifies pairwise floating-point equality, including IEEE NaN and signed
// zero comparison semantics, over the complete original value set.
// Original source: Ghidra/Features/Decompiler/src/decompile/unittests/testfloatemu.cc::float_opEqual.
TEST(FloatFormat, FloatOpEqual) {
    ghidra::FloatFormat format(4);
    for (const float first : float_test_values) {
        const ghidra::uintb encoding1 = format.getEncoding(first);
        for (const float second : float_test_values) {
            const ghidra::uintb true_result = first == second;
            const ghidra::uintb encoding2 = format.getEncoding(second);
            const ghidra::uintb result = format.opEqual(encoding1, encoding2);
            ASSERT_EQ(true_result, result);
        }
    }
}

// Verifies pairwise floating-point inequality, including NaN and signed zero,
// over the complete original value set.
// Original source: Ghidra/Features/Decompiler/src/decompile/unittests/testfloatemu.cc::float_opNotEqual.
TEST(FloatFormat, FloatOpNotEqual) {
    ghidra::FloatFormat format(4);
    for (const float first : float_test_values) {
        const ghidra::uintb encoding1 = format.getEncoding(first);
        for (const float second : float_test_values) {
            const ghidra::uintb true_result = first != second;
            const ghidra::uintb encoding2 = format.getEncoding(second);
            const ghidra::uintb result = format.opNotEqual(encoding1, encoding2);
            ASSERT_EQ(true_result, result);
        }
    }
}

// Verifies pairwise ordered comparison semantics, where unordered NaN pairs
// must produce false, for the complete original value set.
// Original source: Ghidra/Features/Decompiler/src/decompile/unittests/testfloatemu.cc::float_opLess.
TEST(FloatFormat, FloatOpLess) {
    ghidra::FloatFormat format(4);
    for (const float first : float_test_values) {
        const ghidra::uintb encoding1 = format.getEncoding(first);
        for (const float second : float_test_values) {
            const ghidra::uintb true_result = first < second;
            const ghidra::uintb encoding2 = format.getEncoding(second);
            const ghidra::uintb result = format.opLess(encoding1, encoding2);
            ASSERT_EQ(true_result, result);
        }
    }
}

// Verifies pairwise less-than-or-equal semantics, including unordered NaN
// pairs and equality of signed zero, over the original value set.
// Original source: Ghidra/Features/Decompiler/src/decompile/unittests/testfloatemu.cc::float_opLessEqual.
TEST(FloatFormat, FloatOpLessEqual) {
    ghidra::FloatFormat format(4);
    for (const float first : float_test_values) {
        const ghidra::uintb encoding1 = format.getEncoding(first);
        for (const float second : float_test_values) {
            const ghidra::uintb true_result = first <= second;
            const ghidra::uintb encoding2 = format.getEncoding(second);
            const ghidra::uintb result = format.opLessEqual(encoding1, encoding2);
            ASSERT_EQ(true_result, result);
        }
    }
}

// Verifies pairwise addition over the original generated values, including
// signed zero, infinities, and NaN propagation.
// Original source: Ghidra/Features/Decompiler/src/decompile/unittests/testfloatemu.cc::float_opAdd.
TEST(FloatFormat, FloatOpAdd) {
    ghidra::FloatFormat format(4);
    for (const float first : float_test_values) {
        const ghidra::uintb encoding1 = format.getEncoding(first);
        for (const float second : float_test_values) {
            const ghidra::uintb true_result = float_to_raw_bits(first + second);
            const ghidra::uintb encoding2 = format.getEncoding(second);
            const ghidra::uintb result = format.opAdd(encoding1, encoding2);
            ASSERT_EQ(true_result, result);
        }
    }
}

// Verifies pairwise division over the original generated values, including
// zero divisors, infinities, and NaN propagation.
// Original source: Ghidra/Features/Decompiler/src/decompile/unittests/testfloatemu.cc::float_opDiv.
TEST(FloatFormat, FloatOpDiv) {
    ghidra::FloatFormat format(4);
    for (const float first : float_test_values) {
        const ghidra::uintb encoding1 = format.getEncoding(first);
        for (const float second : float_test_values) {
            const ghidra::uintb true_result = float_to_raw_bits(first / second);
            const ghidra::uintb encoding2 = format.getEncoding(second);
            const ghidra::uintb result = format.opDiv(encoding1, encoding2);
            ASSERT_EQ(true_result, result);
        }
    }
}

// Verifies pairwise multiplication over the original generated values,
// including signed zero, infinities, and NaN propagation.
// Original source: Ghidra/Features/Decompiler/src/decompile/unittests/testfloatemu.cc::float_opMult.
TEST(FloatFormat, FloatOpMult) {
    ghidra::FloatFormat format(4);
    for (const float first : float_test_values) {
        const ghidra::uintb encoding1 = format.getEncoding(first);
        for (const float second : float_test_values) {
            const ghidra::uintb true_result = float_to_raw_bits(first * second);
            const ghidra::uintb encoding2 = format.getEncoding(second);
            const ghidra::uintb result = format.opMult(encoding1, encoding2);
            ASSERT_EQ(true_result, result);
        }
    }
}

// Verifies pairwise subtraction over the original generated values, including
// signed zero, infinities, and NaN propagation.
// Original source: Ghidra/Features/Decompiler/src/decompile/unittests/testfloatemu.cc::float_opSub.
TEST(FloatFormat, FloatOpSub) {
    ghidra::FloatFormat format(4);
    for (const float first : float_test_values) {
        const ghidra::uintb encoding1 = format.getEncoding(first);
        for (const float second : float_test_values) {
            const ghidra::uintb true_result = float_to_raw_bits(first - second);
            const ghidra::uintb encoding2 = format.getEncoding(second);
            const ghidra::uintb result = format.opSub(encoding1, encoding2);
            ASSERT_EQ(true_result, result);
        }
    }
}

} // namespace ghidra::native_scalar_tests
