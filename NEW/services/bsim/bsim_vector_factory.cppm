export module recode.service.bsim.vector_factory;

import std;
import recode.core.contracts.bsim;
import recode.service.bsim.cosine_vector;
import recode.service.bsim.idf;
import recode.service.bsim.vector_compare;
import recode.service.bsim.weight_factory;

namespace recode::services::bsim::detail {

/// Returns a trimmed view used for numeric text inside the Ghidra XML files.
[[nodiscard]] std::string_view trim_xml_text(std::string_view text) noexcept {
    const auto first = text.find_first_not_of(" \t\r\n");
    if (first == std::string_view::npos)
        return {};
    const auto last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1U);
}

/// Extracts one XML attribute from a known start-tag range.
[[nodiscard]] std::expected<std::string, std::string> xml_attribute(std::string_view start_tag, std::string_view name) {
    const auto name_position = start_tag.find(name);
    if (name_position == std::string_view::npos)
        return std::unexpected("Missing XML attribute: " + std::string(name));
    const auto equals = start_tag.find('=', name_position + name.size());
    if (equals == std::string_view::npos)
        return std::unexpected("Malformed XML attribute: " + std::string(name));
    const auto quote = start_tag.find_first_of("\"'", equals + 1U);
    if (quote == std::string_view::npos)
        return std::unexpected("Unquoted XML attribute: " + std::string(name));
    const auto end_quote = start_tag.find(start_tag[quote], quote + 1U);
    if (end_quote == std::string_view::npos)
        return std::unexpected("Unterminated XML attribute: " + std::string(name));
    return std::string(start_tag.substr(quote + 1U, end_quote - quote - 1U));
}

/// Parses a floating-point XML text value and reports the source tag on failure.
[[nodiscard]] std::expected<double, std::string> xml_double(std::string_view text, std::string_view tag) {
    try {
        const auto value_text = trim_xml_text(text);
        std::size_t consumed = 0;
        const auto value = std::stod(std::string(value_text), &consumed);
        if (consumed != value_text.size())
            return std::unexpected("Invalid floating-point value in <" + std::string(tag) + ">");
        return value;
    } catch (const std::exception&) {
        return std::unexpected("Invalid floating-point value in <" + std::string(tag) + ">");
    }
}

/// Parses an unsigned XML integer using Java's base-0 hexadecimal convention.
[[nodiscard]] std::expected<std::uint32_t, std::string> xml_unsigned(std::string_view text, std::string_view tag) {
    try {
        const auto value_text = trim_xml_text(text);
        std::size_t consumed = 0;
        const auto value = std::stoull(std::string(value_text), &consumed, 0);
        if (consumed != value_text.size() || value > std::numeric_limits<std::uint32_t>::max())
            return std::unexpected("Invalid unsigned value in <" + std::string(tag) + ">");
        return static_cast<std::uint32_t>(value);
    } catch (const std::exception&) {
        return std::unexpected("Invalid unsigned value in <" + std::string(tag) + ">");
    }
}

/// Returns the first complete element range for a tag in a bounded XML view.
[[nodiscard]] std::expected<std::pair<std::string_view, std::string_view>, std::string>
xml_element(std::string_view section, std::string_view tag, std::size_t search_from = 0) {
    const std::string open_prefix = "<" + std::string(tag);
    const auto open = section.find(open_prefix, search_from);
    if (open == std::string_view::npos)
        return std::unexpected("Missing XML element <" + std::string(tag) + ">");
    const auto close_open = section.find('>', open + open_prefix.size());
    if (close_open == std::string_view::npos)
        return std::unexpected("Malformed XML start tag <" + std::string(tag) + ">");
    if (close_open > open && section[close_open - 1U] == '/')
        return std::pair{section.substr(open, close_open - open + 1U), std::string_view{}};
    const auto close_prefix = "</" + std::string(tag) + ">";
    const auto close = section.find(close_prefix, close_open + 1U);
    if (close == std::string_view::npos)
        return std::unexpected("Missing XML end tag </" + std::string(tag) + ">");
    return std::pair{section.substr(open, close_open - open + 1U),
                     section.substr(close_open + 1U, close - close_open - 1U)};
}

