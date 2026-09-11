module;
#include <cstdlib>
#include <ostream>
#include <sstream>
#include <string>

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
/// \file space.cppm
/// \brief Classes for describing address spaces

#ifndef __SPACE_HH__
#define __SPACE_HH__

#include <string>

export module sleigh_runtime.ghidra:space;
export import :error;
export import :marshal;
export import :varnode;

export namespace ghidra {

/// \brief Fundemental address space types
///
/// Every address space must be one of the following core types
enum spacetype {
    IPTR_CONSTANT = 0,  ///< Special space to represent constants
    IPTR_PROCESSOR = 1, ///< Normal spaces modelled by processor
    IPTR_SPACEBASE = 2, ///< addresses = offsets off of base register
    IPTR_INTERNAL = 3,  ///< Internally managed temporary space
    IPTR_FSPEC = 4,     ///< Special internal FuncCallSpecs reference
    IPTR_IOP = 5,       ///< Special internal PcodeOp reference
    IPTR_JOIN = 6       ///< Special virtual space to represent split variables
};

class AddrSpace;
class AddrSpaceManager;
class Address;
struct VarnodeData;
class Translate;
class JoinRecord;

/// Accessors keep the space partition independent from the translate
/// partition, which derives from the address-space manager defined here.
JoinRecord* addrSpaceManagerFindJoin(const AddrSpaceManager* manager, uintb offset);
JoinRecord* addrSpaceManagerFindAddJoin(const AddrSpaceManager* manager, const vector<VarnodeData>& pieces,
                                        int4 logicalSize);
AddrSpace* addrSpaceManagerGetSpaceByShortcut(const AddrSpaceManager* manager, char shortcut);
int4 addrSpaceManagerDefaultSize(const AddrSpaceManager* manager);
bool translateIsBigEndian(const Translate* translator);
Address joinRecordGetEquivalentAddress(const JoinRecord* record, uintb offset, int4& position);
int4 joinRecordNumPieces(const JoinRecord* record);
const VarnodeData& joinRecordGetPiece(const JoinRecord* record, int4 position);
void joinRecordGetUnified(const JoinRecord* record, AddrSpace*& space, uintb& offset, uint4& size);

inline int4 get_offset_size(const char* ptr, uintb& offset);

extern AttributeId ATTRIB_BASE;          ///< Marshaling attribute "base"
extern AttributeId ATTRIB_DEADCODEDELAY; ///< Marshaling attribute "deadcodedelay"
extern AttributeId ATTRIB_DELAY;         ///< Marshaling attribute "delay"
extern AttributeId ATTRIB_LOGICALSIZE;   ///< Marshaling attribute "logicalsize"
extern AttributeId ATTRIB_PHYSICAL;      ///< Marshaling attribute "physical"
extern AttributeId ATTRIB_PIECE;         ///< Marshaling attribute "piece"
extern ElementId ELEM_SPACE_OVERLAY;     ///< Marshaling element <space_overlay>

/// \brief A region where processor data is stored
///
/// An AddrSpace (Address Space) is an arbitrary sequence of bytes where a processor can store data.
class AddrSpace {
    friend class AddrSpaceManager; // Space container
    friend class Address;
    friend bool addrSpaceHighPtrPossible(const AddrSpace*, const Address&, int4);
    friend void addrSpaceRenormalizeJoinAddress(const AddrSpace*, Address&, int4);

public:
    enum {
        big_endian = 1,             ///< Space is big endian if set, little endian otherwise
        heritaged = 2,              ///< This space is heritaged
        does_deadcode = 4,          ///< Dead-code analysis is done on this space
        programspecific = 8,        ///< Space is specific to a particular loadimage
        reverse_justification = 16, ///< Justification within aligned word is opposite of endianness
        formal_stackspace = 0x20,   ///< Space attached to the formal \b stack \b pointer
        overlay = 0x40,             ///< This space is an overlay of another space
        overlaybase = 0x80,         ///< This is the base space for overlay space(s)
        truncated = 0x100,          ///< Space is truncated from its original size, expect pointers larger than this size
        hasphysical = 0x200,        ///< Has physical memory associated with it
        is_otherspace = 0x400,      ///< Quick check for the OtherSpace derived class
        has_nearpointers = 0x800,   ///< Does there exist near pointers into this space
        allows_wrapped_range = 0x1000, ///< A memory range for \b this space can wrap from high addresses to low
        addressable_all = 0x2000,      ///< Pointers can address the entire space
        addressable_none = 0x4000      ///< Pointers do not exist into \b this space
    };

private:
    const Translate* trans;  ///< Processor translator (for register names etc) for this space
    spacetype type;          ///< Type of space (PROCESSOR, CONSTANT, INTERNAL, ...)
    int4 refcount;           ///< Number of managers using this space
    uint4 flags;             ///< Attributes of the space
    char shortcut;           ///< Shortcut character for printing
    uintb highest;            ///< Highest (byte) offset into this space
    uintb pointerLowerBound; ///< Offset below which we don't search for pointers
    uintb pointerUpperBound; ///< Offset above which we don't search for pointers
protected:
    AddrSpaceManager* manage; ///< Manager for processor using this space
    string name;              ///< Name of this space
    uint4 addressSize;        ///< Size of an address into this space in bytes
    uint4 wordsize;           ///< Size of unit being addressed (1=byte)
    int4 minimumPointerSize;  ///< Smallest size of a pointer into \b this space (in bytes)
    int4 index;               ///< An integer identifier for the space
    int4 delay;               ///< Delay in heritaging this space
    int4 deadcodedelay;       ///< Delay before deadcode removal is allowed on this space

