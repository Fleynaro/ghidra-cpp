// Independent Boost.System consumer for the Ghidra FID pipeline.

#include <boost/system/error_code.hpp>

#include <cstdint>
#include <string>

namespace {

// Exercise category construction, error messages, and condition conversion.
__declspec(noinline) std::uint32_t exercise_system() {
    const boost::system::error_code generic =
        boost::system::errc::make_error_code(boost::system::errc::invalid_argument);
    const boost::system::error_code native(2, boost::system::system_category());
    const std::string generic_message = generic.message();
    const std::string native_message = native.message();
    const bool categories_differ = generic.category() != native.category();
    return categories_differ && !generic_message.empty() && !native_message.empty() ? 3U : 0U;
}

// This function is not supplied by Boost and is the negative FID control.
__declspec(noinline) std::uint32_t non_boost_control(std::uint32_t value) {
    value ^= 0xCAFEBABEU;
    return value * 747796405U + 2891336453U;
}

}  // namespace

// Make both paths observable to Ghidra and the linker.
int main() {
    const std::uint32_t result = exercise_system();
    const std::uint32_t control = non_boost_control(result);
    return result == 0U || control == 0U ? 1 : 0;
}