/// Parses all repeated child values while preserving source order.
[[nodiscard]] std::expected<std::vector<double>, std::string>
xml_doubles(std::string_view section, std::string_view tag, std::size_t expected_count) {
    std::vector<double> values;
    std::size_t search_from = 0;
    for (;;) {
        const auto element = xml_element(section, tag, search_from);
        if (!element) {
            if (values.size() == expected_count)
                return values;
            return std::unexpected(element.error());
        }
        const auto value = xml_double(element->second, tag);
        if (!value)
            return std::unexpected(value.error());
        values.push_back(*value);
        if (values.size() > expected_count)
            return std::unexpected("Too many <" + std::string(tag) + "> values");
        const auto open = section.find("<" + std::string(tag), search_from);
        const auto close_open = section.find('>', open);
        const auto close = section.find("</" + std::string(tag) + ">", close_open + 1U);
        search_from = close + std::string(tag).size() + 3U;
    }
}

/// Represents a fully parsed, not-yet-installed weight resource.
struct ParsedWeightResource {
    WeightFactory weights;
    IDFLookup lookup;
    std::uint32_t settings{};
};

/// Reads one original Ghidra `<weights>` resource without Java XML classes.
[[nodiscard]] std::expected<ParsedWeightResource, std::string>
parse_weight_resource(const std::filesystem::path& path) {
    std::ifstream input(path);
    if (!input)
        return std::unexpected("Unable to open BSim weight resource: " + path.string());
    std::ostringstream content;
    content << input.rdbuf();
    const std::string xml = content.str();

    const auto root = xml_element(xml, "weights");
    if (!root)
        return std::unexpected(root.error());
    const auto root_settings = xml_attribute(root->first, "settings");
    if (!root_settings)
        return std::unexpected(root_settings.error());
    const auto settings = xml_unsigned(*root_settings, "settings");
    if (!settings)
        return std::unexpected(settings.error());

    const auto factory = xml_element(root->second, "weightfactory");
    if (!factory)
        return std::unexpected(factory.error());
    const auto scale_text = xml_attribute(factory->first, "scale");
    const auto addend_text = xml_attribute(factory->first, "addend");
    if (!scale_text)
        return std::unexpected(scale_text.error());
    if (!addend_text)
        return std::unexpected(addend_text.error());
    const auto scale = xml_double(*scale_text, "scale");
    const auto addend = xml_double(*addend_text, "addend");
    if (!scale)
        return std::unexpected(scale.error());
    if (!addend)
        return std::unexpected(addend.error());

    const auto idf = xml_doubles(factory->second, "idf", WeightFactory::idf_size_constant);
    const auto tf = xml_doubles(factory->second, "tf", WeightFactory::tf_size_constant);
    if (!idf)
        return std::unexpected(idf.error());
    if (!tf)
        return std::unexpected(tf.error());

    const auto read_scalar = [&](std::string_view tag) -> std::expected<double, std::string> {
        const auto element = xml_element(factory->second, tag);
        if (!element)
            return std::unexpected(element.error());
        return xml_double(element->second, tag);
    };
    const auto weight_norm = read_scalar("weightnorm");
    const auto prob_flip0 = read_scalar("probflip0");
    const auto prob_flip1 = read_scalar("probflip1");
    const auto prob_diff0 = read_scalar("probdiff0");
    const auto prob_diff1 = read_scalar("probdiff1");
    if (!weight_norm || !prob_flip0 || !prob_flip1 || !prob_diff0 || !prob_diff1)
        return std::unexpected((!weight_norm  ? weight_norm.error()
                                : !prob_flip0 ? prob_flip0.error()
                                : !prob_flip1 ? prob_flip1.error()
                                : !prob_diff0 ? prob_diff0.error()
                                              : prob_diff1.error()));

    WeightFactory parsed_weights;
    const double scale_sqrt = std::sqrt(*scale);
    for (std::size_t index = 0; index < idf->size(); ++index)
        parsed_weights.set_idf_weight(index, (*idf)[index] * scale_sqrt);
    for (std::size_t index = 0; index < tf->size(); ++index)
        parsed_weights.set_tf_weight(index, (*tf)[index]);
    parsed_weights.set_significance_parameters(*weight_norm / *scale, *prob_flip0, *prob_flip1, *prob_diff0,
                                               *prob_diff1, *scale, *addend);

    IDFLookup parsed_lookup;
    const auto lookup = xml_element(root->second, "idflookup");
    if (lookup) {
        const auto lookup_size_text = xml_attribute(lookup->first, "size");
        if (!lookup_size_text) {
            if (lookup->second.empty() && lookup->first.find("/>") != std::string_view::npos)
                return ParsedWeightResource{std::move(parsed_weights), std::move(parsed_lookup), *settings};
            return std::unexpected(lookup_size_text.error());
        }
        const auto lookup_size = xml_unsigned(*lookup_size_text, "size");
        if (!lookup_size)
            return std::unexpected(lookup_size.error());

        std::vector<std::uint32_t> pairs;
        std::size_t search_from = 0;
        for (;;) {
            const auto element = xml_element(lookup->second, "hash", search_from);
            if (!element)
                break;
            const auto count_text = xml_attribute(element->first, "count");
            if (!count_text)
                return std::unexpected(count_text.error());
            const auto count = xml_unsigned(*count_text, "count");
            const auto hash = xml_unsigned(element->second, "hash");
            if (!count || !hash)
                return std::unexpected(!count ? count.error() : hash.error());
            pairs.push_back(*hash);
            pairs.push_back(*count);
            const auto open = lookup->second.find("<hash", search_from);
            const auto close_open = lookup->second.find('>', open);
            const auto close = lookup->second.find("</hash>", close_open + 1U);
            search_from = close + 7U;
        }
        parsed_lookup.set_pairs(pairs, *lookup_size);
    } else if (root->second.find("<idflookup") != std::string_view::npos) {
        return std::unexpected("Malformed <idflookup> element");
    }

    return ParsedWeightResource{std::move(parsed_weights), std::move(parsed_lookup), *settings};
}

} // namespace recode::services::bsim::detail

