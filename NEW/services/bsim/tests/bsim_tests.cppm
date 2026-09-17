#include <gtest/gtest.h>

import std;
import recode.core.contracts.bsim;
import recode.service.bsim;
import recode.service.bsim.cosine_vector;
import recode.service.bsim.idf;
import recode.service.bsim.vector_factory;
import recode.service.bsim.weight_factory;

namespace {

/// Builds a sorted value-only signature result for vector and contract tests.
[[nodiscard]] recode::core::contracts::FunctionSimilarityFeatures synthetic_features() {
    recode::core::contracts::FunctionSimilarityFeatures features;
    features.hashes = {0x10U, 0x10U, 0x20U, 0x80U};
    features.settings = 0x49U;
    features.overall_hash = 0x1234U;
    return features;
}

/// Verifies the fixed-width empty-vector unique hash from LSHCosineVector.java.
TEST(BsimVector, EmptyUniqueHashUsesGhidraSeed) {
    const recode::services::bsim::CosineVector vector;
    EXPECT_EQ(vector.calc_unique_hash(), 0x12cf93abee39b2d6ULL);
}

/// Verifies duplicate feature runs become one sparse entry while preserving public TF.
TEST(BsimVector, DuplicateFeaturesPreserveTermFrequency) {
    recode::services::bsim::WeightFactory weights;
    weights.set_idf_weight(0, 1.0);
    weights.set_logarithmic_tf_weights();
    recode::services::bsim::IDFLookup lookup;
    recode::services::bsim::VectorFactory factory;
    factory.set(weights, lookup, 0x49U);
    const std::array features{1U, 1U, 1U, 2U};
    const auto vector = factory.build_vector(features);
    ASSERT_EQ(vector.entries.size(), 2U);
    EXPECT_EQ(vector.entries[0].term_frequency, 3U);
    EXPECT_EQ(vector.entries[1].term_frequency, 1U);
    EXPECT_EQ(vector.hash_count, 4U);
}

/// Verifies the lower-TF coefficient-square cosine rule on identical weighted vectors.
TEST(BsimVector, IdenticalVectorsHaveUnitCosine) {
    recode::services::bsim::WeightFactory weights;
    weights.set_idf_weight(0, 1.0);
    weights.set_logarithmic_tf_weights();
    recode::services::bsim::IDFLookup lookup;
    recode::services::bsim::VectorFactory factory;
    factory.set(weights, lookup, 0x49U);
    const std::array features{0x10U, 0x20U, 0x20U};
    const auto vector = factory.build_vector(features);
    const auto comparison = factory.compare(vector, vector);
    EXPECT_DOUBLE_EQ(comparison.cosine, 1.0);
    EXPECT_EQ(comparison.intersect_count, vector.hash_count);
}

/// Verifies disjoint sparse hashes produce zero cosine and no intersection.
TEST(BsimVector, DisjointVectorsAreOrthogonal) {
    recode::services::bsim::WeightFactory weights;
    weights.set_idf_weight(0, 1.0);
    weights.set_logarithmic_tf_weights();
    recode::services::bsim::IDFLookup lookup;
    recode::services::bsim::VectorFactory factory;
    factory.set(weights, lookup, 0x49U);
    const auto first = factory.build_vector(std::array{1U});
    const auto second = factory.build_vector(std::array{2U});
    const auto comparison = factory.compare(first, second);
    EXPECT_DOUBLE_EQ(comparison.cosine, 0.0);
    EXPECT_EQ(comparison.intersect_count, 0U);
}

/// Verifies Java's six-bit internal TF slot clamps large duplicate runs at public TF 64.
TEST(BsimVector, TermFrequencyClampsAtSixtyFour) {
    recode::services::bsim::WeightFactory weights;
    weights.set_idf_weight(0, 1.0);
    weights.set_logarithmic_tf_weights();
    recode::services::bsim::IDFLookup lookup;
    recode::services::bsim::VectorFactory factory;
    factory.set(weights, lookup, 0x49U);
    std::array<std::uint32_t, 80> features{};
    features.fill(0x1234U);
    const auto vector = factory.build_vector(features);
    ASSERT_EQ(vector.entries.size(), 1U);
    EXPECT_EQ(vector.entries.front().term_frequency, 64U);
}

/// Verifies accumulator duplicate rejection preserves the first explicit coefficient and finalization state.
TEST(BsimVector, AccumulatorFinalizesOnce) {
    recode::services::bsim::CosineVectorAccumulator accumulator;
    accumulator.add_hash(7U, 2.0);
    accumulator.add_hash(7U, 9.0);
    EXPECT_EQ(accumulator.num_entries(), 1U);
    (void)accumulator.do_finalize();
    EXPECT_THROW(accumulator.add_hash(8U, 1.0), std::runtime_error);
    EXPECT_THROW((void)accumulator.num_entries(), std::logic_error);
}

/// Verifies the Decompiler-produced feature value is deterministic and remains sorted.
TEST(BsimFeatures, DeterministicCanonicalFeatures) {
    const auto features = synthetic_features();
    recode::services::bsim::BsimService service;
    const auto first = service.generate_signature(features);
    const auto second = service.generate_signature(features);
    ASSERT_TRUE(first.has_value());
    ASSERT_TRUE(second.has_value());
    EXPECT_EQ(first->hashes, second->hashes);
    EXPECT_EQ(first->overall_hash, second->overall_hash);
    EXPECT_TRUE(std::ranges::is_sorted(first->hashes));
    EXPECT_FALSE(first->hashes.empty());
}

/// Verifies feature settings incompatible with the loaded vector resource are rejected.
TEST(BsimFeatures, InvalidSettingsReturnAnError) {
    recode::services::bsim::BsimService service;
    auto features = synthetic_features();
    features.settings = 0x4dU;
    const auto result = service.generate_signature(features);
    EXPECT_FALSE(result.has_value());
}

/// Verifies an empty Decompiler feature result is a valid deterministic zero-feature input.
TEST(BsimFeatures, EmptyFunctionProducesEmptyFeatures) {
    recode::services::bsim::BsimService service;
    const auto result = service.generate_signature({});
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->hashes.empty());
    EXPECT_EQ(result->overall_hash, 0U);
}

/// Verifies that one service call returns both raw duplicate-preserving features and a vector.
TEST(BsimService, AnalyzeReturnsBothStages) {
    recode::services::bsim::BsimService service;
    EXPECT_TRUE(service.resource_loaded()) << service.resource_error();
    EXPECT_EQ(service.resource_name(), "lshweights_64.xml");
    const auto result = service.analyze(synthetic_features());
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->vector.hash_count, result->features.hashes.size());
    EXPECT_EQ(result->features.settings, 0x49U);
}

/// Verifies the real fixture is produced by the CMake target used by integration tests.
TEST(BsimFixture, ExecutableExists) {
    const std::filesystem::path fixture(BSIM_FIXTURE_PATH);
    ASSERT_TRUE(std::filesystem::exists(fixture));
    const auto command = std::string{"\""} + fixture.string() + "\"";
    EXPECT_EQ(std::system(command.c_str()), 0);
}

} // namespace
