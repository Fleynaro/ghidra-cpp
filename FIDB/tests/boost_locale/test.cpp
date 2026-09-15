// Independent Boost.Locale consumer for the Ghidra FID pipeline.

#include <boost/locale.hpp>

#include <cstdint>
#include <locale>
#include <string>

namespace {

// Exercise locale generation and real case/normalization conversion APIs.
__declspec(noinline) std::uint32_t exercise_locale() {
    boost::locale::generator generator;
    generator.clear_paths();
    generator.clear_domains();
    const std::locale locale = generator("C");
    const std::string upper = boost::locale::to_upper("Boost Locale", locale);
    const std::string lower = boost::locale::to_lower("BOOST LOCALE", locale);
    const std::string normalized =
        boost::locale::normalize("Boost Locale", boost::locale::norm_default, locale);
    return upper == "BOOST LOCALE" && lower == "boost locale" && !normalized.empty() ? 3U : 0U;
}

// This function is not supplied by Boost and is the negative FID control.
__declspec(noinline) std::uint32_t non_boost_control(std::uint32_t value) {
    value ^= 0x0F0F0F0FU;
    return value * 134775813U + 1U;
}

}  // namespace

// Make both paths part of the executable's observable result.
int main() {
    const std::uint32_t result = exercise_locale();
    const std::uint32_t control = non_boost_control(result);
    return result == 0U || control == 0U ? 1 : 0;
}