// Ported from:
// Ghidra/Framework/Generic/src/main/java/generic/lsh/vector/LSHVectorFactory.java
// Ghidra/Framework/Generic/src/main/java/generic/lsh/vector/WeightedLSHCosineVectorFactory.java
// Original methods: LSHVectorFactory.set, getSelfSignificance,
// calculateSignificance, and WeightedLSHCosineVectorFactory.buildVector.

export namespace recode::services::bsim {

/// Builds contract-owned weighted vectors and calculates Ghidra significance.
///
/// The factory owns copies of the weight and IDF tables so every returned
/// SimilarityVector remains value-owned and independently usable after this
/// factory is destroyed.
class WeightedVectorFactory {
public:
    /// Constructs a factory with Java's fresh zero-weight and empty-lookup state.
    WeightedVectorFactory() = default;

    /// Installs weighting tables and the settings identifier by value.
    void set(const WeightFactory& weights, const IDFLookup& lookup, std::uint32_t settings) {
        weight_factory_ = weights;
        idf_lookup_ = lookup;
        settings_ = settings;
    }

    /// Returns true only when an IDF table has been initialized and is non-null in Java terms.
    [[nodiscard]] bool is_loaded() const noexcept {
        return !idf_lookup_.empty();
    }

    /// Returns the table's final significance scale.
    [[nodiscard]] double get_significance_scale() const noexcept {
        return weight_factory_.get_scale();
    }

    /// Returns the table's final significance addend.
    [[nodiscard]] double get_significance_addend() const noexcept {
        return weight_factory_.get_addend();
    }

    /// Returns the settings identifier associated with the loaded tables.
    [[nodiscard]] std::uint32_t get_settings() const noexcept {
        return settings_;
    }

    /// Builds an empty contract vector with the Java zero-vector metadata.
    [[nodiscard]] recode::core::contracts::SimilarityVector build_zero_vector() const {
        return CosineVector{}.to_similarity_vector();
    }

    /// Builds a sparse contract vector from already sorted duplicate-preserving hashes.
    [[nodiscard]] recode::core::contracts::SimilarityVector
    build_vector(std::span<const std::uint32_t> features) const {
        return build_cosine_vector(features).to_similarity_vector();
    }

    /// Builds a sparse contract vector from the requested owning hash vector.
    [[nodiscard]] recode::core::contracts::SimilarityVector build(const std::vector<std::uint32_t>& features) const {
        return build_vector(std::span<const std::uint32_t>(features));
    }

    /// Loads one original Ghidra `<weights>` XML resource atomically.
    ///
    /// WeightFactory.restoreXml stores IDF values multiplied by sqrt(scale),
    /// while the XML stores normalized values.  This method performs that same
    /// conversion and preserves the IDFLookup XML size attribute.
    std::expected<void, std::string> load_resource(const std::filesystem::path& path) {
        std::expected<detail::ParsedWeightResource, std::string> parsed =
            std::unexpected("Unable to parse BSim weight resource");
        try {
            parsed = detail::parse_weight_resource(path);
        } catch (const std::exception& exception) {
            return std::unexpected("Unable to parse BSim weight resource: " + std::string(exception.what()));
        }
        if (!parsed)
            return std::unexpected(parsed.error());
        weight_factory_ = parsed->weights;
        idf_lookup_ = parsed->lookup;
        settings_ = parsed->settings;
        return {};
    }

