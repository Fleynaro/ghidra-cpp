module;
#include <memory>
#include <zlib.h>

/* ###
 * IP: GHIDRA
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/// \file compression.cppm
/// \brief The Compress and Decompress classes wrapping the deflate and inflate algorithms
#ifndef __COMPRESSION__
#define __COMPRESSION__

export module sleigh_runtime.ghidra:compression;

export import :error;

export namespace ghidra {

/// \brief Wrapper for the inflate algorithm
///
/// Initialize/free algorithm resources. Provide successive arrays of compressed bytes via
/// the input() method. Compute successive arrays of uncompressed bytes via the inflate() method.
class Decompress {
    z_stream compStream; ///< The zlib inflate algorithm state
    bool streamFinished; ///< Set to \b true if the end of the compressed stream has been reached
public:
    /// Initialize the inflate algorithm state.
    Decompress(void) {
        streamFinished = false;
        compStream.zalloc = Z_NULL;
        compStream.zfree = Z_NULL;
        compStream.opaque = Z_NULL;
        compStream.avail_in = 0;
        compStream.next_in = Z_NULL;
        int ret = inflateInit(&compStream);
        if (ret != Z_OK)
            throw LowlevelError("Could not initialize inflate stream state");
    }

    /// Free algorithm state resources.
    ~Decompress(void) {
        inflateEnd(&compStream);
    }

    /// \brief Provide the next sequence of compressed bytes
    ///
    /// \param buffer is a pointer to the compressed bytes
    /// \param sz is the number of bytes
    void input(uint1* buffer, int4 sz) {
        compStream.next_in = buffer;
        compStream.avail_in = sz;
    }

    bool isFinished(void) const {
        return streamFinished;
    } ///< Return \b if end of compressed stream is reached

    /// Return the number of bytes of output space still available.  Output may be limited by the amount
    /// of space in the output buffer or the amount of data available in the current input buffer.
    /// \param buffer is where uncompressed bytes are stored
    /// \param sz is the size, in bytes, of the buffer
    /// \return the number of output bytes still available
    int4 inflate(uint1* buffer, int4 sz) {
        compStream.avail_out = sz;
        compStream.next_out = buffer;

        int ret = ::inflate(&compStream, Z_NO_FLUSH);
        switch (ret) {
            case Z_NEED_DICT:
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
            case Z_STREAM_ERROR:
                throw LowlevelError("Error decompressing stream");
            case Z_STREAM_END:
                streamFinished = true;
                break;
            default:
                break;
        }

        return compStream.avail_out;
    }
};

} // namespace ghidra

#endif
