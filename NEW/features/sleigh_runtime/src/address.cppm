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
/// \file address.cppm
/// \brief Classes for specifying addresses and other low-level constants
///
///  All addresses are absolute and there are no registers in CPUI. However,
///  all addresses are prefixed with an "immutable" pointer, which can
///  specify a separate RAM space, a register space, an i/o space etc. Thus
///  a translation from a real machine language will typically simulate registers
///  by placing them in their own space, separate from RAM. Indirection
///  (i.e. pointers) must be simulated through the LOAD and STORE ops.

export module sleigh_runtime:address;
import std;
export import :marshal;

export namespace ghidra {

class AddrSpace;
class AddrSpaceManager;
class Address;
class Translate;
struct VarnodeData;

/// The address partition uses these opaque operations to avoid importing the
/// space partition, whose complete classes depend back on Address.
int4 addrSpaceGetAddrSize(const AddrSpace* space);
bool addrSpaceIsBigEndian(const AddrSpace* space);
void addrSpacePrintRaw(const AddrSpace* space, ostream& stream, uintb offset);
uintb addrSpaceRead(const AddrSpace* space, const string& text, int4& size);
char addrSpaceGetShortcut(const AddrSpace* space);
const string& addrSpaceGetName(const AddrSpace* space);
int4 addrSpaceGetIndex(const AddrSpace* space);
uintb addrSpaceWrapOffset(const AddrSpace* space, uintb offset);
uintb addrSpaceGetHighest(const AddrSpace* space);
uintb addrSpaceDecodeAttributes(const AddrSpace* space, Decoder& decoder, uint4& size);
bool addrSpaceIsConstant(const AddrSpace* space);
bool addrSpaceIsJoin(const AddrSpace* space);
int4 addrSpaceOverlapJoin(const AddrSpace* space, uintb offset, int4 size, AddrSpace* pointSpace, uintb pointOffset,
                          int4 pointSkip);
void addrSpaceEncodeAttributes(const AddrSpace* space, Encoder& encoder, uintb offset);
void addrSpaceEncodeAttributes(const AddrSpace* space, Encoder& encoder, uintb offset, int4 size);
const Translate* addrSpaceManagerDefaultCodeTranslator(const AddrSpaceManager* manager);
AddrSpace* addrSpaceManagerGetSpaceByName(const AddrSpaceManager* manager, const string& name);
AddrSpace* addrSpaceManagerGetNextSpaceInOrder(const AddrSpaceManager* manager, AddrSpace* space);
void translateGetRegister(const Translate* translator, const string& name, AddrSpace*& space, uintb& offset,
                          uint4& size);
bool addrSpaceHighPtrPossible(const AddrSpace* space, const Address& address, int4 size);
void addrSpaceRenormalizeJoinAddress(const AddrSpace* space, Address& address, int4 size);
void decodeVarnodeData(Decoder& decoder, AddrSpace*& space, uintb& offset, uint4& size);
int4 leastsigbit_set(uintb value);
int4 mostsigbit_set(uintb value);

extern AttributeId ATTRIB_FIRST; ///< Marshaling attribute "first"
extern AttributeId ATTRIB_LAST;  ///< Marshaling attribute "last"
extern AttributeId ATTRIB_UNIQ;  ///< Marshaling attribute "uniq"

extern ElementId ELEM_ADDR;      ///< Marshaling element \<addr>
extern ElementId ELEM_RANGE;     ///< Marshaling element \<range>
extern ElementId ELEM_RANGELIST; ///< Marshaling element \<rangelist>
extern ElementId ELEM_REGISTER;  ///< Marshaling element \<register>
extern ElementId ELEM_SEQNUM;    ///< Marshaling element \<seqnum>
extern ElementId ELEM_VARNODE;   ///< Marshaling element \<varnode>

/// \brief A low-level machine address for labelling bytes and data.
///
/// All data that can be manipulated within the processor reverse
/// engineering model can be labelled with an Address. It is
/// simply an address space (AddrSpace) and an offset within that
/// space.  Note that processor registers are typically modelled
/// by creating a dedicated address space for them, as distinct
/// from RAM say, and then specifying certain addresses within the
/// register space that correspond to particular registers. However,
/// an arbitrary address could refer to anything,
/// RAM, ROM, cpu register, data segment, coprocessor, stack,
/// nvram, etc.
/// An Address represents an offset \e only, not an offset and length
class Address {
protected:
    AddrSpace* base; ///< Pointer to our address space
    uintb offset;    ///< Offset (in bytes)
public:
    /// An enum for specifying extremal addresses
    enum mach_extreme {
        m_minimal, ///< Smallest possible address
        m_maximal  ///< Biggest possible address
    };

    /// Some data structures sort on an Address, and it is convenient
    /// to be able to create an Address that is either bigger than
    /// or smaller than all other Addresses.
    /// \param ex is either \e m_minimal or \e m_maximal
    inline Address(mach_extreme ex) {
        if (ex == m_minimal) {
            base = (AddrSpace*)0;
            offset = 0;
        } else {
            base = (AddrSpace*)~((uintp)0);
            offset = ~((uintb)0);
        }
    }

    /// An invalid address is possible in some circumstances.
    /// This deliberately constructs an invalid address
    inline Address(void) {
        base = (AddrSpace*)0;
    }

    /// This is the basic Address constructor
    /// \param id is the space containing the address
    /// \param off is the offset of the address
    inline Address(AddrSpace* id, uintb off) {
        base = id;
        offset = off;
    }

    /// This is a standard copy constructor, copying the
    /// address space and the offset
    /// \param op2 is the Address to copy
    inline Address(const Address& op2) {
        base = op2.base;
        offset = op2.offset;
    }

    /// Determine if this is an invalid address. This only
    /// detects \e deliberate invalid addresses.
    /// \return \b true if the address is invalid
    inline bool isInvalid(void) const {
        return (base == (AddrSpace*)0);
    }

    /// Get the number of bytes needed to encode the \e offset
    /// for this address.
    /// \return the number of bytes in the encoding
    inline int4 getAddrSize(void) const {
        return addrSpaceGetAddrSize(base);
    }

    /// Determine if data stored at this address is big endian encoded.
    /// \return \b true if the address is big endian
    inline bool isBigEndian(void) const {
        return addrSpaceIsBigEndian(base);
    }

    /// Write a short-hand or debug version of this address to a
    /// stream.
    /// \param s is the stream being written
    inline void printRaw(ostream& s) const {
        if (base == (AddrSpace*)0) {
            s << "invalid_addr";
            return;
        }
        addrSpacePrintRaw(base, s, offset);
    }

