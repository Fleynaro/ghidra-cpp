// Independent Boost.Iostreams consumer for the Ghidra FID pipeline.

#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/iostreams/stream.hpp>

#include <cstdint>
#include <cstdio>
#include <string>

namespace {

// Exercise file source/sink streams and a mapped file over real data.
__declspec(noinline) std::uint32_t exercise_iostreams() {
    const std::string path = "fidb_boost_iostreams_probe.txt";
    {
        boost::iostreams::file_sink sink(path);
        boost::iostreams::stream<boost::iostreams::file_sink> output(sink);
        output << "Boost.Iostreams FID probe";
        output.flush();
        output.close();
    }
    std::string line;
    {
        boost::iostreams::file_source source(path);
        boost::iostreams::stream<boost::iostreams::file_source> input(source);
        std::getline(input, line);
        input.close();
    }
    boost::iostreams::mapped_file_source mapped(path);
    const std::uint32_t size = static_cast<std::uint32_t>(mapped.size());
    mapped.close();
    std::remove(path.c_str());
    return line == "Boost.Iostreams FID probe" && size == line.size() ? 3U : 0U;
}

// This function is not supplied by Boost and is the negative FID control.
__declspec(noinline) std::uint32_t non_boost_control(std::uint32_t value) {
    value ^= 0xF00DFACEU;
    return value * 2654435761U + 2246822519U;
}

}  // namespace

// Make both paths part of the executable's observable result.
int main() {
    const std::uint32_t result = exercise_iostreams();
    const std::uint32_t control = non_boost_control(result);
    return result == 0U || control == 0U ? 1 : 0;
}
