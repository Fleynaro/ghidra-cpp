
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

export module sleigh_runtime:varnode;
export import :address;

export namespace ghidra {

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
    bool operator<(const VarnodeData& other) const {
        if (space != other.space)
            return addrSpaceGetIndex(space) < addrSpaceGetIndex(other.space);
        if (offset != other.offset)
            return offset < other.offset;
        return size > other.size;
    }

    /// Compares all three storage components.
    bool operator==(const VarnodeData& other) const {
        return space == other.space && offset == other.offset && size == other.size;
    }

    /// Returns the inverse of equality.
    bool operator!=(const VarnodeData& other) const {
        return !(*this == other);
    }

    /// Returns this storage triple as an address.
    Address getAddr() const {
        return Address(space, offset);
    }

    /// Interprets the offset as an encoded address-space pointer.
    AddrSpace* getSpaceFromConst() const {
        return reinterpret_cast<AddrSpace*>(static_cast<uintp>(offset));
    }

    /// Decodes a complete varnode element from the active stream.
    ///
    /// Decodes a VarnodeData from an `<addr>`, `<register>`, or `<varnode>` element.
    void decode(Decoder& decoder) {
        const uint4 element = decoder.openElement();
        decodeFromAttributes(decoder);
        decoder.closeElement(element);
    }

    /// Decodes varnode attributes from the currently open element.
    ///
    /// Reads a VarnodeData from attributes that may be mixed with other metadata.
    void decodeFromAttributes(Decoder& decoder) {
        space = nullptr;
        size = 0;
        for (;;) {
            const uint4 attribute = decoder.getNextAttributeId();
            if (attribute == 0)
                break;
            if (attribute == ATTRIB_SPACE) {
                space = decoder.readSpace();
                decoder.rewindAttributes();
                offset = addrSpaceDecodeAttributes(space, decoder, size);
                break;
            }
            if (attribute == ATTRIB_NAME) {
                const Translate* translator = addrSpaceManagerDefaultCodeTranslator(decoder.getAddrSpaceManager());
                AddrSpace* registerSpace;
                uintb registerOffset;
                uint4 registerSize;
                translateGetRegister(translator, decoder.readString(), registerSpace, registerOffset, registerSize);
                space = registerSpace;
                offset = registerOffset;
                size = registerSize;
                break;
            }
        }
    }

    /// Tests whether `other` is contained by this storage range.
    ///
    /// Tests whether another raw storage range is contained by this one.
    bool contains(const VarnodeData& other) const {
        if (space != other.space || other.offset < offset)
            return false;
        return (offset + (size - 1)) >= (other.offset + (other.size - 1));
    }

    /// Tests whether `other` is the immediately lower, contiguous piece.
    ///
    /// Tests whether another raw storage range immediately precedes this one.
    bool isContiguous(const VarnodeData& other) const {
        if (space != other.space)
            return false;
        if (addrSpaceIsBigEndian(space))
            return addrSpaceWrapOffset(space, offset + size) == other.offset;
        return addrSpaceWrapOffset(space, other.offset + other.size) == offset;
    }
};

/// Decodes a varnode for the address partition without requiring a cyclic
/// import back to this partition.
void decodeVarnodeData(Decoder& decoder, AddrSpace*& space, uintb& offset, uint4& size) {
    VarnodeData varnode;
    varnode.decode(decoder);
    space = varnode.space;
    offset = varnode.offset;
    size = varnode.size;
}

} // namespace ghidra