    /// Calculate \e highest based on \e addressSize, and \e wordsize.
    /// This also calculates the default pointerLowerBound.
    inline void calcScaleMask(void) {
        highest = calc_mask(addressSize);              // Maximum address
        highest = highest * wordsize + (wordsize - 1); // Maximum byte address
        pointerLowerBound = 0;
        pointerUpperBound = highest;
        uintb bufferSize = (addressSize < 3) ? 0x100 : 0x1000;
        pointerLowerBound += bufferSize;
        pointerUpperBound -= bufferSize;
    }

    /// An internal method for derived classes to set space attributes.
    /// \param fl is the set of attributes to be set
    inline void setFlags(uint4 fl) {
        flags |= fl;
    }

    /// An internal method for derived classes to clear space attributes.
    /// \param fl is the set of attributes to clear
    inline void clearFlags(uint4 fl) {
        flags &= ~fl;
    }

    /// Walk attributes of the current element and recover all the properties defining this space.
    /// \param decoder is the stream decoder
    inline void decodeBasicAttributes(Decoder& decoder) {
        deadcodedelay = -1;
        for (;;) {
            uint4 attribId = decoder.getNextAttributeId();
            if (attribId == 0)
                break;
            if (attribId == ATTRIB_NAME)
                name = decoder.readString();
            if (attribId == ATTRIB_INDEX)
                index = decoder.readSignedInteger();
            else if (attribId == ATTRIB_SIZE)
                addressSize = decoder.readSignedInteger();
            else if (attribId == ATTRIB_WORDSIZE)
                wordsize = decoder.readUnsignedInteger();
            else if (attribId == ATTRIB_BIGENDIAN) {
                if (decoder.readBool())
                    flags |= big_endian;
            } else if (attribId == ATTRIB_DELAY)
                delay = decoder.readSignedInteger();
            else if (attribId == ATTRIB_DEADCODEDELAY)
                deadcodedelay = decoder.readSignedInteger();
            else if (attribId == ATTRIB_PHYSICAL) {
                if (decoder.readBool())
                    flags |= hasphysical;
            }
        }
        if (deadcodedelay == -1)
            deadcodedelay = delay; // If deadcodedelay attribute not present, set it to delay
        calcScaleMask();
    }

