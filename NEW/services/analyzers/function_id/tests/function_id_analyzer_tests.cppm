module;

#include <gtest/gtest.h>

export module function_id_analyzer_tests;

import analyzer_function_id;
import analyzer_test_support;
import std;

namespace recode::analyzer::tests {
namespace {

/// Verifies a real checked-in packed FID database labels the byte-identical fixture function.
TEST(FunctionIdAnalyzerTest, AppliesRealDatabaseMatch) {
    auto context = load_fixture("function_id");
    context.options().seed_provider_functions = false;
    context.options().fid_database_paths = {std::filesystem::path(ANALYZER_FIXTURE_DIR) / "function_id" / "tests" /
                                            "data" / "test_function_id.fidb"};
    for (const auto& exported : context.image().exported_symbols()) {
        if (!exported.forwarded && context.image().is_executable(exported.address_va)) {
            static_cast<void>(context.disassemble_flow(exported.address_va));
            static_cast<void>(context.create_function(exported.address_va));
        }
    }
    context.options().function_id_minimum_instructions = 4;
    CancellationToken cancellation;
    FunctionIdAnalyzer analyzer;
    analyzer.analyze(context, {}, cancellation);

    ASSERT_NE(context.function_at(0x140001000ULL), nullptr);
    EXPECT_EQ(context.function_at(0x140001000ULL)->name, "known_library_function");
    EXPECT_TRUE(std::any_of(context.bookmarks().begin(), context.bookmarks().end(), [](const Bookmark& bookmark) {
        return bookmark.address == 0x140001000ULL && bookmark.category == "Function ID Analyzer";
    }));
}

/// Verifies the analyzer does not mutate state when the feature option is disabled.
TEST(FunctionIdAnalyzerTest, HonorsDisableOption) {
    auto context = load_fixture("function_id");
    context.options().function_id = false;
    CancellationToken cancellation;
    FunctionIdAnalyzer analyzer;
    analyzer.analyze(context, {}, cancellation);
    EXPECT_TRUE(context.symbols().empty());
    EXPECT_TRUE(context.bookmarks().empty());
}

} // namespace
} // namespace recode::analyzer::tests
