module;

#include <gtest/gtest.h>

export module analyzer_variadic_function_signature_override_tests;

import analyzer_variadic_function_signature_override;
import std;

/// Verifies printf conversions, escaped percent signs, length modifiers, and pointer arguments.
TEST(VariadicSignatureOverride, ParsesPrintfArguments) {
    const auto parsed = ghidra::analyzer::parse_format_string("value=%d name=%s ratio=%0.2f ptr=%p %%", false);
    ASSERT_TRUE(parsed);
    ASSERT_EQ(parsed->size(), 4U);
    EXPECT_EQ((*parsed)[0].type, "int");
    EXPECT_EQ((*parsed)[1].type, "char *");
    EXPECT_EQ((*parsed)[2].type, "double");
    EXPECT_EQ((*parsed)[3].type, "void *");
    EXPECT_FALSE((*parsed)[0].indirect);
}

/// Verifies scanf suppression and output-pointer semantics are not confused with printf inputs.
TEST(VariadicSignatureOverride, ParsesScanfOutputArguments) {
    const auto parsed = ghidra::analyzer::parse_format_string("%*d %u %lf %s %n", true);
    ASSERT_TRUE(parsed);
    ASSERT_EQ(parsed->size(), 4U);
    EXPECT_EQ((*parsed)[0].type, "unsigned int *");
    EXPECT_EQ((*parsed)[1].type, "double *");
    EXPECT_EQ((*parsed)[2].type, "char *");
    EXPECT_EQ((*parsed)[3].type, "int *");
    EXPECT_TRUE((*parsed)[0].indirect);
}

/// Verifies wide-character conversions and scanf pointer-to-pointer semantics.
TEST(VariadicSignatureOverride, ParsesWideAndPointerConversions) {
    const auto parsed = ghidra::analyzer::parse_format_string("%ls %lc %p", true);
    ASSERT_TRUE(parsed);
    ASSERT_EQ(parsed->size(), 3U);
    EXPECT_EQ((*parsed)[0].type, "wchar_t *");
    EXPECT_EQ((*parsed)[1].type, "wint_t *");
    EXPECT_EQ((*parsed)[2].type, "void **");
}

/// Verifies malformed conversions are rejected instead of producing a partial call-site signature.
TEST(VariadicSignatureOverride, RejectsMalformedFormat) {
    const auto parsed = ghidra::analyzer::parse_format_string("bad=%", false);
    ASSERT_FALSE(parsed);
    EXPECT_NE(parsed.error().message.find("ends after"), std::string::npos);
}