    inline void truncateSpace(uint4 newsize) {
        setFlags(truncated);
        addressSize = newsize;
        minimumPointerSize = newsize;
        calcScaleMask();
    }

public:
    /// Initialize an address space with its basic attributes.
    inline AddrSpace(AddrSpaceManager* m, const Translate* t, spacetype tp, const string& nm, bool bigEnd, uint4 size,
                     uint4 ws, int4 ind, uint4 fl, int4 dl, int4 dead) {
        refcount = 0; // No references to this space yet
        manage = m;
        trans = t;
        type = tp;
        name = nm;
        addressSize = size;
        wordsize = ws;
        index = ind;
        delay = dl;
        deadcodedelay = dead;
        minimumPointerSize = 0; // (initially) assume pointers must match the space size exactly
        shortcut = ' ';         // Placeholder meaning shortcut is unassigned
        flags = (fl & hasphysical);
        if (bigEnd)
            flags |= big_endian;
        flags |= (heritaged | does_deadcode);
        calcScaleMask();
        flags |= (delay == 0) ? addressable_none : addressable_all;
    }

    /// This is a partial constructor, for initializing a space via XML.
    inline AddrSpace(AddrSpaceManager* m, const Translate* t, spacetype tp) {
        refcount = 0;
        manage = m;
        trans = t;
        type = tp;
        flags = (heritaged | does_deadcode);
        wordsize = 1;
        minimumPointerSize = 0;
        shortcut = ' ';
    }

    virtual ~AddrSpace(void) {} ///< The address space destructor

    /// Every address space has a unique name, referred to especially in configuration files via XML.
    /// \return the name of this space
    inline const string& getName(void) const {
        return name;
    }
    /// \return a pointer to the Translate object associated with this space
    inline const Translate* getTrans(void) const {
        return trans;
    }
    inline spacetype getType(void) const {
        return type;
    } ///< Get the type of space
    inline int4 getDelay(void) const {
        return delay;
    } ///< Get number of heritage passes being delayed
    inline int4 getDeadcodeDelay(void) const {
        return deadcodedelay;
    } ///< Get number of passes before deadcode removal is allowed
    inline int4 getIndex(void) const {
        return index;
    } ///< Get the integer identifier
    inline uint4 getWordSize(void) const {
        return wordsize;
    } ///< Get the addressable unit size
    inline uint4 getAddrSize(void) const {
        return addressSize;
    } ///< Get the size of the space
    inline uintb getHighest(void) const {
        return highest;
    } ///< Get the highest byte-scaled address
    inline uintb wrapOffset(uintb off) const {
        if (off <= highest)
            return off;
        intb mod = (intb)(highest + 1);
        intb res = (intb)off % mod;
        if (res < 0)
            res += mod;
        return (uintb)res;
    } ///< Wrap -off- to the offset that fits into this space
    inline char getShortcut(void) const {
        return shortcut;
    } ///< Get the shortcut character
    inline bool isHeritaged(void) const {
        return ((flags & heritaged) != 0);
    } ///< Return \b true if dataflow has been traced
    inline bool hasPhysical(void) const {
        return ((flags & hasphysical) != 0);
    } ///< Return \b true if data is physically stored in this
    inline bool isBigEndian(void) const {
        return ((flags & big_endian) != 0);
    } ///< Return \b true if values in this space are big endian
    inline bool isFormalStackSpace(void) const {
        return ((flags & formal_stackspace) != 0);
    } ///< Return \b true if this is attached to the formal stack pointer
    inline bool isOverlay(void) const {
        return ((flags & overlay) != 0);
    } ///< Return \b true if this is an overlay space
    inline bool isOtherSpace(void) const {
        return ((flags & is_otherspace) != 0);
    } ///< Return \b true if this is the other address space
    inline bool allowsWrappedRange(void) const {
        return ((flags & allows_wrapped_range) != 0);
    } ///< Return \b true if memory range can span high to low addresses

    /// Print the offset as hexadecimal digits.
    inline void printOffset(ostream& s, uintb offset) const {
        s << "0x" << hex << offset;
    }

    /// If this is not the join space, null is returned, otherwise look up any record associated with offset.
    inline JoinRecord* findJoin(uintb offset) const {
        if (type != IPTR_JOIN)
            return (JoinRecord*)0;
        return addrSpaceManagerFindJoin(manage, offset);
    }