    /// Convert a string into an address. The string format can be
    /// tailored for the particular address space.
    /// \param s is the string to parse
    /// \return any size associated with the parsed string
    inline int4 read(const string& s) {
        int4 sz;
        offset = addrSpaceRead(base, s, sz);
        return sz;
    }

    /// Get the address space associated with this address.
    /// \return the AddressSpace pointer, or \b NULL if invalid
    inline AddrSpace* getSpace(void) const {
        return base;
    }

    /// Get the offset of the address as an integer.
    /// \return the offset integer
    inline uintb getOffset(void) const {
        return offset;
    }

    /// Each address has a shortcut character associated with it
    /// for use with the read and printRaw methods.
    /// \return the shortcut char
    inline char getShortcut(void) const {
        return addrSpaceGetShortcut(base);
    }

    /// This is a standard assignment operator, copying the
    /// address space pointer and the offset
    /// \param op2 is the Address being assigned
    /// \return a reference to altered address
    inline Address& operator=(const Address& op2) {
        base = op2.base;
        offset = op2.offset;
        return *this;
    }

    /// Check if two addresses are equal. I.e. if their address
    /// space and offset are the same.
    /// \param op2 is the address to compare to \e this
    /// \return \b true if the addresses are the same
    inline bool operator==(const Address& op2) const {
        return ((base == op2.base) && (offset == op2.offset));
    }

    /// Check if two addresses are not equal.  I.e. if either their
    /// address space or offset are different.
    /// \param op2 is the address to compare to \e this
    /// \return \b true if the addresses are different
    inline bool operator!=(const Address& op2) const {
        return !(*this == op2);
    }

    /// Do an ordering comparison of two addresses.  Addresses are
    /// sorted first on space, then on offset.  So two addresses in
    /// the same space compare naturally based on their offset, but
    /// addresses in different spaces also compare. Different spaces
    /// are ordered by their index.
    /// \param op2 is the address to compare to
    /// \return \b true if \e this comes before \e op2
    inline bool operator<(const Address& op2) const {
        if (base != op2.base) {
            if (base == (AddrSpace*)0) {
                return true;
            } else if (base == (AddrSpace*)~((uintp)0)) {
                return false;
            } else if (op2.base == (AddrSpace*)0) {
                return false;
            } else if (op2.base == (AddrSpace*)~((uintp)0)) {
                return true;
            }
            return (addrSpaceGetIndex(base) < addrSpaceGetIndex(op2.base));
        }
        if (offset != op2.offset)
            return (offset < op2.offset);
        return false;
    }

    /// Do an ordering comparison of two addresses.
    /// \param op2 is the address to compare to
    /// \return \b true if \e this comes before or is equal to \e op2
    inline bool operator<=(const Address& op2) const {
        if (base != op2.base) {
            if (base == (AddrSpace*)0) {
                return true;
            } else if (base == (AddrSpace*)~((uintp)0)) {
                return false;
            } else if (op2.base == (AddrSpace*)0) {
                return false;
            } else if (op2.base == (AddrSpace*)~((uintp)0)) {
                return true;
            }
            return (addrSpaceGetIndex(base) < addrSpaceGetIndex(op2.base));
        }
        if (offset != op2.offset)
            return (offset < op2.offset);
        return true;
    }

    /// Add an integer value to the offset portion of the address.
    /// The addition takes into account the \e size of the address
    /// space, and the Address will wrap around if necessary.
    /// \param off is the number to add to the offset
    /// \return the new incremented address
    inline Address operator+(int8 off) const {
        return Address(base, addrSpaceWrapOffset(base, offset + off));
    }

    /// Subtract an integer value from the offset portion of the
    /// address.  The subtraction takes into account the \e size of
    /// the address space, and the Address will wrap around if
    /// necessary.
    /// \param off is the number to subtract from the offset
    /// \return the new decremented address
    inline Address operator-(int8 off) const {
        return Address(base, addrSpaceWrapOffset(base, offset - off));
    }

    friend ostream& operator<<(ostream& s, const Address& addr); ///< Write out an address to stream

    /// If the range starting at \b this address and extending for \b size bytes, encompasses bytes beyond
    /// the edge of the address space (or wraps), then return \b false.
    /// \param size is the number of bytes in the range (must be non-zero)
    /// \return \b true if the range is properly contained in the address space
    inline bool isValidRange(uint8 size) const {
        return (size - 1) <= (addrSpaceGetHighest(base) - offset);
    }

    /// Return \b true if the range starting at \b this extending the given number of bytes
    /// is contained by the second given range.
    /// \param sz is the given number of bytes in \b this range
    /// \param op2 is the start of the second given range
    /// \param sz2 is the number of bytes in the second given range
    /// \return \b true if the second given range contains \b this range
    inline bool containedBy(int4 sz, const Address& op2, int4 sz2) const {
        if (base != op2.base)
            return false;
        if (op2.offset > offset)
            return false;
        uintb off1 = offset + (sz - 1);
        uintb off2 = op2.offset + (sz2 - 1);
        return (off2 >= off1);
    }

    /// Return -1 if (\e op2,\e sz2) is not properly contained in (\e this,\e sz).
    /// If it is contained, return the endian aware offset of (\e op2,\e sz2)
    /// I.e. if the least significant byte of the \e op2 range falls on the least significant
    /// byte of the \e this range, return 0.  If it intersects the second least significant, return 1, etc.
    /// The -forceleft- toggle causes the check to be made against the left (lowest address) side
    /// of the container, regardless of the endianness.  I.e. it forces a little endian interpretation.
    /// \param sz is the size of \e this range
    /// \param op2 is the address of the second range
    /// \param sz2 is the size of the second range
    /// \param forceleft is \b true if containments is forced to be on the left even for big endian
    /// \return the endian aware offset, or -1
    inline int4 justifiedContain(int4 sz, const Address& op2, int4 sz2, bool forceleft) const {
        if (base != op2.base)
            return -1;
        if (op2.offset < offset)
            return -1;
        uintb off1 = offset + (sz - 1);
        uintb off2 = op2.offset + (sz2 - 1);
        if (off2 > off1)
            return -1;
        if (addrSpaceIsBigEndian(base) && (!forceleft))
            return (int4)(off1 - off2);
        return (int4)(op2.offset - offset);
    }

