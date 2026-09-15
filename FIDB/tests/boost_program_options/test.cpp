// Independent Boost.Program_options consumer for the Ghidra FID pipeline.

#include <boost/program_options.hpp>

#include <cstdint>

namespace {

// Parse real command-line options and materialize a Boost variables_map.
__declspec(noinline) std::uint32_t exercise_program_options() {
    namespace po = boost::program_options;
    po::options_description description("FID probe options");
    description.add_options()("count", po::value<int>()->default_value(0), "count")(
        "name", po::value<std::string>()->default_value(""), "name");
    const char* arguments[] = {"probe", "--count=7", "--name=Boost"};
    po::parsed_options parsed = po::parse_command_line(3, arguments, description);
    po::variables_map values;
    po::store(parsed, values);
    po::notify(values);
    return values["count"].as<int>() == 7 && values["name"].as<std::string>() == "Boost" ? 3U : 0U;
}

// This function is not supplied by Boost and is the negative FID control.
__declspec(noinline) std::uint32_t non_boost_control(std::uint32_t value) {
    value ^= 0x55AA55AAU;
    return value * 22695477U + 1U;
}

}  // namespace

// Make both paths part of the process result.
int main() {
    const std::uint32_t result = exercise_program_options();
    const std::uint32_t control = non_boost_control(result);
    return result == 0U || control == 0U ? 1 : 0;
}
