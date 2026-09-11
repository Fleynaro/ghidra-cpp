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
/// \file varnode.cppm
/// \brief The raw storage triple shared by the runtime's address model.

#ifndef __VARNODE_HH__
#define __VARNODE_HH__

#include "address.cppm"

namespace ghidra {

/// Describes a concrete address-space, offset, and byte-size triple.
///
/// The active runtime materializes this value while reading compiled SLA
/// metadata and while emitting p-code. Evaluation behavior and data-flow links
/// are deliberately not part of this low-level value type.
struct VarnodeData {
    AddrSpace* space;
    uintb offset;
    uint4 size;

    /// Orders storage triples by address space, offset, and descending size.
    bool operator<(const VarnodeData& other) const;

    /// Compares all three storage components.
    bool operator==(const VarnodeData& other) const;

    /// Returns the inverse of equality.
    bool operator!=(const VarnodeData& other) const;

    /// Returns this storage triple as an address.
    Address getAddr() const;

    /// Interprets the offset as an encoded address-space pointer.
    AddrSpace* getSpaceFromConst() const;

    /// Decodes a complete varnode element from the active stream.
    void decode(Decoder& decoder);

    /// Decodes varnode attributes from the currently open element.
    void decodeFromAttributes(Decoder& decoder);

    /// Tests whether `other` is contained by this storage range.
    bool contains(const VarnodeData& other) const;

    /// Tests whether `other` is the immediately lower, contiguous piece.
    bool isContiguous(const VarnodeData& other) const;
};

/// Orders Varnodes by their space, offset, and size so they can be map keys.
inline bool VarnodeData::operator<(const VarnodeData& other) const {
    if (space != other.space)
        return space->getIndex() < other.space->getIndex();
    if (offset != other.offset)
        return offset < other.offset;
    return size > other.size;
}

/// Compares the complete identity of two raw storage triples.
inline bool VarnodeData::operator==(const VarnodeData& other) const {
    return space == other.space && offset == other.offset && size == other.size;
}

/// Returns true when any component differs.
inline bool VarnodeData::operator!=(const VarnodeData& other) const {
    return !(*this == other);
}

/// Builds an Address from the storage space and byte offset.
inline Address VarnodeData::getAddr() const {
    return Address(space, offset);
}

/// Recovers an address-space pointer encoded in a constant varnode.
inline AddrSpace* VarnodeData::getSpaceFromConst() const {
    return reinterpret_cast<AddrSpace*>(static_cast<uintp>(offset));
}

} // namespace ghidra
#endif
