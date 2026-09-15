// Independent Boost.Filesystem consumer for the Ghidra FID pipeline.

#include <boost/filesystem.hpp>

#include <cstdint>
#include <fstream>
#include <string>

namespace {

// Exercise real filesystem creation, status, iteration, and recursive removal APIs.
__declspec(noinline) std::uint32_t exercise_filesystem() {
    namespace fs = boost::filesystem;
    fs::path root = fs::temp_directory_path() / "fidb_boost_filesystem_probe";
    boost::system::error_code error;
    fs::remove_all(root, error);
    fs::create_directories(root / "nested", error);
    if (error) {
        return 0;
    }
    std::ofstream output((root / "nested" / "sample.txt").string(), std::ios::binary);
    output << "Boost.Filesystem FID probe";
    output.close();
    const fs::file_status status = fs::status(root / "nested" / "sample.txt", error);
    std::uint32_t count = fs::is_regular_file(status) ? 1U : 0U;
    for (fs::recursive_directory_iterator iterator(root), end; iterator != end; ++iterator) {
        ++count;
    }
    count += static_cast<std::uint32_t>(fs::remove_all(root, error));
    return error ? 0U : count;
}

// This function is not supplied by Boost and is the negative FID control.
__declspec(noinline) std::uint32_t non_boost_control(std::uint32_t value) {
    value ^= 0x13579BDFU;
    return value * 1103515245U + 12345U;
}

}  // namespace

// Retain both paths in a real process result so the linker cannot discard them.
int main() {
    const std::uint32_t result = exercise_filesystem();
    const std::uint32_t control = non_boost_control(result);
    return result == 0U || control == 0U ? 1 : 0;
}