    /// Builds the reusable implementation vector before value conversion.
    [[nodiscard]] CosineVector build_cosine_vector(std::span<const std::uint32_t> features) const {
        return CosineVector(features, weight_factory_, idf_lookup_);
    }

    /// Calculates self significance as squared length plus the factory addend.
    [[nodiscard]] double get_self_significance(const CosineVector& vector) const noexcept {
        return vector.get_length() * vector.get_length() + weight_factory_.get_addend();
    }

    /// Calculates self significance from the contract's stored vector length.
    [[nodiscard]] double get_self_significance(const recode::core::contracts::SimilarityVector& vector) const noexcept {
        return vector.length * vector.length + weight_factory_.get_addend();
    }

    /// Fills count metadata and applies Ghidra's normalized significance equation.
    [[nodiscard]] double calculate_significance(VectorCompare& data) const noexcept {
        data.fill_out();
        return data.dotproduct -
               static_cast<double>(data.numflip) * (weight_factory_.get_flip_norm0() +
                                                    weight_factory_.get_flip_norm1() / static_cast<double>(data.max)) -
               static_cast<double>(data.diff) * (weight_factory_.get_diff_norm0() +
                                                 weight_factory_.get_diff_norm1() / static_cast<double>(data.max)) +
               weight_factory_.get_addend();
    }

    /// Compares contract vectors and returns cosine, counts, and significance.
    [[nodiscard]] recode::core::contracts::SimilarityComparison
    compare(const recode::core::contracts::SimilarityVector& first,
            const recode::core::contracts::SimilarityVector& second) const {
        const CosineVector first_vector(first);
        const CosineVector second_vector(second);
        VectorCompare data;
        const double cosine = first_vector.compare(second_vector, data);
        const double significance = calculate_significance(data);

        recode::core::contracts::SimilarityComparison result;
        result.cosine = cosine;
        result.dot_product = data.dotproduct;
        result.first_hash_count = data.acount;
        result.second_hash_count = data.bcount;
        result.intersect_count = data.intersectcount;
        result.minimum_count = data.min;
        result.maximum_count = data.max;
        result.flipped_count = data.numflip;
        result.difference_count = data.diff;
        result.significance = significance;
        return result;
    }

    /// Returns the owned weight state for resource loaders and diagnostics.
    [[nodiscard]] const WeightFactory& weight_factory() const noexcept {
        return weight_factory_;
    }

    /// Returns the owned IDF state for resource loaders and diagnostics.
    [[nodiscard]] const IDFLookup& idf_lookup() const noexcept {
        return idf_lookup_;
    }

    /// Java-style spelling retained for direct source-to-source comparisons.
    [[nodiscard]] bool isLoaded() const noexcept {
        return is_loaded();
    }

    /// Java-style spelling retained for direct source-to-source comparisons.
    [[nodiscard]] double getSignificanceScale() const noexcept {
        return get_significance_scale();
    }

    /// Java-style spelling retained for direct source-to-source comparisons.
    [[nodiscard]] double getSignificanceAddend() const noexcept {
        return get_significance_addend();
    }

    /// Java-style spelling retained for direct source-to-source comparisons.
    [[nodiscard]] std::uint32_t getSettings() const noexcept {
        return get_settings();
    }

    /// Java-style spelling retained for direct source-to-source comparisons.
    [[nodiscard]] recode::core::contracts::SimilarityVector buildZeroVector() const {
        return build_zero_vector();
    }

    /// Java-style spelling retained for direct source-to-source comparisons.
    [[nodiscard]] recode::core::contracts::SimilarityVector buildVector(std::span<const std::uint32_t> features) const {
        return build_vector(features);
    }

    /// Java-style spelling retained for direct source-to-source comparisons.
    [[nodiscard]] double getSelfSignificance(const CosineVector& vector) const noexcept {
        return get_self_significance(vector);
    }

    /// Java-style spelling retained for direct source-to-source comparisons.
    [[nodiscard]] double calculateSignificance(VectorCompare& data) const noexcept {
        return calculate_significance(data);
    }

private:
    WeightFactory weight_factory_;
    IDFLookup idf_lookup_;
    std::uint32_t settings_{};
};

/// Keeps the short internal name used by existing BSim service adapters.
using VectorFactory = WeightedVectorFactory;

/// Retains the original Java factory name for source-to-source adapters.
using WeightedLSHCosineVectorFactory = WeightedVectorFactory;

} // namespace recode::services::bsim
