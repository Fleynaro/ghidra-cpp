module;

#include <gtest/gtest.h>

export module bsim_similarity_integration_tests;

import decompiler;
import pe_loader;
import recode.decompiler;
import recode.core;
import recode.service.bsim;
import sleigh_runtime;
import std;

namespace {

/// Bridges the loaded PE image into the decompiler's immutable memory contract.
class PeMemoryProvider final : public recode::decompiler::MemoryProvider {
public:
    /// Keeps the parsed image alive for every Sleigh/decompiler read.
    explicit PeMemoryProvider(std::shared_ptr<const pe::LoadedPeImage> image) : image_(std::move(image)) {}

    /// Reads exactly the requested virtual image range and preserves loader diagnostics.
    [[nodiscard]] std::expected<std::vector<std::uint8_t>, recode::decompiler::ProviderError>
    read(std::uint64_t address, std::size_t size) const override {
        const auto bytes = image_->read_memory(address, size);
        if (!bytes)
            return std::unexpected(recode::decompiler::ProviderError{bytes.error().message});
        return *bytes;
    }

private:
    std::shared_ptr<const pe::LoadedPeImage> image_;
};

/// Creates a stable value identity for one exported fixture function.
[[nodiscard]] recode::core::FunctionSnapshot snapshot_for(std::string name, std::uint64_t address) {
    recode::core::FunctionSnapshot snapshot;
    snapshot.key = {recode::core::EntityId{"bsim-fixture-" + name},
                    recode::core::Address{recode::core::AddressSpaceId{"ram"}, address}};
    snapshot.name = std::move(name);
    return snapshot;
}

/// Finds one named exported fixture address without relying on PE function order.
[[nodiscard]] std::optional<std::uint64_t> exported_address(const pe::LoadedPeImage& image, std::string_view name) {
    for (const auto& symbol : image.exported_symbols())
        if (symbol.name && *symbol.name == name)
            return symbol.address_va;
    return std::nullopt;
}

/// Returns the PE exception-table end address for one exported function.
[[nodiscard]] std::optional<std::uint64_t> runtime_end(const pe::LoadedPeImage& image, std::uint64_t address) {
    for (const auto& entry : image.exception_functions())
        if (entry.begin_va == address)
            return entry.end_va;
    return std::nullopt;
}

/// Decompiles every named fixture export through PE, Sleigh, native Decompiler, and BSim.
[[nodiscard]] std::map<std::string, recode::core::contracts::FunctionSimilarityResult>
analyze_fixture(const std::shared_ptr<const pe::LoadedPeImage>& image, const std::vector<std::string>& names) {
    auto memory = std::make_shared<PeMemoryProvider>(image);
    auto pcode = std::make_shared<recode::decompiler::SleighPcodeProvider>(
        sleigh_runtime::resolve_sla_path("x86-64.sla"), memory,
        std::vector<std::pair<std::string, std::uint64_t>>{
            {"addrsize", 2}, {"opsize", 1}, {"rexprefix", 0}, {"longMode", 1}});
    recode::decompiler::ProviderContext providers;
    providers.pcode = pcode;
    providers.memory = memory;
    recode::decompiler::Decompiler decompiler(recode::decompiler::make_x86_64_architecture(), std::move(providers));
    recode::services::bsim::BsimService service;
    std::map<std::string, recode::core::contracts::FunctionSimilarityResult> results;
    for (const auto& name : names) {
        const auto address = exported_address(*image, name);
        if (!address)
            throw std::runtime_error("Fixture export is missing: " + name);
        const auto end = runtime_end(*image, *address).value_or(address.value() + 0x200);
        recode::decompiler::DecompilationResult native;
        try {
            native = decompiler.decompile(
                recode::decompiler::FunctionDescription{name, *address, end, false, true, 0x49, 3, 1, 0});
        } catch (const ghidra::LowlevelError& error) {
            throw std::runtime_error("Native decompiler threw while analyzing fixture export " + name + ": " +
                                     error.explain);
        } catch (const std::exception& error) {
            throw std::runtime_error("Native decompiler threw while analyzing fixture export " + name + ": " +
                                     error.what());
        } catch (...) {
            throw std::runtime_error("Native decompiler threw while analyzing fixture export: " + name);
        }
        auto result = service.analyze_decompiler(snapshot_for(name, *address), native);
        if (!result)
            throw std::runtime_error("BSim analysis failed for " + name + ": " + result.error().message);
        std::cout << name << " features=" << result->features.hashes.size()
                  << " vector_entries=" << result->vector.entries.size() << " vector_length=" << result->vector.length
                  << '\n';
        results.emplace(name, std::move(*result));
    }
    return results;
}

/// Compares a named pair and returns its cosine while retaining diagnostic output.
[[nodiscard]] double cosine(const std::map<std::string, recode::core::contracts::FunctionSimilarityResult>& results,
                            const recode::services::bsim::BsimService& service, std::string_view first,
                            std::string_view second) {
    const auto comparison =
        service.compare(results.at(std::string(first)).vector, results.at(std::string(second)).vector);
    if (!comparison)
        throw std::runtime_error("BSim comparison failed: " + comparison.error().message);
    return comparison->cosine;
}

} // namespace

/// Verifies semantic twin families outrank an unrelated fixture utility after the real analysis pipeline.
TEST(BsimSimilarityIntegration, SemanticTwinsRankAboveUnrelatedFunctions) {
    auto loaded = pe::PeLoader::load_file(BSIM_FIXTURE_PATH);
    ASSERT_TRUE(loaded.has_value()) << loaded.error().message;
    auto image = std::make_shared<pe::LoadedPeImage>(std::move(*loaded));
    const std::vector<std::string> names{"sort_insertion_ascending",
                                         "sort_selection_ascending",
                                         "find_first_linear",
                                         "find_first_reverse",
                                         "maximum_scan",
                                         "maximum_pairwise",
                                         "absolute_branch",
                                         "absolute_mask",
                                         "checksum_forward",
                                         "checksum_reverse",
                                         "rotate_left32",
                                         "population_count"};
    std::map<std::string, recode::core::contracts::FunctionSimilarityResult> results;
    try {
        results = analyze_fixture(image, names);
    } catch (const std::exception& error) {
        FAIL() << error.what();
    }
    ASSERT_EQ(results.size(), names.size());
    recode::services::bsim::BsimService service;

    const std::vector<std::pair<std::string, std::string>> twin_pairs{
        {"sort_insertion_ascending", "sort_selection_ascending"},
        {"find_first_linear", "find_first_reverse"},
        {"maximum_scan", "maximum_pairwise"},
        {"absolute_branch", "absolute_mask"},
        {"checksum_forward", "checksum_reverse"}};
    for (const auto& [first, second] : twin_pairs) {
        const auto twin_score = cosine(results, service, first, second);
        const auto unrelated_score = cosine(results, service, first, "population_count");
        std::cout << first << " vs " << second << " cosine=" << twin_score
                  << "; vs population_count=" << unrelated_score << '\n';
        SCOPED_TRACE(first + " vs " + second);
        EXPECT_GT(twin_score, unrelated_score);
    }
}