    /// If \e this + \e skip falls in the range \e op to \e op + \e size, then a non-negative integer is
    /// returned indicating where in the interval it falls. I.e. if \e this + \e skip == \e op, then 0 is returned.
    /// Otherwise, -1 is returned.
    /// \param skip is an adjust to \e this address
    /// \param op is the start of the range to check
    /// \param size is the size of the range
    /// \return an integer indicating how overlap occurs
    inline int4 overlap(int4 skip, const Address& op, int4 size) const {
        uintb dist;
        if (base != op.base)
            return -1; // Must be in same address space to overlap
        if (addrSpaceIsConstant(base))
            return -1; // Must not be constants
        dist = addrSpaceWrapOffset(base, offset + skip - op.offset);
        if (dist >= size)
            return -1; // but must fall before op+size
        return (int4)dist;
    }

    /// This method is equivalent to Address::overlap, but a range in the \e join space can be
    /// considered overlapped with its constituent pieces.
    /// If \e this + \e skip falls in the range, \e op to \e op + \e size, then a non-negative integer is
    /// returned indicating where in the interval it falls. Otherwise -1 is returned.
    /// \param skip is an adjust to \e this address
    /// \param op is the start of the range to check
    /// \param size is the size of the range
    /// \return an integer indicating how overlap occurs
    inline int4 overlapJoin(int4 skip, const Address& op, int4 size) const {
        return addrSpaceOverlapJoin(op.getSpace(), op.getOffset(), size, base, offset, skip);
    }

    /// Does the location \e this, \e sz form a contiguous region to \e loaddr, \e losz,
    /// where \e this forms the most significant piece of the logical whole
    /// \param sz is the size of \e this hi region
    /// \param loaddr is the starting address of the low region
    /// \param losz is the size of the low region
    /// \return \b true if the pieces form a contiguous whole
    inline bool isContiguous(int4 sz, const Address& loaddr, int4 losz) const {
        if (base != loaddr.base)
            return false;
        if (addrSpaceIsBigEndian(base)) {
            uintb nextoff = addrSpaceWrapOffset(base, offset + sz);
            if (nextoff == loaddr.offset)
                return true;
        } else {
            uintb nextoff = addrSpaceWrapOffset(base, loaddr.offset + losz);
            if (nextoff == offset)
                return true;
        }
        return false;
    }

    /// Determine if this address is from the \e constant \e space.
    /// All constant values are represented as an offset into the \e constant \e space.
    /// \return \b true if this address represents a constant
    inline bool isConstant(void) const {
        return addrSpaceIsConstant(base);
    }

    /// If pointers are possible anywhere within the \b size byte region return \b true, \b false otherwise.
    /// \param size is the number bytes in the region
    /// \return \b true if pointers into the region are possible
    inline bool highPtrPossible(int4 size) const {
        return addrSpaceHighPtrPossible(base, *this, size);
    }

    /// If \b this is (originally) a \e join address, reevaluate it in terms of its new
    /// \e offset and \e size, changing the space and offset if necessary.
    /// \param size is the new size in bytes of the underlying object
    inline void renormalize(int4 size) {
        if (addrSpaceIsJoin(base))
            addrSpaceRenormalizeJoinAddress(base, *this, size);
    }

    /// Determine if this address represents a set of joined memory locations.
    /// \return \b true if this address represents a join
    inline bool isJoin(void) const {
        return addrSpaceIsJoin(base);
    }

    /// Save an \<addr\> element corresponding to this address to a stream.
    /// The exact format is determined by the address space, but this generally has a space and offset attribute.
    /// \param encoder is the stream encoder
    inline void encode(Encoder& encoder) const {
        encoder.openElement(ELEM_ADDR);
        if (base != (AddrSpace*)0)
            addrSpaceEncodeAttributes(base, encoder, offset);
        encoder.closeElement(ELEM_ADDR);
    }

    /// Encode an \<addr> element corresponding to this address to a stream.
    /// The tag will also include an extra \e size attribute so that it can describe an entire memory range.
    /// \param encoder is the stream encoder
    /// \param size is the number of bytes in the range
    inline void encode(Encoder& encoder, int4 size) const {
        encoder.openElement(ELEM_ADDR);
        if (base != (AddrSpace*)0)
            addrSpaceEncodeAttributes(base, encoder, offset, size);
        encoder.closeElement(ELEM_ADDR);
    }

    /// Decode an address from a stream.
    static Address decode(Decoder& decoder) {
        AddrSpace* space;
        uintb offset;
        uint4 size;
        decodeVarnodeData(decoder, space, offset, size);
        return Address(space, offset);
    }

    /// Decode an address and size from a stream.
    static Address decode(Decoder& decoder, int4& size) {
        AddrSpace* space;
        uintb offset;
        uint4 decodedSize;
        decodeVarnodeData(decoder, space, offset, decodedSize);
        size = decodedSize;
        return Address(space, offset);
    }
};

/// \brief A partially parsed description of a Range
///
/// Class that allows \<range> tags to be parsed, when the address space doesn't yet exist
class RangeProperties {
    friend class Range;
    string spaceName; ///< Name of the address space containing the range
    uintb first;      ///< Offset of first byte in the Range
    uintb last;       ///< Offset of last byte in the Range
    bool isRegister;  ///< Range is specified a  register name
    bool seenLast;    ///< End of the range is actively specified
public:
    inline RangeProperties(void) {
        first = 0;
        last = 0;
        isRegister = false;
        seenLast = false;
    }

    /// Decode \b this from a stream
    inline void decode(Decoder& decoder) {
        uint4 elemId = decoder.openElement();
        if (elemId != ELEM_RANGE && elemId != ELEM_REGISTER)
            throw DecoderError("Expecting <range> or <register> element");
        for (;;) {
            uint4 attribId = decoder.getNextAttributeId();
            if (attribId == 0)
                break;
            if (attribId == ATTRIB_SPACE)
                spaceName = decoder.readString();
            else if (attribId == ATTRIB_FIRST)
                first = decoder.readUnsignedInteger();
            else if (attribId == ATTRIB_LAST) {
                last = decoder.readUnsignedInteger();
                seenLast = true;
            } else if (attribId == ATTRIB_NAME) {
                spaceName = decoder.readString();
                isRegister = true;
            }
        }
        decoder.closeElement(elemId);
    }
};

/// \brief A contiguous range of bytes in some address space
class Range {
    friend class RangeList;
    AddrSpace* spc; ///< Space containing range
    uintb first;    ///< Offset of first byte in \b this Range
    uintb last;     ///< Offset of last byte in \b this Range
public:
    /// \brief Construct a Range from offsets
    ///
    /// Offsets must expressed in \e bytes as opposed to addressable \e words
    /// \param s is the address space containing the range
    /// \param f is the offset of the first byte in the range
    /// \param l is the offset of the last byte in the range
    inline Range(AddrSpace* s, uintb f, uintb l) {
        spc = s;
        first = f;
        last = l;
    }
    inline Range(void) {} ///< Constructor for use with decode