    inline virtual int4 numSpacebase(void) const {
        return 0;
    } ///< Number of base registers associated with this space
    inline virtual const VarnodeData& getSpacebase(int4 i) const {
        throw LowlevelError(name + " space is not virtual and has no associated base register");
    } ///< Get a base register that creates this virtual space
    inline virtual const VarnodeData& getSpacebaseFull(int4 i) const {
        throw LowlevelError(name + " has no truncated registers");
    } ///< Return original spacebase register before truncation
    inline virtual bool stackGrowsNegative(void) const {
        return true;
    } ///< Return \b true if a stack in this space grows negative
    inline virtual AddrSpace* getContain(void) const {
        return (AddrSpace*)0;
    } ///< Return this space's containing space (if any)

    inline virtual int4 overlapJoin(uintb offset, int4 size, AddrSpace* pointSpace, uintb pointOff, int4 pointSkip) const {
        if (this != pointSpace)
            return -1;
        uintb dist = wrapOffset(pointOff + pointSkip - offset);
        if (dist >= size)
            return -1; // but must fall before op+size
        return (int4)dist;
    }

    inline virtual void encodeAttributes(Encoder& encoder, uintb offset) const {
        encoder.writeSpace(ATTRIB_SPACE, this);
        encoder.writeUnsignedInteger(ATTRIB_OFFSET, offset);
    }

    inline virtual void encodeAttributes(Encoder& encoder, uintb offset, int4 size) const {
        encoder.writeSpace(ATTRIB_SPACE, this);
        encoder.writeUnsignedInteger(ATTRIB_OFFSET, offset);
        encoder.writeSignedInteger(ATTRIB_SIZE, size);
    }

    inline virtual uintb decodeAttributes(Decoder& decoder, uint4& size) const {
        uintb offset;
        bool foundoffset = false;
        for (;;) {
            uint4 attribId = decoder.getNextAttributeId();
            if (attribId == 0)
                break;
            if (attribId == ATTRIB_OFFSET) {
                foundoffset = true;
                offset = decoder.readUnsignedInteger();
            } else if (attribId == ATTRIB_SIZE)
                size = decoder.readSignedInteger();
        }
        if (!foundoffset)
            throw LowlevelError("Address is missing offset");
        return offset;
    }

    inline virtual void printRaw(ostream& s, uintb offset) const {
        int4 sz = getAddrSize();
        if (sz > 4) {
            if ((offset >> 32) == 0)
                sz = 4;
            else if ((offset >> 48) == 0)
                sz = 6;
        }
        s << "0x" << setfill('0') << setw(2 * sz) << hex << byteToAddress(offset, wordsize);
        if (wordsize > 1) {
            int4 cut = offset % wordsize;
            if (cut != 0)
                s << '+' << dec << cut;
        }
    }

    inline virtual uintb read(const string& s, int4& size) const {
        const char* enddata;
        char* tmpdata;
        int4 expsize;
        string::size_type append;
        string frontpart;
        uintb offset;
        append = s.find_first_of(":+");
        try {
            if (append == string::npos) {
                AddrSpace* registerSpace;
                uint4 registerSize;
                translateGetRegister(trans, s, registerSpace, offset, registerSize);
                size = registerSize;
            } else {
                frontpart = s.substr(0, append);
                AddrSpace* registerSpace;
                uint4 registerSize;
                translateGetRegister(trans, frontpart, registerSpace, offset, registerSize);
                size = registerSize;
            }
        } catch (LowlevelError&) {
            offset = strtoul(s.c_str(), &tmpdata, 0);
            offset = addressToByte(offset, wordsize);
            enddata = (const char*)tmpdata;
            if (enddata - s.c_str() == s.size()) {
                size = addrSpaceManagerDefaultSize(manage);
                return offset;
            }
            size = addrSpaceManagerDefaultSize(manage);
        }
        if (append != string::npos) {
            enddata = s.c_str() + append;
            expsize = get_offset_size(enddata, offset);
            if (expsize != -1) {
                size = expsize;
                return offset;
            }
        }
        return offset;
    }

