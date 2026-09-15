// Independent Boost.Regex consumer for the Ghidra FID pipeline.

#include <boost/regex.hpp>

#include <cstdint>
#include <string>

namespace {

// Exercise compiled expressions, search, match, and replacement operations.
__declspec(noinline) std::uint32_t exercise_regex() {
    const boost::regex expression("(Boost) (Regex)");
    const std::string text = "Boost Regex FID probe";
    boost::smatch match;
    const bool found = boost::regex_search(text, match, expression);
    const bool exact = boost::regex_match(std::string("Boost Regex"), expression);
    const std::string replaced = boost::regex_replace(text, expression, "Ghidra \\2");
    return found && exact && replaced == "Ghidra Regex FID probe" ? 3U : 0U;
}

// This function is not supplied by Boost and is the negative FID control.
__declspec(noinline) std::uint32_t non_boost_control(std::uint32_t value) {
    value ^= 0x2468ACE0U;
    return value * 1664525U + 1013904223U;
}

}  // namespace

// Make the positive and negative paths observable to the executable.
int main() {
    const std::uint32_t result = exercise_regex();
    const std::uint32_t control = non_boost_control(result);
    return result == 0U || control == 0U ? 1 : 0;
}
