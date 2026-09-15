// Independent Boost.Serialization consumer for the Ghidra FID pipeline.

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/string.hpp>

#include <cstdint>
#include <sstream>
#include <string>

namespace {

struct probe_record {
    int id = 0;
    std::string name;

    // Serialize the complete record through Boost's archive contract.
    template <typename Archive>
    void serialize(Archive& archive, const unsigned int) {
        archive& id;
        archive& name;
    }
};

// Exercise text output and input archives with a real round trip.
__declspec(noinline) std::uint32_t exercise_serialization() {
    probe_record original{17, "Boost.Serialization"};
    std::stringstream stream;
    {
        boost::archive::text_oarchive archive(stream);
        archive << original;
    }
    probe_record restored;
    {
        boost::archive::text_iarchive archive(stream);
        archive >> restored;
    }
    return restored.id == original.id && restored.name == original.name ? 3U : 0U;
}

// This function is not supplied by Boost and is the negative FID control.
__declspec(noinline) std::uint32_t non_boost_control(std::uint32_t value) {
    value ^= 0xDEADBEEFU;
    return value * 69069U + 1U;
}

}  // namespace

// Make both paths observable to the executable.
int main() {
    const std::uint32_t result = exercise_serialization();
    const std::uint32_t control = non_boost_control(result);
    return result == 0U || control == 0U ? 1 : 0;
}
