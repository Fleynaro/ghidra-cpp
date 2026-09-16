module;

#include <gtest/gtest.h>

export module ghidra.services.tests;

import ghidra.core;
import ghidra.service.pe_loader;
import ghidra.service.sleigh;
import ghidra.runtime.workers.pool;
import std;

namespace ghidra::services::tests {
namespace {

/// Resolves the checked PE fixture used to prove the service boundary parses real input.
[[nodiscard]] std::filesystem::path pe_fixture() {
    return std::filesystem::path(NEW_GHIDRA_SERVICE_FIXTURE_DIR) / "services" / "pe_loader" / "tests" / "data" /
           "test.exe";
}

/// Verifies that PE parsing returns core regions and architecture metadata without exposing parser state.
TEST(ServiceBoundaryTest, PeLoaderProducesCoreImageFacts) {
    core::BinaryArtifact artifact;
    artifact.id = core::ArtifactId{"fixture"};
    artifact.locator = pe_fixture().string();
    artifact.display_name = "test.exe";
    artifact.format = "PE";
    artifact.primary = true;
    pe_loader::PeLoaderService service;
    const auto result = service.load(artifact, {});
    ASSERT_TRUE(result) << (result ? "" : result.error().message);
    EXPECT_FALSE(result->memory_regions.empty());
    EXPECT_FALSE(result->architecture.architecture_id.empty());
}

/// Verifies that a compiled SLA resource decodes an actual machine-code byte window into core values.
TEST(ServiceBoundaryTest, SleighServiceProducesCanonicalInstruction) {
    auto pool =
        std::make_shared<ghidra::runtime::workers::WorkerPool>(ghidra::runtime::workers::WorkerPoolConfig{1, 8});
    const auto path =
        std::filesystem::path(NEW_GHIDRA_SERVICE_FIXTURE_DIR) / "services" / "sleigh" / "specifications" / "x86-64.sla";
    auto service = sleigh::SleighService::open(path, pool);
    ASSERT_TRUE(service) << (service ? "" : service.error().message);
    core::contracts::DecodeRequest request;
    request.address = core::Address{core::AddressSpaceId{"ram"}, 0x1000};
    request.bytes = core::Bytes{std::vector<core::Byte>{0xc3}};
    const auto decoded = (*service)->decode(request);
    ASSERT_TRUE(decoded) << (decoded ? "" : decoded.error().message);
    EXPECT_EQ(decoded->length, 1U);
    EXPECT_FALSE(decoded->mnemonic.empty());
    EXPECT_FALSE(decoded->pcode.operations.empty());
}

} // namespace
} // namespace ghidra::services::tests
