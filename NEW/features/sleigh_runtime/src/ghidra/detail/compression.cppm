module;

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

#include <zlib.h>

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
    Decompress(void);  ///< Initialize the inflate algorithm state
    ~Decompress(void); ///< Free algorithm state resources

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
    int4 inflate(uint1* buffer, int4 sz); ///< Inflate as much as possible into given buffer
};

} // namespace ghidra

#endif
