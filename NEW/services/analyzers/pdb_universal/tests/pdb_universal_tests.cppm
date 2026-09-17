module;

#include <gtest/gtest.h>

export module analyzer_pdb_universal_tests;

import analyzer_pdb_universal;
import analyzer_test_support;
import std;

namespace {

/// Returns the checked-in PDB fixture used to assert raw MSF and CodeView behavior.
[[nodiscard]] std::filesystem::path fixture_path() {
    return std::filesystem::path(ANALYZER_FIXTURE_DIR) / "pdb_universal" / "tests" / "data" / "test_pdb_universal.pdb";
}

} // namespace

/// Verifies identity, named records, and procedure symbols from the real fixture.
TEST(PdbUniversal, ParsesSupportedMsfCodeViewRecords) {
    const auto parsed = recode::pdb::universal::PdbReader::parse(fixture_path());
    ASSERT_TRUE(parsed) << (parsed ? "" : parsed.error().message);
    EXPECT_EQ(parsed->identity.age, 12U);
    EXPECT_EQ(parsed->identity.guid_string(), "4d1a0d5b-567a-67c8-83ba-d3f22c4b9fbf");
    const auto record = std::find_if(parsed->types.begin(), parsed->types.end(),
                                     [](const auto& type) { return type.name == "PdbUniversalRecord"; });
    ASSERT_NE(record, parsed->types.end());
    EXPECT_EQ(record->kind, "struct");
    EXPECT_EQ(record->size, 24U);
    EXPECT_EQ(parsed->type_name(record->index), "PdbUniversalRecord");
    ASSERT_EQ(record->fields.size(), 3U);
    EXPECT_EQ(record->fields[0].first, "identifier");
    EXPECT_EQ(record->fields[1].first, "quantity");
    EXPECT_EQ(record->fields[2].first, "label");
    const auto function = std::find_if(parsed->symbols.begin(), parsed->symbols.end(),
                                       [](const auto& symbol) { return symbol.name == "pdb_universal_compute"; });
    ASSERT_NE(function, parsed->symbols.end());
    EXPECT_TRUE(function->function);
    EXPECT_EQ(function->section, 1U);
    EXPECT_EQ(function->offset, 0U);
    ASSERT_TRUE(parsed->procedure_signature(function->type_index));
    EXPECT_EQ(parsed->procedure_signature(function->type_index)->second.size(), 2U);
}

/// Verifies that malformed input fails with a diagnostic instead of being treated as a name-only PDB.
TEST(PdbUniversal, RejectsNonPdbInput) {
    const auto parsed = recode::pdb::universal::PdbReader::parse(
        std::filesystem::path(ANALYZER_FIXTURE_DIR) / "pdb_universal" / "tests" / "data" / "test_pdb_universal.cpp");
    ASSERT_FALSE(parsed);
    EXPECT_NE(parsed.error().message.find("not a Microsoft PDB"), std::string::npos);
}

/// Verifies application of types, functions, names, and the identity bookmark to the native model.
TEST(PdbUniversal, AppliesRecordsToAnalysisContext) {
    auto context = recode::analyzer::tests::load_fixture("pdb_universal");
    context.options().pdb_universal = true;
    context.options().pdb_path = fixture_path();
    recode::analyzer::PdbUniversalAnalyzer analyzer;
    recode::analyzer::CancellationToken cancellation;
    analyzer.analyze(context, {}, cancellation);
    const auto* function = context.function_at(0x140001000ULL);
    ASSERT_NE(function, nullptr);
    EXPECT_EQ(function->name, "pdb_universal_compute");
    EXPECT_EQ(function->return_type, "__int64");
    ASSERT_EQ(function->parameters.size(), 2U);
    EXPECT_EQ(function->parameters[0].type, "PdbUniversalRecord *");
    EXPECT_EQ(function->parameters[1].type, "PdbUniversalMode");
    EXPECT_FALSE(context.pdb_types().empty());
    ASSERT_FALSE(context.bookmarks().empty());
    EXPECT_NE(context.bookmarks().back().comment.find("PDB Loaded: 4d1a0d5b"), std::string::npos);
}