    inline virtual void decode(Decoder& decoder) {
        uint4 elemId = decoder.openElement();
        decodeBasicAttributes(decoder);
        decoder.closeElement(elemId);
    }

    inline static uintb addressToByte(uintb val, uint4 ws) {
        return val * ws;
    } ///< Scale from addressable units to byte units
    inline static uintb byteToAddress(uintb val, uint4 ws) {
        return val / ws;
    } ///< Scale from byte units to addressable units
    inline static int8 addressToByteInt(int8 val, uint4 ws) {
        return val * ws;
    } ///< Scale int8 from addressable units to byte units
    inline static int8 byteToAddressInt(int8 val, uint4 ws) {
        return val / ws;
    } ///< Scale int8 from byte units to addressable units
    inline static bool compareByIndex(const AddrSpace* a, const AddrSpace* b) {
        return (a->index < b->index);
    } ///< Compare two spaces by their index
};

inline int4 get_offset_size(const char* ptr, uintb& offset) { // Get optional size and offset fields from string
    int4 size;
    uint4 val;
    char* ptr2;
    val = 0;
    size = -1;
    if (*ptr == ':') {
        size = strtoul(ptr + 1, &ptr2, 0);
        if (*ptr2 == '+')
            val = strtoul(ptr2 + 1, &ptr2, 0);
    }
    if (*ptr == '+')
        val = strtoul(ptr + 1, &ptr2, 0);
    offset += val;
    return size;
}

/// \brief Special AddrSpace for representing constants during analysis.
class ConstantSpace : public AddrSpace {
public:
    inline ConstantSpace(AddrSpaceManager* m, const Translate* t)
        : AddrSpace(m, t, IPTR_CONSTANT, NAME, false, sizeof(uintb), 1, INDEX, 0, 0, 0) {
        clearFlags(heritaged | does_deadcode | big_endian);
        if (HOST_ENDIAN == 1)
            setFlags(big_endian);
    }
    inline int4 overlapJoin(uintb offset, int4 size, AddrSpace* pointSpace, uintb pointOff, int4 pointSkip) const override {
        return -1;
    }
    inline void printRaw(ostream& s, uintb offset) const override {
        s << "0x" << hex << offset;
    }
    inline void decode(Decoder& decoder) override {
        throw LowlevelError("Should never decode the constant space");
    }
    static const string NAME; ///< Reserved name for the address space
    static const int4 INDEX;  ///< Reserved index for constant space
};

/// \brief Special AddrSpace for special/user-defined address spaces
class OtherSpace : public AddrSpace {
public:
    inline OtherSpace(AddrSpaceManager* m, const Translate* t, int4 ind)
        : AddrSpace(m, t, IPTR_PROCESSOR, NAME, false, sizeof(uintb), 1, INDEX, 0, 0, 0) {
        clearFlags(heritaged | does_deadcode);
        setFlags(is_otherspace);
    }
    inline OtherSpace(AddrSpaceManager* m, const Translate* t) : AddrSpace(m, t, IPTR_PROCESSOR) {
        clearFlags(heritaged | does_deadcode);
        setFlags(is_otherspace);
    }
    inline void printRaw(ostream& s, uintb offset) const override {
        s << "0x" << hex << offset;
    }
    static const string NAME; ///< Reserved name for the other space
    static const int4 INDEX;  ///< Reserved index for the other space
};

/// \brief The pool of temporary storage registers
class UniqueSpace : public AddrSpace {
public:
    inline UniqueSpace(AddrSpaceManager* m, const Translate* t, int4 ind, uint4 fl)
        : AddrSpace(m, t, IPTR_INTERNAL, NAME, translateIsBigEndian(t), SIZE, 1, ind, fl, 0, 0) {
        setFlags(hasphysical);
    }
    inline UniqueSpace(AddrSpaceManager* m, const Translate* t) : AddrSpace(m, t, IPTR_INTERNAL) {
        setFlags(hasphysical);
    }
    static const string NAME; ///< Reserved name for the unique space
    static const uint4 SIZE; ///< Fixed size for unique space offsets
};

/// \brief The pool of logically joined variables
class JoinSpace : public AddrSpace {
    static const int4 MAX_PIECES = 64; ///< Maximum number of pieces that can be marshaled in one join address
public:
    inline JoinSpace(AddrSpaceManager* m, const Translate* t, int4 ind)
        : AddrSpace(m, t, IPTR_JOIN, NAME, translateIsBigEndian(t), sizeof(uintm), 1, ind, 0, 0, 0) {
        clearFlags(heritaged); // This space is never heritaged, but does dead-code analysis
    }