    /// Construct range out of basic properties.
    inline Range(const RangeProperties& properties, const AddrSpaceManager* manage) {
        if (properties.isRegister) {
            const Translate* translator = addrSpaceManagerDefaultCodeTranslator(manage);
            uint4 size;
            translateGetRegister(translator, properties.spaceName, spc, first, size);
            last = (first - 1) + size;
            return;
        }
        spc = addrSpaceManagerGetSpaceByName(manage, properties.spaceName);
        if (spc == (AddrSpace*)0)
            throw LowlevelError("Undefined space: " + properties.spaceName);
        if (spc == (AddrSpace*)0)
            throw LowlevelError("No address space indicated in range tag");
        first = properties.first;
        last = properties.last;
        if (!properties.seenLast)
            last = addrSpaceGetHighest(spc);
        if (first > addrSpaceGetHighest(spc) || last > addrSpaceGetHighest(spc) || last < first)
            throw LowlevelError("Illegal range tag");
    }

    inline AddrSpace* getSpace(void) const {
        return spc;
    } ///< Get the address space containing \b this Range
    inline uintb getFirst(void) const {
        return first;
    } ///< Get the offset of the first byte in \b this Range
    inline uintb getLast(void) const {
        return last;
    } ///< Get the offset of the last byte in \b this Range
    inline Address getFirstAddr(void) const {
        return Address(spc, first);
    } ///< Get the address of the first byte
    inline Address getLastAddr(void) const {
        return Address(spc, last);
    } ///< Get the address of the last byte

    /// Get the last address +1, updating the space, or returning the extremal address if necessary
    /// \param manage is used to fetch the next address space
    inline Address getLastAddrOpen(const AddrSpaceManager* manage) const {
        AddrSpace* curspc = spc;
        uintb curlast = last;
        if (curlast == addrSpaceGetHighest(curspc)) {
            curspc = addrSpaceManagerGetNextSpaceInOrder(manage, curspc);
            curlast = 0;
        } else
            curlast += 1;
        if (curspc == (AddrSpace*)0)
            return Address(Address::m_maximal);
        return Address(curspc, curlast);
    }

    /// \param addr is the Address to test for containment
    /// \return \b true if addr is in \b this Range
    inline bool contains(const Address& addr) const {
        if (spc != addr.getSpace())
            return false;
        if (first > addr.getOffset())
            return false;
        if (last < addr.getOffset())
            return false;
        return true;
    }

    /// \brief Sorting operator for Ranges
    ///
    /// Compare based on address space, then the starting offset
    /// \param op2 is the Range to compare with \b this
    /// \return \b true if \b this comes before op2
    inline bool operator<(const Range& op2) const {
        if (addrSpaceGetIndex(spc) != addrSpaceGetIndex(op2.spc))
            return (addrSpaceGetIndex(spc) < addrSpaceGetIndex(op2.spc));
        return (first < op2.first);
    }

    /// Output a description of this Range like: ram: 7f-9c
    /// \param s is the output stream
    inline void printBounds(ostream& s) const {
        s << addrSpaceGetName(spc) << ": ";
        s << hex << first << '-' << last;
    }

    /// Encode \b this to a stream as a \<range> element.
    /// \param encoder is the stream encoder
    inline void encode(Encoder& encoder) const {
        encoder.openElement(ELEM_RANGE);
        encoder.writeSpace(ATTRIB_SPACE, spc);
        encoder.writeUnsignedInteger(ATTRIB_FIRST, first);
        encoder.writeUnsignedInteger(ATTRIB_LAST, last);
        encoder.closeElement(ELEM_RANGE);
    }

    /// Reconstruct this object from a \<range> or \<register> element
    /// \param decoder is the stream decoder
    inline void decode(Decoder& decoder) {
        uint4 elemId = decoder.openElement();
        if (elemId != ELEM_RANGE && elemId != ELEM_REGISTER)
            throw DecoderError("Expecting <range> or <register> element");
        decodeFromAttributes(decoder);
        decoder.closeElement(elemId);
    }

    /// Reconstruct from attributes that may not be part of a \<range> element.
    /// \param decoder is the stream decoder
    inline void decodeFromAttributes(Decoder& decoder) {
        spc = (AddrSpace*)0;
        bool seenLast = false;
        first = 0;
        last = 0;
        for (;;) {
            uint4 attribId = decoder.getNextAttributeId();
            if (attribId == 0)
                break;
            if (attribId == ATTRIB_SPACE)
                spc = decoder.readSpace();
            else if (attribId == ATTRIB_FIRST)
                first = decoder.readUnsignedInteger();
            else if (attribId == ATTRIB_LAST) {
                last = decoder.readUnsignedInteger();
                seenLast = true;
            } else if (attribId == ATTRIB_NAME) {
                const Translate* translator = addrSpaceManagerDefaultCodeTranslator(decoder.getAddrSpaceManager());
                uint4 size;
                translateGetRegister(translator, decoder.readString(), spc, first, size);
                last = (first - 1) + size;
                return; // There should be no (space,first,last) attributes
            }
        }
        if (spc == (AddrSpace*)0)
            throw LowlevelError("No address space indicated in range tag");
        if (!seenLast)
            last = addrSpaceGetHighest(spc);
        if (first > addrSpaceGetHighest(spc) || last > addrSpaceGetHighest(spc) || last < first)
            throw LowlevelError("Illegal range tag");
    }
};

/// \brief A disjoint set of Ranges, possibly across multiple address spaces
///
/// This is a container for addresses. It maintains a disjoint list of Ranges
/// that cover all the addresses in the container.  Ranges can be inserted
/// and removed, but overlapping/adjacent ranges will get merged.
class RangeList {
    set<Range> tree; ///< The sorted list of Range objects
public:
    inline RangeList(const RangeList& op2) {
        tree = op2.tree;
    } ///< Copy constructor
    inline RangeList(void) {} ///< Construct an empty container
    inline void clear(void) {
        tree.clear();
    } ///< Clear \b this container to empty
    inline bool empty(void) const {
        return tree.empty();
    } ///< Return \b true if \b this is empty
    inline set<Range>::const_iterator begin(void) const {
        return tree.begin();
    } ///< Get iterator to beginning Range
    inline set<Range>::const_iterator end(void) const {
        return tree.end();
    } ///< Get iterator to ending Range
    inline int4 numRanges(void) const {
        return tree.size();
    } ///< Return the number of Range objects in container

