// Independent consumer for the FIDB pipeline.
// It intentionally links the generated static zlib library instead of declaring
// replacement functions, so the analyzed executable contains real zlib code.

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

#include <zlib.h>

namespace {

volatile std::uint32_t g_result = 0;

// Exercise several independent exported zlib APIs with real compression data.
// The noinline boundary keeps this consumer function distinct from the library
// code while preventing the optimizer from removing the calls.
__declspec(noinline) std::uint32_t run_zlib_operations(const unsigned char* input,
                                                       uLong input_size) {
    uLongf compressed_size = compressBound(input_size);
    std::vector<unsigned char> compressed(compressed_size);
    const int compress_status = compress(compressed.data(), &compressed_size, input, input_size);
    if (compress_status != Z_OK) {
        return 0;
    }

    uLongf restored_size = input_size;
    std::vector<unsigned char> restored(restored_size);
    const int uncompress_status =
        uncompress(restored.data(), &restored_size, compressed.data(), compressed_size);
    if (uncompress_status != Z_OK || restored_size != input_size ||
        std::memcmp(restored.data(), input, input_size) != 0) {
        return 0;
    }

    const uLong checksum = crc32_z(crc32_z(0L, Z_NULL, 0), input, input_size);
    const uLong digest = adler32_z(adler32_z(0L, Z_NULL, 0), input, input_size);
    return static_cast<std::uint32_t>(checksum ^ digest ^ compressed_size ^ restored_size);
}

// This function is deliberately not supplied by zlib and is the negative FID control.
// Its result is used by main so the linker must retain the function in the executable.
__declspec(noinline) std::uint32_t non_zlib_control(std::uint32_t value) {
    value ^= 0xA5A5A5A5u;
    value = value * 1664525u + 1013904223u;
    return (value >> 13) ^ (value << 7);
}

}  // namespace

// Keep the executable entry point observable and make both positive and negative
// paths part of a real process result.
int main() {
    static constexpr char input[] =
        "FIDB independently exercises zlib compression, decompression, CRC, and Adler data.";
    const std::uint32_t zlib_result = run_zlib_operations(
        reinterpret_cast<const unsigned char*>(input), static_cast<uLong>(std::strlen(input)));
    const std::uint32_t control_result = non_zlib_control(zlib_result);
    g_result = zlib_result ^ control_result;
    std::printf("zlib_result=%u control_result=%u\n", zlib_result, control_result);
    return zlib_result == 0 ? 1 : 0;
}
