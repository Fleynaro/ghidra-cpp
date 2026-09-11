
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
/// \file loadimage.cppm
/// \brief The byte-loading contract used by the standalone SLEIGH runtime.

export module sleigh_runtime:loadimage;
import std;
export import :address;

export namespace ghidra {

/// Exception indicating that an image cannot satisfy a byte request.
struct DataUnavailError : public LowlevelError {
    /// Creates an error describing the unavailable address range.
    explicit DataUnavailError(const string& message) : LowlevelError(message) {}
};

/// Supplies instruction bytes to the decoder.
///
/// The full Ghidra loader API also exposed symbols, sections, raw-file
/// ownership, and readonly ranges. None of those concepts participate in a
/// compiled `.sla` decode, so this runtime keeps only the four operations used
/// by `Sleigh` and its in-memory adapter.
class LoadImage {
public:
    /// Creates an image byte-source contract.
    LoadImage() = default;

    /// Destroys the polymorphic image interface.
    virtual ~LoadImage() = default;

    /// Fills `ptr` with `size` bytes beginning at `addr`.
    virtual void loadFill(uint1* ptr, int4 size, const Address& addr) = 0;

    /// Returns the architecture label associated with the image.
    virtual string getArchType() const = 0;

    /// Applies a loader-specific virtual-address adjustment.
    virtual void adjustVma(long adjust) = 0;
};

} // namespace ghidra