    /// \return the first contiguous range of addresses or NULL if empty
    inline const Range* getFirstRange(void) const {
        if (tree.empty())
            return (const Range*)0;
        return &(*tree.begin());
    }

    /// \return the last contiguous range of addresses or NULL if empty
    inline const Range* getLastRange(void) const {
        if (tree.empty())
            return (const Range*)0;
        set<Range>::const_iterator iter = tree.end();
        --iter;
        return &(*iter);
    }

    /// Treating offsets with their high-bits set as coming \e before offset where the high-bit is clear,
    /// return the last/latest contiguous Range within the given address space.
    /// \param spaceid is the given address space
    /// \return indicated Range or NULL if empty
    inline const Range* getLastSignedRange(AddrSpace* spaceid) const {
        uintb midway = addrSpaceGetHighest(spaceid) / 2; // Maximal signed value
        Range range(spaceid, midway, midway);
        set<Range>::const_iterator iter =
            tree.upper_bound(range); // First element greater than -range- (should be MOST negative)
        if (iter != tree.begin()) {
            --iter;
            if ((*iter).getSpace() == spaceid)
                return &(*iter);
        }
        range = Range(spaceid, addrSpaceGetHighest(spaceid), addrSpaceGetHighest(spaceid));
        iter = tree.upper_bound(range);
        if (iter != tree.begin()) {
            --iter;
            if ((*iter).getSpace() == spaceid)
                return &(*iter);
        }
        return (const Range*)0;
    }

    /// If \b this RangeList contains the specific address (spaceid,offset), return it
    /// \return the containing Range or NULL
    inline const Range* getRange(AddrSpace* spaceid, uintb offset) const {
        if (tree.empty())
            return (const Range*)0;
        set<Range>::const_iterator iter = tree.upper_bound(Range(spaceid, offset, offset));
        if (iter == tree.begin())
            return (const Range*)0;
        --iter;
        if ((*iter).spc != spaceid)
            return (const Range*)0;
        if ((*iter).last >= offset)
            return &(*iter);
        return (const Range*)0;
    }

    /// If this contains no Range in the given address space, null is returned.
    /// If a range contains \b offset, it is returned.
    /// Otherwise the range with a boundary point closest to \b offset is returned.
    /// If two ranges are equidistant to \b offset, the earlier Range is returned.
    /// \param spaceid is the given address space
    /// \param offset is the given offset
    /// \return the nearest Range in the same address space or null
    inline const Range* getNearestRange(AddrSpace* spaceid, uintb offset) const {
        if (tree.empty())
            return (const Range*)0;
        set<Range>::const_iterator iter = tree.upper_bound(Range(spaceid, offset, offset));
        const Range* after = (const Range*)0;
        if (iter != tree.end()) {
            after = &(*iter);
            if (after->spc != spaceid)
                after = (const Range*)0;
        }
        if (iter == tree.begin())
            return after; // nothing earlier, after is closest
        --iter;
        const Range* before = &(*iter);
        if (before->spc != spaceid)
            return after;
        if (after == (const Range*)0)
            return before;
        if (before->last >= offset)
            return before; // Range contains offset, it is closest
        uint8 distafter = after->first - offset;
        uint8 distbefore = offset - before->last;
        return (distafter < distbefore) ? after : before;
    }

    /// Insert a new Range merging as appropriate to maintain the disjoint cover.
    /// \param spc is the address space containing the new range
    /// \param first is the offset of the first byte in the new range
    /// \param last is the offset of the last byte in the new range
    inline void insertRange(AddrSpace* spc, uintb first, uintb last) {
        set<Range>::iterator iter1, iter2;
        iter1 = tree.upper_bound(Range(spc, first, first));
        if (iter1 != tree.begin()) {
            --iter1;
            if (((*iter1).spc != spc) || ((*iter1).last < first))
                ++iter1;
        }
        iter2 = tree.upper_bound(Range(spc, last, last));
        while (iter1 != iter2) {
            if ((*iter1).first < first)
                first = (*iter1).first;
            if ((*iter1).last > last)
                last = (*iter1).last;
            tree.erase(iter1++);
        }
        tree.insert(Range(spc, first, last));
    }

    inline void insertRange(const Range& rng) {
        insertRange(rng.getSpace(), rng.getFirst(), rng.getLast());
    } ///< Insert a range

    /// Remove/narrow/split existing Range objects to eliminate the indicated addresses while still maintaining a
    /// disjoint cover.
    /// \param spc is the address space of the address range to remove
    /// \param first is the offset of the first byte of the range
    /// \param last is the offset of the last byte of the range
    inline void removeRange(AddrSpace* spc, uintb first, uintb last) {
        set<Range>::iterator iter1, iter2;
        if (tree.empty())
            return; // Nothing to do
        iter1 = tree.upper_bound(Range(spc, first, first));
        if (iter1 != tree.begin()) {
            --iter1;
            if (((*iter1).spc != spc) || ((*iter1).last < first))
                ++iter1;
        }
        iter2 = tree.upper_bound(Range(spc, last, last));
        while (iter1 != iter2) {
            uintb a, b;
            a = (*iter1).first;
            b = (*iter1).last;
            tree.erase(iter1++);
            if (a < first)
                tree.insert(Range(spc, a, first - 1));
            if (b > last)
                tree.insert(Range(spc, last + 1, b));
        }
    }

    inline void removeRange(const Range& rng) {
        removeRange(rng.getSpace(), rng.getFirst(), rng.getLast());
    } ///< Remove a range

    inline void merge(const RangeList& op2) {
        set<Range>::const_iterator iter1, iter2;
        iter1 = op2.tree.begin();
        iter2 = op2.tree.end();
        while (iter1 != iter2) {
            const Range& range(*iter1);
            ++iter1;
            insertRange(range);
        }
    }

    /// Make sure indicated range of addresses is \e contained in \b this RangeList
    /// \param addr is the first Address in the target range
    /// \param size is the number of bytes in the target range
    /// \return \b true if the range is fully contained by this RangeList
    inline bool inRange(const Address& addr, uintb size) const {
        set<Range>::const_iterator iter;
        if (addr.isInvalid())
            return true; // We don't really care
        if (tree.empty())
            return false;
        iter = tree.upper_bound(Range(addr.getSpace(), addr.getOffset(), addr.getOffset()));
        if (iter == tree.begin())
            return false;
        --iter;
        if ((*iter).spc != addr.getSpace())
            return false;
        uintb end = addr.getOffset() + size - 1;
        if (end < addr.getOffset())
            return false; // size causes overflow
        if ((*iter).last >= end)
            return true;
        return false;
    }

