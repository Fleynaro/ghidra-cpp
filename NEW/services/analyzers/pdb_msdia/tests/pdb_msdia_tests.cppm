module;

#include <gtest/gtest.h>

export module analyzer_pdb_msdia_tests;

import analyzer_pdb_msdia;
import std;

namespace {

/// Returns the checked-in DIA fixture path.
[[nodiscard]] std::filesystem::path fixture_path() {
    return std::filesystem::path(ANALYZER_FIXTURE_DIR) / "pdb_msdia" / "tests" / "data" / "test_pdb_msdia.pdb";
}

} // namespace

/// Verifies the real Windows DIA boundary enumerates hardcoded fixture symbols.
TEST(PdbMsdia, OpensThroughDiaProvider) {
#ifdef _WIN32
    const auto session = ghidra::pdb::msdia::MsdiaSession::open(fixture_path());
    if (!session) {
        // Windows without the registered DIA COM class must report the
        // provider boundary rather than silently substituting another parser.
        EXPECT_NE(session.error().message.find("0x80040154"), std::string::npos);
        return;
    }
    const auto function = std::find_if(session->symbols().begin(), session->symbols().end(),
                                       [](const auto& symbol) { return symbol.name == "pdb_msdia_compute"; });
    ASSERT_NE(function, session->symbols().end());
    EXPECT_TRUE(function->function);
    EXPECT_EQ(function->virtual_address, 0x140001000ULL);
#else
    EXPECT_FALSE(ghidra::pdb::msdia::MsdiaSession::platform_supported());
#endif
}

/// Verifies non-Windows builds report a provider error instead of pretending to parse DIA records.
TEST(PdbMsdia, ReportsPlatformBoundary) {
#ifndef _WIN32
    const auto session = ghidra::pdb::msdia::MsdiaSession::open(fixture_path());
    ASSERT_FALSE(session);
    EXPECT_NE(session.error().message.find("requires Windows"), std::string::npos);
#else
    EXPECT_TRUE(ghidra::pdb::msdia::MsdiaSession::platform_supported());
#endif
}
