module;

#include <gtest/gtest.h>

export module create_address_tables_tests;

import analyzer_create_address_tables;
import analyzer_test_support;
import std;

namespace recode::analyzer::tests {
namespace {

/// Verifies the four-entry PE32+ fixture table and its Address Table bookmark.
TEST(CreateAddressTablesIntegrationTest, RecognizesFixturePointerRun) {
    auto context = load_fixture("create_address_tables");
    context.options().create_address_tables = true;
    context.options().address_table_minimum_entries = 2U;
    context.options().address_table_alignment = 8U;
    context.options().address_table_pointer_alignment = 1U;
    context.options().address_table_relocation_guide = true;

    AutoAnalysisManager manager(context);
    manager.register_analyzer(std::make_unique<CreateAddressTablesAnalyzer>());
    ASSERT_TRUE(manager.analyze().completed);

    ASSERT_EQ(context.address_tables().size(), 1U);
    const auto& table = context.address_tables().front();
    EXPECT_EQ(table.address, 0x140002038ULL);
    EXPECT_EQ(table.entry_size, 8U);
    EXPECT_EQ(table.targets.size(), 4U);
    for (std::size_t index = 0; index < table.targets.size(); ++index) {
        const auto cell = table.address + index * table.entry_size;
        ASSERT_TRUE(context.data().contains(cell));
        EXPECT_EQ(context.data().at(cell).size, table.entry_size);
        EXPECT_EQ(context.data().at(cell).type, "pointer");
    }
    ASSERT_EQ(std::count_if(context.bookmarks().begin(), context.bookmarks().end(),
                            [](const Bookmark& bookmark) { return bookmark.category == "Address Table"; }),
              1U);
    EXPECT_TRUE(std::any_of(context.bookmarks().begin(), context.bookmarks().end(), [&](const Bookmark& bookmark) {
        return bookmark.address == table.address && bookmark.comment == "Address table[4] created";
    }));
}

/// Verifies that the exported non-pointer sentinel is not consumed as a fifth table entry.
TEST(CreateAddressTablesIntegrationTest, StopsAtSentinel) {
    auto context = load_fixture("create_address_tables");
    context.options().create_address_tables = true;
    context.options().address_table_minimum_entries = 2U;
    context.options().address_table_alignment = 8U;
    context.options().address_table_relocation_guide = true;

    AutoAnalysisManager manager(context);
    manager.register_analyzer(std::make_unique<CreateAddressTablesAnalyzer>());
    ASSERT_TRUE(manager.analyze().completed);
    ASSERT_EQ(context.address_tables().size(), 1U);
    EXPECT_EQ(context.address_tables().front().targets.size(), 4U);
    EXPECT_FALSE(context.data().contains(0x140002058ULL));
}

} // namespace
} // namespace recode::analyzer::tests