    /// \param rng is the target range
    /// \return \b true if the range is fully contained by this RangeList
    inline bool inRange(const Range& rng) const {
        set<Range>::const_iterator iter;
        if (tree.empty())
            return false;
        iter = tree.upper_bound(rng);
        if (iter == tree.begin())
            return false;
        --iter;
        if ((*iter).spc != rng.getSpace())
            return false;
        return ((*iter).last >= rng.last);
    }

    /// Return the size of the biggest contiguous sequence of addresses in this RangeList which contain the given
    /// address.
    /// \param addr is the given address
    /// \param maxsize is the large range to consider before giving up
    /// \return the size (in bytes) of the biggest range
    inline uintb longestFit(const Address& addr, uintb maxsize) const {
        set<Range>::const_iterator iter;
        if (addr.isInvalid())
            return 0;
        if (tree.empty())
            return 0;
        uintb offset = addr.getOffset();
        iter = tree.upper_bound(Range(addr.getSpace(), offset, offset));
        if (iter == tree.begin())
            return 0;
        --iter;
        uintb sizeres = 0;
        if ((*iter).last < offset)
            return sizeres;
        do {
            if ((*iter).spc != addr.getSpace())
                break;
            if ((*iter).first > offset)
                break;
            sizeres += ((*iter).last + 1 - offset);
            offset = (*iter).last + 1;
            if (sizeres >= maxsize)
                break;
            ++iter;
        } while (iter != tree.end());
        return sizeres;
    }

    /// Print a one line description of each disjoint Range making up \b this RangeList
    /// \param s is the output stream
    inline void printBounds(ostream& s) const {
        if (tree.empty())
            s << "all" << endl;
        else {
            set<Range>::const_iterator iter;
            for (iter = tree.begin(); iter != tree.end(); ++iter) {
                (*iter).printBounds(s);
                s << endl;
            }
        }
    }

    /// Encode \b this as a \<rangelist> element
    /// \param encoder is the stream encoder
    inline void encode(Encoder& encoder) const {
        set<Range>::const_iterator iter;
        encoder.openElement(ELEM_RANGELIST);
        for (iter = tree.begin(); iter != tree.end(); ++iter)
            (*iter).encode(encoder);
        encoder.closeElement(ELEM_RANGELIST);
    }

    /// Recover each individual disjoint Range for \b this RangeList.
    /// \param decoder is the stream decoder
    inline void decode(Decoder& decoder) {
        uint4 elemId = decoder.openElement(ELEM_RANGELIST);
        while (decoder.peekElement() != 0) {
            Range range;
            range.decode(decoder);
            tree.insert(range);
        }
        decoder.closeElement(elemId);
    }
};

/// \brief An endian aware range of bits contained in a contiguous set of bytes
class BitRange {
public:
    int4 byteOffset;  ///< Byte offset of the region containing the range
    int4 byteSize;    ///< Size of the region in bytes
    int4 leastSigBit; ///< Least significant bit of the bit-range within its region
    int4 numBits;     ///< Number of bits in the range
    bool isBigEndian; ///< Is the underlying encoding big endian
    inline BitRange(void) {
        byteOffset = -1;
        byteSize = -1;
        leastSigBit = -1;
        numBits = -1;
        isBigEndian = false;
    } ///< Construct \e undefined range
    inline BitRange(int4 bOff, int4 bSize, bool bigEndian) {
        byteOffset = bOff;
        byteSize = bSize;
        leastSigBit = 0;
        numBits = bSize * 8;
        isBigEndian = bigEndian;
    } ///< Construct byte range

    /// Constructor, copy range into new container
    inline BitRange(const BitRange& op2, int4 off, int4 sz) {
        byteOffset = off;
        byteSize = sz;
        numBits = op2.numBits;
        isBigEndian = op2.isBigEndian;
        leastSigBit = translateLSB(op2);
    }

    inline BitRange(int4 bOff, int4 bSize, int4 least, int4 num, bool bigEndian) {
        byteOffset = bOff;
        byteSize = bSize;
        leastSigBit = least;
        numBits = num;
        isBigEndian = bigEndian;
    } ///< Constructor
    inline bool empty(void) const {
        return (numBits <= 0);
    } ///< Return \b true if \b this is an empty bit range (zero bits)

    /// Both the byte container and the bit range are compared and must be equal to return 0.
    /// \param op2 is the other bit range to compare with
    /// \return -1, 0, or 1 to establish ordering the two ranges
    inline int4 compare(const BitRange& op2) const {
        if (byteOffset != op2.byteOffset)
            return (byteOffset < op2.byteOffset) ? -1 : 1;
        if (byteSize != op2.byteSize)
            return (byteSize < op2.byteSize) ? -1 : 1;
        if (leastSigBit != op2.leastSigBit)
            return (leastSigBit < op2.leastSigBit) ? -1 : 1;
        if (numBits != op2.numBits)
            return (numBits < op2.numBits) ? -1 : 1;
        return 0;
    }

    /// The returned result is directly comparable with \b leastSigBit for determining order/overlap.
    /// \param op2 is the other BitRange to translate into \b this frame
    /// \return the translated value of op2.leastSigBit
    inline int4 translateLSB(const BitRange& op2) const {
        int4 op2Sig = op2.leastSigBit;
        if (isBigEndian) {
            int4 thisPos = byteOffset + byteSize;
            int4 op2Pos = op2.byteOffset + op2.byteSize;
            op2Sig += 8 * (thisPos - op2Pos);
        } else {
            op2Sig += 8 * (op2.byteOffset - byteOffset);
        }
        return op2Sig;
    }

    /// Return the intersection classification described by the original Ghidra BitRange contract.
    inline int4 overlapTest(const BitRange& op2) const {
        int4 op2Sig = translateLSB(op2);
        int4 thisMost = leastSigBit + numBits;
        int4 op2Most = op2Sig + op2.numBits;
        if (isBigEndian) {
            if (leastSigBit >= op2Most)
                return -1;
            if (op2Sig >= thisMost)
                return 1;
        } else {
            if (thisMost <= op2Sig)
                return -1;
            if (op2Most <= leastSigBit)
                return 1;
        }
        if (leastSigBit == op2Sig && thisMost == op2Most)
            return 0;
        if (op2Sig <= leastSigBit && op2Most >= thisMost)
            return 2; /// this contained in op2
        if (leastSigBit <= op2Sig && thisMost >= op2Most)
            return 3; /// op2 contained in this
        return 4;
    }

