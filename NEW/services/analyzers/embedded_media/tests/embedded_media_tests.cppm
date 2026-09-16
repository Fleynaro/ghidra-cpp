module;

#include <gtest/gtest.h>

export module embedded_media_tests;

import analyzer_embedded_media;
import analyzer_test_support;
import std;

namespace ghidra::analyzer::tests {
namespace {

/// Verifies every positive media container in the checked-in fixture and the default bookmark contract.
TEST(EmbeddedMediaIntegrationTest, MatchesAllSupportedFixtureMedia) {
    auto context = load_fixture("embedded_media");
    context.options().embedded_media = true;
    context.options().create_analysis_bookmarks = true;

    AutoAnalysisManager manager(context);
    manager.register_analyzer(std::make_unique<EmbeddedMediaAnalyzer>());
    const auto result = manager.analyze();
    ASSERT_TRUE(result.completed);

    const std::map<Address, std::pair<std::string, std::uint32_t>> expected{
        {0x140002040ULL, {"GIF-Image", 35U}},  {0x140002068ULL, {"GIF-Image", 35U}},
        {0x140002090ULL, {"PNG-Image", 68U}},  {0x1400020E0ULL, {"JPEG-Image", 1345U}},
        {0x140002628ULL, {"WAVE-Sound", 44U}}, {0x140002658ULL, {"MIDI-Score", 26U}},
        {0x140002678ULL, {"AU-Sound", 24U}},
    };
    ASSERT_EQ(context.embedded_media().size(), expected.size());
    ASSERT_EQ(std::count_if(context.bookmarks().begin(), context.bookmarks().end(),
                            [](const Bookmark& bookmark) { return bookmark.category == "Embedded Media"; }),
              expected.size());
    for (const auto& media : context.embedded_media()) {
        const auto expected_media = expected.find(media.address);
        ASSERT_NE(expected_media, expected.end());
        EXPECT_EQ(media.type, expected_media->second.first);
        EXPECT_EQ(media.size, expected_media->second.second);
        EXPECT_TRUE(media.validated);
        EXPECT_TRUE(std::any_of(context.bookmarks().begin(), context.bookmarks().end(), [&](const Bookmark& bookmark) {
            return bookmark.address == media.address && bookmark.category == "Embedded Media";
        }));
    }
}

/// Verifies that an already defined listing object prevents a second media application at the same address.
TEST(EmbeddedMediaIntegrationTest, DoesNotOverwriteExistingData) {
    auto context = load_fixture("embedded_media");
    ASSERT_TRUE(context.add_data(DataObject{0x140002040ULL, 35U, "existing"}));
    AutoAnalysisManager manager(context);
    manager.register_analyzer(std::make_unique<EmbeddedMediaAnalyzer>());
    ASSERT_TRUE(manager.analyze().completed);
    EXPECT_EQ(std::count_if(context.embedded_media().begin(), context.embedded_media().end(),
                            [](const EmbeddedMediaRecord& media) { return media.address == 0x140002040ULL; }),
              0U);
}

} // namespace
} // namespace ghidra::analyzer::tests