    inline int4 overlapJoin(uintb offset, int4 size, AddrSpace* pointSpace, uintb pointOffset, int4 pointSkip) const override {
        if (this == pointSpace) {
            JoinRecord* pieceRecord = addrSpaceManagerFindJoin(manage, pointOffset);
            int4 pos;
            Address addr = joinRecordGetEquivalentAddress(pieceRecord, pointOffset + pointSkip, pos);
            pointSpace = addr.getSpace();
            pointOffset = addr.getOffset();
        } else {
            if (pointSpace->getType() == IPTR_CONSTANT)
                return -1;
            pointOffset = pointSpace->wrapOffset(pointOffset + pointSkip);
        }
        JoinRecord* joinRecord = addrSpaceManagerFindJoin(manage, offset);
        int4 startPiece, endPiece, dir;
        if (isBigEndian()) {
            startPiece = 0;
            endPiece = joinRecordNumPieces(joinRecord);
            dir = 1;
        } else {
            startPiece = joinRecordNumPieces(joinRecord) - 1;
            endPiece = -1;
            dir = -1;
        }
        int4 bytesAccum = 0;
        for (int4 i = startPiece; i != endPiece; i += dir) {
            const VarnodeData& vData(joinRecordGetPiece(joinRecord, i));
            if (vData.space == pointSpace && pointOffset >= vData.offset &&
                pointOffset <= vData.offset + (vData.size - 1)) {
                int4 res = (int4)(pointOffset - vData.offset) + bytesAccum;
                if (res >= size)
                    return -1;
                return res;
            }
            bytesAccum += vData.size;
        }
        return -1;
    }

    inline void encodeAttributes(Encoder& encoder, uintb offset) const override {
        JoinRecord* rec = addrSpaceManagerFindJoin(manage, offset);
        encoder.writeSpace(ATTRIB_SPACE, this);
        int4 num = joinRecordNumPieces(rec);
        if (num > MAX_PIECES)
            throw LowlevelError("Exceeded maximum pieces in one join address");
        for (int4 i = 0; i < num; ++i) {
            const VarnodeData& vdata(joinRecordGetPiece(rec, i));
            ostringstream t;
            t << vdata.space->getName() << ":0x";
            t << hex << vdata.offset << ':' << dec << vdata.size;
            encoder.writeStringIndexed(ATTRIB_PIECE, i, t.str());
        }
        if (num == 1) {
            AddrSpace* unifiedSpace;
            uintb unifiedOffset;
            uint4 unifiedSize;
            joinRecordGetUnified(rec, unifiedSpace, unifiedOffset, unifiedSize);
            encoder.writeUnsignedInteger(ATTRIB_LOGICALSIZE, unifiedSize);
        }
    }

    inline void encodeAttributes(Encoder& encoder, uintb offset, int4 size) const override {
        encodeAttributes(encoder, offset); // Ignore size
    }