    /// The byte container for \b this does not change only \b leastSigBit and \b numBits.
    /// If the intersection is empty, \b numBits is set to 0.
    /// \param op2 is the bit range to intersect with \b this.
    inline void intersection(const BitRange& op2) {
        int4 op2Sig = translateLSB(op2);
        int4 op2Most = op2Sig + op2.numBits;
        int4 thisMost = leastSigBit + numBits;
        if (op2Sig > leastSigBit) {
            numBits -= (op2Sig - leastSigBit);
            leastSigBit = op2Sig;
        }
        if (op2Most < thisMost)
            numBits -= (thisMost - op2Most);
        if (numBits < 0) {
            leastSigBit = 0;
            numBits = 0;
        }
    }

    /// The range of bits is intersected with the 1-bits of the mask.  The resulting range is the minimal cover.
    /// \param mask is the mask to intersect with
    inline void intersectMask(uintb mask) {
        mask &= getMask();
        if (mask == 0) {
            leastSigBit = 0;
            numBits = 0;
            return;
        }
        int4 newLeastSig = leastsigbit_set(mask);
        int4 newMostSig = mostsigbit_set(mask) + 1;
        int4 thisMost = leastSigBit + numBits;
        if (newLeastSig > leastSigBit) {
            numBits -= (newLeastSig - leastSigBit);
            leastSigBit = newLeastSig;
        }
        if (newMostSig < thisMost)
            numBits -= (thisMost - newMostSig);
    }

    /// The bit range is shifted to the left by the given amount.
    /// \param leftShiftAmount is the amount to shift the range by
    inline void shift(int4 leftShiftAmount) {
        leastSigBit += leftShiftAmount;
        int4 most = leastSigBit + numBits;
        if (leastSigBit < 0) {
            numBits += leastSigBit;
            leastSigBit = 0;
        } else if (most > byteSize * 8)
            numBits -= (most - byteSize * 8);
        if (numBits < 0) {
            leastSigBit = 0;
            numBits = 0;
        }
    }

    /// The number of bits may be affected.
    /// \param num is the number of bytes to truncate
    inline void truncateMostSigBytes(int4 num) {
        if (isBigEndian)
            byteOffset += num;
        byteSize -= num;
        int4 maxOffset = leastSigBit + numBits;
        if (maxOffset > byteSize * 8)
            numBits -= (maxOffset - byteSize * 8);
        if (numBits < 0)
            numBits = 0;
    }

    /// \param num is the number of bytes to truncate
    inline void truncateLeastSigBytes(int4 num) {
        if (!isBigEndian)
            byteOffset += num;
        byteSize -= num;
        leastSigBit -= num * 8;
        if (leastSigBit < 0) {
            numBits = numBits + leastSigBit;
            leastSigBit = 0;
            if (numBits < 0)
                numBits = 0;
        }
    }

    /// Only the container is affected, the bit range itself does not change.
    /// \param num is the number of bytes to add
    inline void extendBytes(int4 num) {
        if (isBigEndian)
            byteOffset -= num;
        byteSize += num;
    }

    /// The bit-mask is aligned with the byte container.
    /// \return the bit-mask describing \b this range
    inline uintb getMask(void) const {
        uintb res;
        if (numBits >= sizeof(uintb) * 8)
            res = 0;
        else {
            res = 1;
            res <<= numBits;
        }
        res -= 1;
        res <<= leastSigBit;
        return res;
    }

    /// \return \b true if the beginning and end of the range fall on byte boundaries
    inline bool isByteRange(void) const {
        if ((numBits & 7) != 0)
            return false;
        if ((leastSigBit & 7) != 0)
            return false;
        return true;
    }

    /// \return \b true if the most significant bit of the field and the container are the same
    inline bool isMostSignificant(void) const {
        return 8 * byteSize == leastSigBit + numBits;
    }

    inline void minimizeContainer(void) {
        int4 trunc = leastSigBit / 8;
        if (isBigEndian)
            byteSize -= trunc;
        else
            byteOffset += trunc;
        leastSigBit &= 7;
        int4 num = byteSize - ((leastSigBit + numBits + 7) / 8);
        if (num > 0) {
            if (isBigEndian)
                byteOffset += num;
            byteSize -= num;
        }
    }

