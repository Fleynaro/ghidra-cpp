export module recode.service.bsim;

import std;
import recode.core.contracts.bsim;
import recode.service.bsim.resource;
import recode.service.bsim.vector_factory;

// Ported orchestration from Ghidra/Features/BSim/src/main/java/ghidra/features/bsim/query/GenSignatures.java.

export namespace recode::services::bsim {

/// Provides the autonomous function-to-feature-to-vector BSim computational service.
class BsimService final : public recode::core::contracts::IFunctionSimilarityService {
public:
    /// Constructs the service and loads the architecture-selected Ghidra weight resource.
    explicit BsimService(recode::core::contracts::SimilarityOptions options = {}) : options_(std::move(options)) {
        if (options_.resource_directory.empty()) {
#ifdef BSIM_RESOURCE_DIRECTORY
            options_.resource_directory = BSIM_RESOURCE_DIRECTORY;
#endif
        }
        VectorFactory factory;
        try {
            const auto configuration = load_weight_configuration(select_resource());
            factory.set(configuration.weights, configuration.lookup, configuration.settings);
            selected_resource_ = select_resource().filename().string();
            loaded_resource_ = true;
        } catch (const std::exception& error) {
            // The fallback is deterministic and keeps synthetic unit tests usable;
            // callers can inspect resource_loaded() before claiming compatibility.
            fallback_error_ = error.what();
            WeightFactory weights;
            weights.set_logarithmic_tf_weights();
            weights.set_idf_weight(0, 1.0);
            weights.set_significance_parameters(13.0, 0.2, 20.0, 0.2, 20.0, 1.0, 0.0);
            IDFLookup lookup;
            factory.set(weights, lookup, options_.settings);
        }
        vector_factory_ = std::move(factory);
    }

    /// Reports whether the original XML resource was loaded successfully.
    [[nodiscard]] bool resource_loaded() const noexcept {
        return loaded_resource_;
    }

    /// Returns a diagnostic explaining a deterministic fallback, if one was needed.
    [[nodiscard]] const std::string& resource_error() const noexcept {
        return fallback_error_;
    }

    /// Returns the architecture-selected original Ghidra resource filename.
    [[nodiscard]] const std::string& resource_name() const noexcept {
        return selected_resource_;
    }

    /// Validates and canonicalizes the sorted feature result produced by the Decompiler.
    [[nodiscard]] recode::core::Result<recode::core::contracts::FunctionSimilarityFeatures>
    generate_signature(const recode::core::contracts::FunctionSimilarityFeatures& features) const override {
        if (!std::ranges::is_sorted(features.hashes))
            return std::unexpected(
                recode::core::Error::make(recode::core::DiagnosticCode::invalid_argument,
                                          "BSim feature hashes must be sorted as unsigned 32-bit values",
                                          "Use the sorted feature vector returned by the native Decompiler."));
        if (features.settings != 0U && features.settings != options_.settings)
            return std::unexpected(
                recode::core::Error::make(recode::core::DiagnosticCode::invalid_argument,
                                          "BSim feature settings do not match the loaded vector resource",
                                          "Use the same signature settings for native generation and vectorization."));
        auto result = features;
        if (result.settings == 0U)
            result.settings = options_.settings;
        return result;
    }

    /// Generates a weighted sparse vector from one analyzed function result.
    [[nodiscard]] recode::core::Result<recode::core::contracts::SimilarityVector>
    generate_vector(const recode::core::contracts::FunctionSimilarityFeatures& features) const override {
        auto signature = generate_signature(features);
        if (!signature)
            return std::unexpected(signature.error());
        return vector_factory_.build_vector(signature->hashes);
    }

    /// Generates both the canonical feature result and weighted vector without recomputing the graph.
    [[nodiscard]] recode::core::Result<recode::core::contracts::FunctionSimilarityResult>
    analyze(const recode::core::contracts::FunctionSimilarityFeatures& features) const override {
        auto signature = generate_signature(features);
        if (!signature)
            return std::unexpected(signature.error());
        recode::core::contracts::FunctionSimilarityResult result;
        result.features = *signature;
        result.vector = vector_factory_.build_vector(signature->hashes);
        return result;
    }

    /// Compares two contract vectors using the ported cosine/significance layer.
    [[nodiscard]] recode::core::Result<recode::core::contracts::SimilarityComparison>
    compare(const recode::core::contracts::SimilarityVector& first,
            const recode::core::contracts::SimilarityVector& second) const override {
        return vector_factory_.compare(first, second);
    }

private:
    /// Selects the same native architecture weight file as GenSignatures.getWeightsFile.
    [[nodiscard]] std::filesystem::path select_resource() const {
        const auto& language = options_.language_id;
        std::vector<std::string> tokens;
        std::size_t begin = 0;
        while (begin <= language.size()) {
            const auto end = language.find(':', begin);
            tokens.push_back(language.substr(begin, end == std::string::npos ? std::string::npos : end - begin));
            if (end == std::string::npos)
                break;
            begin = end + 1U;
        }
        const auto processor = tokens.empty() ? std::string{} : tokens.front();
        const auto size = tokens.size() > 2U ? tokens[2] : std::string{};
        std::string filename = "lshweights_nosize.xml";
        if (processor == "Dalvik" || processor == "JVM")
            filename = "lshweights_cpool.xml";
        else if (size == "32")
            filename = "lshweights_32.xml";
        else if (size == "64")
            filename = language.find("-32") != std::string::npos ? "lshweights_64_32.xml" : "lshweights_64.xml";
        return options_.resource_directory / filename;
    }

    recode::core::contracts::SimilarityOptions options_;
    VectorFactory vector_factory_;
    bool loaded_resource_{};
    std::string fallback_error_;
    std::string selected_resource_;
};

} // namespace recode::services::bsim