    inline uintb decodeAttributes(Decoder& decoder, uint4& size) const override {
        vector<VarnodeData> pieces;
        uint4 sizesum = 0;
        uint4 logicalsize = 0;
        for (;;) {
            uint4 attribId = decoder.getNextAttributeId();
            if (attribId == 0)
                break;
            if (attribId == ATTRIB_LOGICALSIZE) {
                logicalsize = decoder.readUnsignedInteger();
                continue;
            } else if (attribId == ATTRIB_UNKNOWN)
                attribId = decoder.getIndexedAttributeId(ATTRIB_PIECE);
            if (attribId < ATTRIB_PIECE.getId())
                continue;
            int4 pos = (int4)(attribId - ATTRIB_PIECE.getId());
            if (pos > MAX_PIECES)
                continue;
            while (pieces.size() <= pos)
                pieces.emplace_back();
            VarnodeData& vdat(pieces[pos]);
            string attrVal = decoder.readString();
            string::size_type offpos = attrVal.find(':');
            if (offpos == string::npos) {
                AddrSpace* registerSpace;
                uintb registerOffset;
                uint4 registerSize;
                translateGetRegister(getTrans(), attrVal, registerSpace, registerOffset, registerSize);
                vdat.space = registerSpace;
                vdat.offset = registerOffset;
                vdat.size = registerSize;
            } else {
                string::size_type szpos = attrVal.find(':', offpos + 1);
                if (szpos == string::npos)
                    throw LowlevelError("join address piece attribute is malformed");
                string spcname = attrVal.substr(0, offpos);
                vdat.space = addrSpaceManagerGetSpaceByName(manage, spcname);
                istringstream s1(attrVal.substr(offpos + 1, szpos));
                s1.unsetf(ios::dec | ios::hex | ios::oct);
                s1 >> vdat.offset;
                istringstream s2(attrVal.substr(szpos + 1));
                s2.unsetf(ios::dec | ios::hex | ios::oct);
                s2 >> vdat.size;
            }
            sizesum += vdat.size;
        }
        JoinRecord* rec = addrSpaceManagerFindAddJoin(manage, pieces, logicalsize);
        AddrSpace* unifiedSpace;
        uintb unifiedOffset;
        uint4 unifiedSize;
        joinRecordGetUnified(rec, unifiedSpace, unifiedOffset, unifiedSize);
        size = unifiedSize;
        return unifiedOffset;
    }

    inline void printRaw(ostream& s, uintb offset) const override {
        JoinRecord* rec = addrSpaceManagerFindJoin(manage, offset);
        int4 szsum = 0;
        int4 num = joinRecordNumPieces(rec);
        s << '{';
        for (int4 i = 0; i < num; ++i) {
            const VarnodeData& vdat(joinRecordGetPiece(rec, i));
            szsum += vdat.size;
            if (i != 0)
                s << ',';
            vdat.space->printRaw(s, vdat.offset);
        }
        if (num == 1) {
            AddrSpace* unifiedSpace;
            uintb unifiedOffset;
            uint4 unifiedSize;
            joinRecordGetUnified(rec, unifiedSpace, unifiedOffset, unifiedSize);
            szsum = unifiedSize;
            s << ':' << szsum;
        }
        s << '}';
    }

    inline uintb read(const string& s, int4& size) const override {
        vector<VarnodeData> pieces;
        int4 szsum = 0;
        int4 i = 0;
        while (i < s.size()) {
            pieces.emplace_back();
            string token;
            while ((i < s.size()) && (s[i] != ',')) {
                token += s[i];
                i += 1;
            }
            i += 1;
            try {
                AddrSpace* registerSpace;
                uintb registerOffset;
                uint4 registerSize;
                translateGetRegister(getTrans(), token, registerSpace, registerOffset, registerSize);
                pieces.back().space = registerSpace;
                pieces.back().offset = registerOffset;
                pieces.back().size = registerSize;
            } catch (LowlevelError&) {
                char tryShortcut = token[0];
                AddrSpace* spc = addrSpaceManagerGetSpaceByShortcut(manage, tryShortcut);
                if (spc == (AddrSpace*)0)
                    throw LowlevelError("Could not parse join string");
                int4 subsize;
                pieces.back().space = spc;
                pieces.back().offset = spc->read(token.substr(1), subsize);
                pieces.back().size = subsize;
            }
            szsum += pieces.back().size;
        }
        JoinRecord* rec = addrSpaceManagerFindAddJoin(manage, pieces, 0);
        size = szsum;
        AddrSpace* unifiedSpace;
        uintb unifiedOffset;
        uint4 unifiedSize;
        joinRecordGetUnified(rec, unifiedSpace, unifiedOffset, unifiedSize);
        return unifiedOffset;
    }

