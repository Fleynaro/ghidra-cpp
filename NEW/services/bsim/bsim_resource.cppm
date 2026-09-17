export module recode.service.bsim.resource;

import std;
import recode.service.bsim.idf;
import recode.service.bsim.weight_factory;

// Ported from:
// Ghidra/Framework/Generic/src/main/java/generic/lsh/vector/LSHVectorFactory.java
// Original: readWeights, WeightFactory.restoreXml, and IDFLookup.restoreXml.

export namespace recode::services::bsim {

/// Owns the two independent tables parsed from one original Ghidra XML resource.
struct WeightConfiguration final {
    WeightFactory weights;
    IDFLookup lookup;
    std::uint32_t settings{};
};

namespace detail {

/// Extracts all text values for one repeated XML element.
[[nodiscard]] inline std::vector<std::string> xml_values(const std::string& text, std::string_view tag) {
    std::vector<std::string> values;
    const std::string open = "<" + std::string(tag) + ">";
    const std::string close = "</" + std::string(tag) + ">";
    std::size_t position = 0;
    while ((position = text.find(open, position)) != std::string::npos) {
        const auto begin = position + open.size();
        const auto end = text.find(close, begin);
        if (end == std::string::npos)
            throw std::runtime_error("Malformed BSim resource element: " + std::string(tag));
        values.push_back(text.substr(begin, end - begin));
        position = end + close.size();
    }
    return values;
}

/// Reads one XML attribute from the first matching element.
[[nodiscard]] inline std::string xml_attribute(const std::string& text, std::string_view element,
                                               std::string_view attribute) {
    const auto begin = text.find("<" + std::string(element));
    if (begin == std::string::npos)
        throw std::runtime_error("Missing BSim resource element: " + std::string(element));
    const auto end = text.find('>', begin);
    const auto key = std::string(attribute) + "=\"";
    const auto key_position = text.find(key, begin);
    if (key_position == std::string::npos || key_position > end)
        throw std::runtime_error("Missing BSim resource attribute: " + std::string(attribute));
    const auto value_begin = key_position + key.size();
    const auto value_end = text.find('"', value_begin);
    return text.substr(value_begin, value_end - value_begin);
}

/// Parses a Ghidra hexadecimal integer with optional signed/unsigned prefixes.
[[nodiscard]] inline std::uint32_t parse_unsigned(std::string value) {
    std::size_t consumed = 0;
    const auto result = std::stoull(value, &consumed, 0);
    if (consumed != value.size() || result > std::numeric_limits<std::uint32_t>::max())
        throw std::runtime_error("Invalid unsigned integer in BSim resource: " + value);
    return static_cast<std::uint32_t>(result);
}

} // namespace detail

/// Loads one original lshweights XML resource without Ghidra resource APIs.
[[nodiscard]] inline WeightConfiguration load_weight_configuration(const std::filesystem::path& path) {
    std::ifstream input(path);
    if (!input)
        throw std::runtime_error("Unable to open BSim weight resource: " + path.string());
    const std::string text{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
    WeightConfiguration configuration;
    configuration.settings = detail::parse_unsigned(detail::xml_attribute(text, "weights", "settings"));
    const auto scale = std::stod(detail::xml_attribute(text, "weightfactory", "scale"));
    const auto addend = std::stod(detail::xml_attribute(text, "weightfactory", "addend"));
    const auto idf_values = detail::xml_values(text, "idf");
    const auto tf_values = detail::xml_values(text, "tf");
    if (idf_values.size() != WeightFactory::idf_size_constant || tf_values.size() != WeightFactory::tf_size_constant)
        throw std::runtime_error("BSim resource has an unexpected weight-table size: " + path.string());
    const auto scale_sqrt = std::sqrt(scale);
    for (std::size_t index = 0; index < idf_values.size(); ++index)
        configuration.weights.set_idf_weight(index, std::stod(idf_values[index]) * scale_sqrt);
    for (std::size_t index = 0; index < tf_values.size(); ++index)
        configuration.weights.set_tf_weight(index, std::stod(tf_values[index]));
    const auto weight_norm = std::stod(detail::xml_values(text, "weightnorm").at(0)) / scale;
    const auto flip0 = std::stod(detail::xml_values(text, "probflip0").at(0));
    const auto flip1 = std::stod(detail::xml_values(text, "probflip1").at(0));
    const auto diff0 = std::stod(detail::xml_values(text, "probdiff0").at(0));
    const auto diff1 = std::stod(detail::xml_values(text, "probdiff1").at(0));
    configuration.weights.set_significance_parameters(weight_norm, flip0, flip1, diff0, diff1, scale, addend);

    const auto lookup_size = detail::parse_unsigned(detail::xml_attribute(text, "idflookup", "size"));
    std::vector<std::uint32_t> pairs;
    pairs.reserve(static_cast<std::size_t>(lookup_size) * 2U);
    std::size_t position = text.find("<idflookup");
    const auto lookup_end = text.find("</idflookup>", position);
    while ((position = text.find("<hash", position)) != std::string::npos && position < lookup_end) {
        const auto element_end = text.find('>', position);
        const auto count_key = text.find("count=\"", position);
        if (count_key == std::string::npos || count_key > element_end)
            throw std::runtime_error("Missing IDF count in BSim resource");
        const auto count_begin = count_key + 7U;
        const auto count_end = text.find('"', count_begin);
        const auto value_begin = element_end + 1U;
        const auto value_end = text.find("</hash>", value_begin);
        pairs.push_back(detail::parse_unsigned(text.substr(value_begin, value_end - value_begin)));
        pairs.push_back(detail::parse_unsigned(text.substr(count_begin, count_end - count_begin)));
        position = value_end + 7U;
    }
    configuration.lookup.set(pairs);
    return configuration;
}

} // namespace recode::services::bsim