    inline void expandToMost(void) {
        numBits = 8 * byteSize - leastSigBit; // Increase number of bits to maximum that still fits
    }
};

/// \param size of the integer in bytes
/// \return largest unsigned integer value for given size
inline uintb calc_uint_max(int4 size) {
    return calc_mask(size);
}

/// \param size of the integer in bytes
/// \return largest signed integer value for given size
inline uintb calc_int_max(int4 size) {
    return calc_mask(size) >> 1;
}

/// \param size of the integer in bytes
/// \return smallest signed integer value for given size
inline uintb calc_int_min(int4 size) {
    return (uintb)1 << (size * 8 - 1);
}

/// Perform a CPUI_INT_RIGHT on the given val
/// \param val is the value to shift
/// \param sa is the number of bits to shift
/// \return the shifted value
inline uintb pcode_right(uintb val, int4 sa) {
    if (sa >= 8 * sizeof(uintb))
        return 0;
    return val >> sa;
}

/// Perform a CPUI_INT_LEFT on the given val
/// \param val is the value to shift
/// \param sa is the number of bits to shift
/// \return the shifted value
inline uintb pcode_left(uintb val, int4 sa) {
    if (sa >= 8 * sizeof(uintb))
        return 0;
    return val << sa;
}

/// \brief Calculate smallest mask that covers the given value
///
/// Calculcate a mask that covers either the least significant byte, uint2, uint4, or uint8,
/// whatever is smallest.
/// \param val is the given value
/// \return the minimal mask
inline uintb minimalmask(uintb val) {
    if (val > 0xffffffff)
        return ~((uintb)0);
    if (val > 0xffff)
        return 0xffffffff;
    if (val > 0xff)
        return 0xffff;
    return 0xff;
}

/// \brief Sign extend above given bit
///
/// Sign extend \b val starting at \b bit
/// \param val is the value to be sign-extended
/// \param bit is the index of the bit to extend from (0=least significant bit)
/// \return the sign extended value
inline intb sign_extend(intb val, int4 bit) {
    int4 sa = 8 * sizeof(intb) - (bit + 1);
    val = (val << sa) >> sa;
    return val;
}

/// \brief Clear all bits above given bit
///
/// Zero extend \b val starting at \b bit
/// \param val is the value to be zero extended
/// \param bit is the index of the bit to extend from (0=least significant bit)
/// \return the extended value
inline intb zero_extend(intb val, int4 bit) {
    int4 sa = sizeof(intb) * 8 - (bit + 1);
    return (intb)(((uintb)val << sa) >> sa);
}

/// This allows an Address to be written to a stream using the standard '<<' operator.
/// This is a wrapper for the printRaw method and is intended for debugging and console mode uses.
/// \param s is the stream being written to
/// \param addr is the Address to write
/// \return the output stream
inline ostream& operator<<(ostream& s, const Address& addr) {
    addr.printRaw(s);
    return s;
}

/// Treat the given \b val as a constant of \b size bytes.
/// \param val is the given value
/// \param size is the size in bytes
/// \return \b true if the constant (as sized) has its sign bit set
inline bool signbit_negative(uintb val, int4 size) {
    uintb mask = 0x80;
    mask <<= 8 * (size - 1);
    return ((val & mask) != 0);
}

/// Treat the given \b in as a constant of \b size bytes. Negate this constant keeping the upper bytes zero.
/// \param in is the given value
/// \param size is the size in bytes
/// \return the negation of the sized constant
inline uintb uintb_negate(uintb in, int4 size) {
    return ((~in) & calc_mask(size));
}

/// Take the first \b sizein bytes of the given \b in and sign-extend this to \b sizeout bytes.
/// \param in is the given value
/// \param sizein is the size to treat that value as an input
/// \param sizeout is the size to sign-extend the value to
/// \return the sign-extended value
inline uintb sign_extend(uintb in, int4 sizein, int4 sizeout) {
    sizein = (sizein < sizeof(uintb)) ? sizein : sizeof(uintb);
    sizeout = (sizeout < sizeof(uintb)) ? sizeout : sizeof(uintb);
    intb sval = in;
    sval <<= (sizeof(intb) - sizein) * 8;
    uintb res = (uintb)(sval >> (sizeout - sizein) * 8);
    res >>= (sizeof(uintb) - sizeout) * 8;
    return res;
}

/// \param val is the value to extend
/// \param numbits is the number of bits in the value
/// \param size is the integer size in bytes
/// \return the extended value
inline uintb extend_signbit(uintb val, int4 numbits, int4 size) {
    if (numbits < size * 8) {
        int4 sa = 8 * sizeof(intb) - numbits;
        intb sval = val;
        val = (sval << sa) >> sa;
        val &= calc_mask(size);
    }
    return val;
}

/// Swap the least significant \b size bytes in \b val
/// \param val is a reference to the value to swap
/// \param size is the number of bytes to swap
inline void byte_swap(intb& val, int4 size) {
    intb res = 0;
    while (size > 0) {
        res <<= 8;
        res |= (val & 0xff);
        val >>= 8;
        size -= 1;
    }
    val = res;
}

/// Swap the least significant \b size bytes in \b val
/// \param val is the value to swap
/// \param size is the number of bytes to swap
/// \return the swapped value
inline uintb byte_swap(uintb val, int4 size) {
    uintb res = 0;
    while (size > 0) {
        res <<= 8;
        res |= (val & 0xff);
        val >>= 8;
        size -= 1;
    }
    return res;
}

/// The least significant bit is index 0.
/// \param val is the given value
/// \return the index of the least significant set bit, or -1 if none are set
inline int4 leastsigbit_set(uintb val) {
    if (val == 0)
        return -1;
    int4 res = 0;
    int4 sz = 4 * sizeof(uintb);
    uintb mask = ~((uintb)0);
    do {
        mask >>= sz;
        if ((mask & val) == 0) {
            res += sz;
            val >>= sz;
        }
        sz >>= 1;
    } while (sz != 0);
    return res;
}

/// The least significant bit is index 0.
/// \param val is the given value
/// \return the index of the most significant set bit, or -1 if none are set
inline int4 mostsigbit_set(uintb val) {
    if (val == 0)
        return -1;
    int4 res = 8 * sizeof(uintb) - 1;
    int4 sz = 4 * sizeof(uintb);
    uintb mask = ~((uintb)0);
    do {
        mask <<= sz;
        if ((mask & val) == 0) {
            res -= sz;
            val <<= sz;
        }
        sz >>= 1;
    } while (sz != 0);
    return res;
}

/// Count the number (population) bits set.
/// \param val is the given value
/// \return the number of one bits
inline int4 popcount(uintb val) {
    val = (val & 0x5555555555555555L) + ((val >> 1) & 0x5555555555555555L);
    val = (val & 0x3333333333333333L) + ((val >> 2) & 0x3333333333333333L);
    val = (val & 0x0f0f0f0f0f0f0f0fL) + ((val >> 4) & 0x0f0f0f0f0f0f0f0fL);
    val = (val & 0x00ff00ff00ff00ffL) + ((val >> 8) & 0x00ff00ff00ff00ffL);
    val = (val & 0x0000ffff0000ffffL) + ((val >> 16) & 0x0000ffff0000ffffL);
    int4 res = (int4)(val & 0xff);
    res += (int4)((val >> 32) & 0xff);
    return res;
}

/// Count the number of more significant zero bits before the most significant one bit in the representation of val.
/// \param val is the given value
/// \return the number of zero bits
inline int4 count_leading_zeros(uintb val) {
    if (val == 0)
        return 8 * sizeof(uintb);
    uintb mask = ~((uintb)0);
    int4 maskSize = 4 * sizeof(uintb);
    mask &= (mask << maskSize);
    int4 bit = 0;
    do {
        if ((mask & val) == 0) {
            bit += maskSize;
            maskSize >>= 1;
            mask |= (mask >> maskSize);
        } else {
            maskSize >>= 1;
            mask &= (mask << maskSize);
        }
    } while (maskSize != 0);
    return bit;
}

/// Return smallest number of form 2^n-1, bigger or equal to the given value
/// \param val is the given value
/// \return the mask
inline uintb coveringmask(uintb val) {
    uintb res = val;
    int4 sz = 1;
    while (sz < 8 * sizeof(uintb)) {
        res = res | (res >> sz);
        sz <<= 1;
    }
    return res;
}

/// Treat \b val as a constant of size \b sz and count its bit transitions.
/// \param val is the given value
/// \param sz is the size to treat the value as
/// \return the number of transitions
inline int4 bit_transitions(uintb val, int4 sz) {
    int4 res = 0;
    int4 last = val & 1;
    int4 cur;
    for (int4 i = 1; i < 8 * sz; ++i) {
        val >>= 1;
        cur = val & 1;
        if (cur != last) {
            res += 1;
            last = cur;
        }
        if (val == 0)
            break;
    }
    return res;
}

AttributeId ATTRIB_FIRST = AttributeId("first", 27);
AttributeId ATTRIB_LAST = AttributeId("last", 28);

} // End namespace ghidra
