module;

#include <gtest/gtest.h>

export module ascii_strings_tests;

import analyzer_ascii_strings;
import analyzer_test_support;
import std;

namespace recode::analyzer::tests {
namespace {

/// Verifies the checked-in ASCII fixture creates both terminated strings and preserves negatives.
TEST(AsciiStringsAnalyzerTest, CreatesExpectedTerminatedStrings) {
    auto context = load_fixture("ascii_strings");
    CancellationToken cancellation;
    AsciiStringsAnalyzer analyzer;
    analyzer.analyze(context, {}, cancellation);

    ASSERT_EQ(context.strings().size(), 2U);
    EXPECT_EQ(context.strings()[0].address, 0x140002000ULL);
    EXPECT_EQ(context.strings()[0].size, 44U);
    EXPECT_EQ(context.strings()[0].value, "The quick brown fox jumps over the lazy dog");
    EXPECT_EQ(context.strings()[1].address, 0x140002030ULL);
    // The default four-byte end alignment consumes the three zero padding bytes after the terminator.
    EXPECT_EQ(context.strings()[1].size, 56U);
    EXPECT_EQ(context.strings()[1].value, "Ghidra ASCII string analysis discovers readable data");
    EXPECT_FALSE(std::any_of(context.strings().begin(), context.strings().end(),
                             [](const StringRecord& item) { return item.address == 0x14000202cULL; }));
}

/// Verifies the public contract retains the original option defaults and late priority.
TEST(AsciiStringsAnalyzerTest, PreservesDefaultOptionsAndPriority) {
    const auto descriptor = AsciiStringsAnalyzer{}.descriptor();
    EXPECT_EQ(descriptor.name, "ASCII Strings");
    EXPECT_EQ(descriptor.priority, 905);
    EXPECT_TRUE(AnalysisOptions{}.ascii_strings);
    EXPECT_EQ(AnalysisOptions{}.ascii_minimum_length, 5U);
    EXPECT_TRUE(AnalysisOptions{}.ascii_require_null_termination);
}

} // namespace
} // namespace recode::analyzer::tests