    inline void decode(Decoder& decoder) override {
        throw LowlevelError("Should never decode join space");
    }
    static const string NAME; ///< Reserved name for the join space
};

/// \brief An overlay space.
class OverlaySpace : public AddrSpace {
    AddrSpace* baseSpace; ///< Space being overlayed
public:
    inline OverlaySpace(AddrSpaceManager* m, const Translate* t) : AddrSpace(m, t, IPTR_PROCESSOR) {
        baseSpace = (AddrSpace*)0;
        setFlags(overlay);
    }
    inline AddrSpace* getContain(void) const override {
        return baseSpace;
    }
    inline void decode(Decoder& decoder) override {
        uint4 elemId = decoder.openElement(ELEM_SPACE_OVERLAY);
        name = decoder.readString(ATTRIB_NAME);
        index = decoder.readSignedInteger(ATTRIB_INDEX);
        baseSpace = decoder.readSpace(ATTRIB_BASE);
        decoder.closeElement(elemId);
        addressSize = baseSpace->getAddrSize();
        wordsize = baseSpace->getWordSize();
        delay = baseSpace->getDelay();
        deadcodedelay = baseSpace->getDeadcodeDelay();
        calcScaleMask();
        if (baseSpace->isBigEndian())
            setFlags(big_endian);
        if (baseSpace->hasPhysical())
            setFlags(hasphysical);
    }
};

/// Exposes the concrete address-space operations required by the address
/// partition without importing this partition back into it.
int4 addrSpaceGetAddrSize(const AddrSpace* space) { return space->getAddrSize(); }
bool addrSpaceIsBigEndian(const AddrSpace* space) { return space->isBigEndian(); }
void addrSpacePrintRaw(const AddrSpace* space, ostream& stream, uintb offset) { space->printRaw(stream, offset); }
uintb addrSpaceRead(const AddrSpace* space, const string& text, int4& size) { return space->read(text, size); }
char addrSpaceGetShortcut(const AddrSpace* space) { return space->getShortcut(); }
const string& addrSpaceGetName(const AddrSpace* space) { return space->getName(); }
int4 addrSpaceGetIndex(const AddrSpace* space) { return space->getIndex(); }
uintb addrSpaceWrapOffset(const AddrSpace* space, uintb offset) { return space->wrapOffset(offset); }
uintb addrSpaceGetHighest(const AddrSpace* space) { return space->getHighest(); }
uintb addrSpaceDecodeAttributes(const AddrSpace* space, Decoder& decoder, uint4& size) {
    return space->decodeAttributes(decoder, size);
}
bool addrSpaceIsConstant(const AddrSpace* space) { return space->getType() == IPTR_CONSTANT; }
bool addrSpaceIsJoin(const AddrSpace* space) { return space->getType() == IPTR_JOIN; }
int4 addrSpaceOverlapJoin(const AddrSpace* space, uintb offset, int4 size, AddrSpace* pointSpace, uintb pointOffset,
                          int4 pointSkip) {
    return space->overlapJoin(offset, size, pointSpace, pointOffset, pointSkip);
}
void addrSpaceEncodeAttributes(const AddrSpace* space, Encoder& encoder, uintb offset) {
    space->encodeAttributes(encoder, offset);
}
void addrSpaceEncodeAttributes(const AddrSpace* space, Encoder& encoder, uintb offset, int4 size) {
    space->encodeAttributes(encoder, offset, size);
}

const string ConstantSpace::NAME = "const";
const int4 ConstantSpace::INDEX = 0;
const string OtherSpace::NAME = "OTHER";
const int4 OtherSpace::INDEX = 1;
const string UniqueSpace::NAME = "unique";
const uint4 UniqueSpace::SIZE = 4;
const string JoinSpace::NAME = "join";

AttributeId ATTRIB_BASE = AttributeId("base", 89);
AttributeId ATTRIB_DEADCODEDELAY = AttributeId("deadcodedelay", 90);
AttributeId ATTRIB_DELAY = AttributeId("delay", 91);
AttributeId ATTRIB_LOGICALSIZE = AttributeId("logicalsize", 92);
AttributeId ATTRIB_PHYSICAL = AttributeId("physical", 93);

} // End namespace ghidra
#endif
