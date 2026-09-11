module;
#include <algorithm>
#include <cstdint>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

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
/// \file translate.cppm
/// \brief Classes for disassembly and pcode generation
///
/// Classes for keeping track of spaces and registers (for a single architecture).

#ifndef __TRANSLATE_HH__
#define __TRANSLATE_HH__

export module sleigh_runtime.ghidra:translate;
export import :varnode;
export import :space;

export namespace ghidra {

extern AttributeId ATTRIB_CODE;         ///< Marshaling attribute "code"
extern AttributeId ATTRIB_CONTAIN;      ///< Marshaling attribute "contain"
extern AttributeId ATTRIB_DEFAULTSPACE; ///< Marshaling attribute "defaultspace"
extern AttributeId ATTRIB_UNIQBASE;     ///< Marshaling attribute "uniqbase"

extern ElementId ELEM_OP;             ///< Marshaling element \<op>
extern ElementId ELEM_SLEIGH;         ///< Marshaling element \<sleigh>
extern ElementId ELEM_SPACE;          ///< Marshaling element \<space>
extern ElementId ELEM_SPACEID;        ///< Marshaling element \<spaceid>
extern ElementId ELEM_SPACES;         ///< Marshaling element \<spaces>
extern ElementId ELEM_SPACE_BASE;     ///< Marshaling element \<space_base>
extern ElementId ELEM_SPACE_OTHER;    ///< Marshaling element \<space_other>
extern ElementId ELEM_SPACE_OVERLAY;  ///< Marshaling element \<space_overlay>
extern ElementId ELEM_SPACE_UNIQUE;   ///< Marshaling element \<space_unique>
extern ElementId ELEM_TRUNCATE_SPACE; ///< Marshaling element \<truncate_space>

/// \brief Exception for encountering unimplemented pcode
struct UnimplError : public LowlevelError {
    int4 instruction_length; ///< Number of bytes in the unimplemented instruction
    /// \param s is a more verbose description of the error
    /// \param l is the length (in bytes) of the unimplemented instruction
    inline UnimplError(const string& s, int4 l) : LowlevelError(s) {
        instruction_length = l;
    }
};

/// \brief Exception for bad instruction data
struct BadDataError : public LowlevelError {
    /// \param s is a more verbose description of the error
    inline BadDataError(const string& s) : LowlevelError(s) {}
};

class Translate;
bool translateIsBigEndian(const Translate* translator);
string translateGetExactRegisterName(const Translate* translator, AddrSpace* space, uintb offset, int4 size);
string translateGetRegisterName(const Translate* translator, AddrSpace* space, uintb offset, int4 size);

/// \brief Object for describing how a space should be truncated
class TruncationTag {
    string spaceName; ///< Name of space to be truncated
    uint4 size;       ///< Size truncated addresses into the space
public:
    /// Parse a \<truncate_space> element to configure this object.
    inline void decode(Decoder& decoder) {
        uint4 elemId = decoder.openElement(ELEM_TRUNCATE_SPACE);
        spaceName = decoder.readString(ATTRIB_SPACE);
        size = decoder.readUnsignedInteger(ATTRIB_SIZE);
        decoder.closeElement(elemId);
    }
    inline const string& getName(void) const {
        return spaceName;
    } ///< Get name of address space being truncated
    inline uint4 getSize(void) const {
        return size;
    } ///< Size (of pointers) for new truncated space
};

/// \brief Abstract class for emitting pcode to an application
class PcodeEmit {
public:
    virtual ~PcodeEmit(void) {} ///< Virtual destructor
    virtual void dump(const Address& addr, OpCode opc, VarnodeData* outvar, VarnodeData* vars, int4 isize) = 0;
};

/// \brief Abstract class for emitting disassembly to an application
class AssemblyEmit {
public:
    virtual ~AssemblyEmit(void) {} ///< Virtual destructor
    virtual void dump(const Address& addr, const string& mnem, const string& body) = 0;
};

/// \brief Abstract class for converting native constants to addresses
class AddressResolver {
public:
    virtual ~AddressResolver(void) {} ///> Virtual destructor
    virtual Address resolve(uintb val, int4 sz, const Address& point, uintb& fullEncoding) = 0;
};

/// \brief A virtual space \e stack space
class SpacebaseSpace : public AddrSpace {
    friend class AddrSpaceManager;
    AddrSpace* contain;   ///< Containing space
    bool hasbaseregister; ///< true if a base register has been attached
    bool isNegativeStack; ///< true if stack grows in negative direction
    VarnodeData baseloc;  ///< location data of the base register
    VarnodeData baseOrig; ///< Original base register before any truncation

    /// Set the base register at time space is created.
    inline void setBaseRegister(const VarnodeData& data, int4 origSize, bool stackGrowth) {
        if (hasbaseregister) {
            if ((baseloc != data) || (isNegativeStack != stackGrowth))
                throw LowlevelError("Attempt to assign more than one base register to space: " + getName());
        }
        hasbaseregister = true;
        isNegativeStack = stackGrowth;
        baseOrig = data;
        baseloc = data;
        if (origSize != baseloc.size) {
            if (baseloc.space->isBigEndian())
                baseloc.offset += (baseloc.size - origSize);
            baseloc.size = origSize;
        }
    }

public:
    /// Construct a virtual space, usually a stack space.
    inline SpacebaseSpace(AddrSpaceManager* m, const Translate* t, const string& nm, int4 ind, int4 sz, AddrSpace* base,
                          int4 dl, bool isFormal)
        : AddrSpace(m, t, IPTR_SPACEBASE, nm, translateIsBigEndian(t), sz, base->getWordSize(), ind, 0, dl, dl) {
        contain = base;
        hasbaseregister = false;
        isNegativeStack = true;
        setFlags(allows_wrapped_range);
        if (isFormal)
            setFlags(formal_stackspace);
    }

    /// Partial constructor for use with decode.
    inline SpacebaseSpace(AddrSpaceManager* m, const Translate* t) : AddrSpace(m, t, IPTR_SPACEBASE) {
        contain = (AddrSpace*)0;
        hasbaseregister = false;
        isNegativeStack = true;
        setFlags(programspecific | allows_wrapped_range);
    }

    inline int4 numSpacebase(void) const override {
        return hasbaseregister ? 1 : 0;
    }
    inline const VarnodeData& getSpacebase(int4 i) const override {
        if ((!hasbaseregister) || (i != 0))
            throw LowlevelError("No base register specified for space: " + getName());
        return baseloc;
    }
    inline const VarnodeData& getSpacebaseFull(int4 i) const override {
        if ((!hasbaseregister) || (i != 0))
            throw LowlevelError("No base register specified for space: " + getName());
        return baseOrig;
    }
    inline bool stackGrowsNegative(void) const override {
        return isNegativeStack;
    }
    inline AddrSpace* getContain(void) const override {
        return contain;
    } ///< Return containing space
    inline void decode(Decoder& decoder) override {
        uint4 elemId = decoder.openElement(ELEM_SPACE_BASE);
        decodeBasicAttributes(decoder);
        contain = decoder.readSpace(ATTRIB_CONTAIN);
        decoder.closeElement(elemId);
    }
};

/// \brief A record describing how logical values are split
class JoinRecord {
    friend class AddrSpaceManager;
    vector<VarnodeData> pieces; ///< All the physical pieces, most significant to least
    VarnodeData unified;        ///< Special entry representing entire symbol in one chunk
public:
    inline int4 numPieces(void) const {
        return pieces.size();
    } ///< Get number of pieces in this record
    inline bool isFloatExtension(void) const {
        return (pieces.size() == 1);
    } ///< Does this record extend a float varnode
    inline const VarnodeData& getPiece(int4 i) const {
        return pieces[i];
    } ///< Get the i-th piece
    inline const VarnodeData& getUnified(void) const {
        return unified;
    } ///< Get the Varnode whole

    /// Given an offset in the join range, map it to an equivalent piece address.
    inline Address getEquivalentAddress(uintb offset, int4& pos) const {
        if (offset < unified.offset)
            return Address();
        int4 smallOff = (int4)(offset - unified.offset);
        if (pieces[0].space->isBigEndian()) {
            for (pos = 0; pos < pieces.size(); ++pos) {
                int4 pieceSize = pieces[pos].size;
                if (smallOff < pieceSize)
                    break;
                smallOff -= pieceSize;
            }
            if (pos == pieces.size())
                return Address();
        } else {
            for (pos = pieces.size() - 1; pos >= 0; --pos) {
                int4 pieceSize = pieces[pos].size;
                if (smallOff < pieceSize)
                    break;
                smallOff -= pieceSize;
            }
            if (pos < 0)
                return Address();
        }
        return Address(pieces[pos].space, pieces[pos].offset + smallOff);
    }

    /// Allow sorting on JoinRecords so collections can map pieces to their logical whole.
    inline bool operator<(const JoinRecord& op2) const {
        if (unified.size != op2.unified.size)
            return (unified.size < op2.unified.size);
        int4 i = 0;
        for (;;) {
            if (pieces.size() == i)
                return (op2.pieces.size() > i);
            if (op2.pieces.size() == i)
                return false;
            if (pieces[i] != op2.pieces[i])
                return (pieces[i] < op2.pieces[i]);
            i += 1;
        }
    }

    /// Merge contiguous elements in a most-significant-to-least-significant sequence.
    inline static void mergeSequence(vector<VarnodeData>& seq, const Translate* trans) {
        int4 i = 1;
        while (i < seq.size()) {
            VarnodeData& hi(seq[i - 1]);
            VarnodeData& lo(seq[i]);
            if (hi.isContiguous(lo))
                break;
            i += 1;
        }
        if (i >= seq.size())
            return;
        vector<VarnodeData> res;
        i = 1;
        res.push_back(seq.front());
        bool lastIsInformal = false;
        while (i < seq.size()) {
            VarnodeData& hi(res.back());
            VarnodeData& lo(seq[i]);
            if (hi.isContiguous(lo)) {
                hi.offset = hi.space->isBigEndian() ? hi.offset : lo.offset;
                hi.size += lo.size;
                if (hi.space->getType() != IPTR_SPACEBASE)
                    lastIsInformal = translateGetExactRegisterName(trans, hi.space, hi.offset, hi.size).size() == 0;
            } else {
                if (lastIsInformal)
                    break;
                res.push_back(lo);
            }
            i += 1;
        }
        if (lastIsInformal)
            return;
        seq = res;
    }
};

/// \brief Comparator for JoinRecord objects
struct JoinRecordCompare {
    inline bool operator()(const JoinRecord* a, const JoinRecord* b) const {
        return *a < *b;
    } ///< Compare to JoinRecords using their built-in comparison
};

/// \brief A manager for different address spaces
class AddrSpaceManager {
    vector<AddrSpace*> baselist;
    vector<AddressResolver*> resolvelist;
    map<string, AddrSpace*> name2Space;
    map<int4, AddrSpace*> shortcut2Space;
    AddrSpace* constantspace;
    AddrSpace* defaultcodespace;
    AddrSpace* defaultdataspace;
    AddrSpace* iopspace;
    AddrSpace* fspecspace;
    AddrSpace* joinspace;
    AddrSpace* stackspace;
    AddrSpace* uniqspace;
    RangeList nohighptr;
    mutable uintb joinallocate;
    mutable set<JoinRecord*, JoinRecordCompare> splitset;
    mutable vector<JoinRecord*> splitlist;

protected:
    inline AddrSpace* decodeSpace(Decoder& decoder, const Translate* trans) {
        uint4 elemId = decoder.peekElement();
        unique_ptr<AddrSpace> res;
        if (elemId == ELEM_SPACE_BASE)
            res.reset(new SpacebaseSpace(this, trans));
        else if (elemId == ELEM_SPACE_UNIQUE)
            res.reset(new UniqueSpace(this, trans));
        else if (elemId == ELEM_SPACE_OTHER)
            res.reset(new OtherSpace(this, trans));
        else if (elemId == ELEM_SPACE_OVERLAY)
            res.reset(new OverlaySpace(this, trans));
        else if (elemId == ELEM_SPACE)
            res.reset(new AddrSpace(this, trans, IPTR_PROCESSOR));
        else
            throw LowlevelError("Invalid address space element");
        res->decode(decoder);
        return res.release();
    }

    inline void decodeSpaces(Decoder& decoder, const Translate* trans) {
        insertSpace(new ConstantSpace(this, trans));
        uint4 elemId = decoder.openElement(ELEM_SPACES);
        string defname = decoder.readString(ATTRIB_DEFAULTSPACE);
        while (decoder.peekElement() != 0) {
            AddrSpace* spc = decodeSpace(decoder, trans);
            insertSpace(spc);
        }
        decoder.closeElement(elemId);
        AddrSpace* spc = getSpaceByName(defname);
        if (spc == (AddrSpace*)0)
            throw LowlevelError("Bad 'defaultspace' attribute: " + defname);
        setDefaultCodeSpace(spc->getIndex());
    }

    inline void setDefaultCodeSpace(int4 index) {
        if (defaultcodespace != (AddrSpace*)0)
            throw LowlevelError("Default space set multiple times");
        if (baselist.size() <= index || baselist[index] == (AddrSpace*)0)
            throw LowlevelError("Bad index for default space");
        defaultcodespace = baselist[index];
        defaultdataspace = defaultcodespace;
    }

    inline void setDefaultDataSpace(int4 index) {
        if (defaultcodespace == (AddrSpace*)0)
            throw LowlevelError("Default data space must be set after the code space");
        if (baselist.size() <= index || baselist[index] == (AddrSpace*)0)
            throw LowlevelError("Bad index for default data space");
        defaultdataspace = baselist[index];
    }

    inline void setReverseJustified(AddrSpace* spc) {
        spc->setFlags(AddrSpace::reverse_justification);
    }

    inline void assignShortcut(AddrSpace* spc) {
        if (spc->shortcut != ' ') {
            shortcut2Space.insert(pair<int4, AddrSpace*>(spc->shortcut, spc));
            return;
        }
        char shortcut;
        switch (spc->getType()) {
            case IPTR_CONSTANT: shortcut = '#'; break;
            case IPTR_PROCESSOR: shortcut = (spc->getName() == "register") ? '%' : spc->getName()[0]; break;
            case IPTR_SPACEBASE: shortcut = 's'; break;
            case IPTR_INTERNAL: shortcut = 'u'; break;
            case IPTR_FSPEC: shortcut = 'f'; break;
            case IPTR_JOIN: shortcut = 'j'; break;
            case IPTR_IOP: shortcut = 'i'; break;
            default: shortcut = 'x'; break;
        }
        if (shortcut >= 'A' && shortcut <= 'Z')
            shortcut += 0x20;
        int4 collisionCount = 0;
        while (!shortcut2Space.insert(pair<int4, AddrSpace*>(shortcut, spc)).second) {
            collisionCount += 1;
            if (collisionCount > 26) {
                spc->shortcut = 'z';
                return;
            }
            shortcut += 1;
            if (shortcut < 'a' || shortcut > 'z')
                shortcut = 'a';
        }
        spc->shortcut = (char)shortcut;
    }

    inline void markNearPointers(AddrSpace* spc, int4 size) {
        spc->setFlags(AddrSpace::has_nearpointers);
        if (spc->minimumPointerSize == 0 && spc->addressSize != size)
            spc->minimumPointerSize = size;
    }

    inline void insertSpace(AddrSpace* spc) {
        unique_ptr<AddrSpace> owner;
        if (spc->refcount == 0)
            owner.reset(spc);
        switch (spc->getType()) {
            case IPTR_CONSTANT:
                if (spc->getName() != ConstantSpace::NAME)
                    throw LowlevelError("Space " + spc->getName() + " was initialized with wrong type");
                if (spc->index != ConstantSpace::INDEX)
                    throw LowlevelError("const space must be assigned index 0");
                constantspace = spc;
                break;
            case IPTR_INTERNAL:
                if (spc->getName() != UniqueSpace::NAME)
                    throw LowlevelError("Space " + spc->getName() + " was initialized with wrong type");
                if (uniqspace != (AddrSpace*)0)
                    throw LowlevelError("Space " + spc->getName() + " was initialized more than once");
                uniqspace = spc;
                break;
            case IPTR_FSPEC:
                if (spc->getName() != "fspec")
                    throw LowlevelError("Space " + spc->getName() + " was initialized with wrong type");
                if (fspecspace != (AddrSpace*)0)
                    throw LowlevelError("Space " + spc->getName() + " was initialized more than once");
                fspecspace = spc;
                break;
            case IPTR_JOIN:
                if (spc->getName() != JoinSpace::NAME)
                    throw LowlevelError("Space " + spc->getName() + " was initialized with wrong type");
                if (joinspace != (AddrSpace*)0)
                    throw LowlevelError("Space " + spc->getName() + " was initialized more than once");
                joinspace = spc;
                break;
            case IPTR_IOP:
                if (spc->getName() != "iop")
                    throw LowlevelError("Space " + spc->getName() + " was initialized with wrong type");
                if (iopspace != (AddrSpace*)0)
                    throw LowlevelError("Space " + spc->getName() + " was initialized more than once");
                iopspace = spc;
                break;
            case IPTR_SPACEBASE:
                if (spc->getName() == "stack") {
                    if (stackspace != (AddrSpace*)0)
                        throw LowlevelError("Space " + spc->getName() + " was initialized more than once");
                    stackspace = spc;
                }
                [[fallthrough]];
            case IPTR_PROCESSOR:
                if (spc->isOverlay())
                    spc->getContain()->setFlags(AddrSpace::overlaybase);
                else if (spc->isOtherSpace() && spc->index != OtherSpace::INDEX)
                    throw LowlevelError("OTHER space must be assigned index 1");
                break;
        }
        if (baselist.size() <= spc->index)
            baselist.resize(spc->index + 1, (AddrSpace*)0);
        if (baselist[spc->index] != (AddrSpace*)0)
            throw LowlevelError("Space " + spc->getName() + " was assigned id duplicating: " + baselist[spc->index]->getName());
        if (!name2Space.insert(pair<string, AddrSpace*>(spc->getName(), spc)).second)
            throw LowlevelError("Space " + spc->getName() + " was initialized more than once");
        baselist[spc->index] = spc;
        owner.release();
        spc->refcount += 1;
        assignShortcut(spc);
    }

    inline void copySpaces(const AddrSpaceManager* op2) {
        for (int4 i = 0; i < op2->baselist.size(); ++i) {
            AddrSpace* spc = op2->baselist[i];
            if (spc != (AddrSpace*)0)
                insertSpace(spc);
        }
        setDefaultCodeSpace(op2->getDefaultCodeSpace()->getIndex());
        setDefaultDataSpace(op2->getDefaultDataSpace()->getIndex());
    }

    inline void addSpacebasePointer(SpacebaseSpace* basespace, const VarnodeData& ptrdata, int4 truncSize,
                                    bool stackGrowth) {
        basespace->setBaseRegister(ptrdata, truncSize, stackGrowth);
    }

    inline void addNoHighPtr(const Range& rng) {
        AddrSpace* spc = rng.getSpace();
        if (spc->manage != this) {
            spc->manage->addNoHighPtr(rng);
            return;
        }
        if ((spc->flags & AddrSpace::addressable_none) != 0)
            return;
        if ((spc->flags & AddrSpace::addressable_all) != 0)
            spc->flags &= ~(uint4)AddrSpace::addressable_all;
        nohighptr.insertRange(spc, rng.getFirst(), rng.getLast());
        Range whole(spc, 0, spc->getHighest());
        if (nohighptr.inRange(whole))
            spc->flags |= AddrSpace::addressable_none;
    }

    inline void insertResolver(AddrSpace* spc, AddressResolver* rsolv) {
        int4 ind = spc->getIndex();
        while (resolvelist.size() <= ind)
            resolvelist.push_back((AddressResolver*)0);
        if (resolvelist[ind] != (AddressResolver*)0)
            delete resolvelist[ind];
        resolvelist[ind] = rsolv;
    }

    inline void setInferPtrBounds(const Range& range) {
        range.getSpace()->pointerLowerBound = range.getFirst();
        range.getSpace()->pointerUpperBound = range.getLast();
    }

    inline JoinRecord* findJoinInternal(uintb offset) const {
        int4 min = 0;
        int4 max = splitlist.size() - 1;
        while (min <= max) {
            int4 mid = (min + max) / 2;
            JoinRecord* rec = splitlist[mid];
            uintb val = rec->unified.offset;
            if (val + rec->unified.size <= offset)
                min = mid + 1;
            else if (val > offset)
                max = mid - 1;
            else
                return rec;
        }
        return (JoinRecord*)0;
    }

public:
    inline AddrSpaceManager(void) {
        defaultcodespace = (AddrSpace*)0;
        defaultdataspace = (AddrSpace*)0;
        constantspace = (AddrSpace*)0;
        iopspace = (AddrSpace*)0;
        fspecspace = (AddrSpace*)0;
        joinspace = (AddrSpace*)0;
        stackspace = (AddrSpace*)0;
        uniqspace = (AddrSpace*)0;
        joinallocate = 0;
    }

    inline virtual ~AddrSpaceManager(void) {
        for (vector<AddrSpace*>::iterator iter = baselist.begin(); iter != baselist.end(); ++iter) {
            AddrSpace* spc = *iter;
            if (spc == (AddrSpace*)0)
                continue;
            if (spc->refcount > 1)
                spc->refcount -= 1;
            else
                delete spc;
        }
        for (int4 i = 0; i < resolvelist.size(); ++i) {
            if (resolvelist[i] != (AddressResolver*)0)
                delete resolvelist[i];
        }
        for (int4 i = 0; i < splitlist.size(); ++i)
            delete splitlist[i];
    }

    inline int4 getDefaultSize(void) const {
        return defaultcodespace->getAddrSize();
    }
    inline AddrSpace* getSpaceByName(const string& nm) const {
        map<string, AddrSpace*>::const_iterator iter = name2Space.find(nm);
        if (iter == name2Space.end())
            return (AddrSpace*)0;
        return (*iter).second;
    }
    inline AddrSpace* getSpaceByShortcut(char sc) const {
        map<int4, AddrSpace*>::const_iterator iter = shortcut2Space.find(sc);
        if (iter == shortcut2Space.end())
            return (AddrSpace*)0;
        return (*iter).second;
    }
    inline AddrSpace* getIopSpace(void) const { return iopspace; }
    inline AddrSpace* getFspecSpace(void) const { return fspecspace; }
    inline AddrSpace* getJoinSpace(void) const { return joinspace; }
    inline AddrSpace* getStackSpace(void) const { return stackspace; }
    inline AddrSpace* getUniqueSpace(void) const { return uniqspace; }
    inline AddrSpace* getDefaultCodeSpace(void) const { return defaultcodespace; }
    inline AddrSpace* getDefaultDataSpace(void) const { return defaultdataspace; }
    inline AddrSpace* getConstantSpace(void) const { return constantspace; }
    inline Address getConstant(uintb val) const { return Address(constantspace, val); }

    inline bool highPtrPossible(const Address& loc, int4 size) const {
        uint4 fl = loc.getSpace()->flags & (AddrSpace::addressable_all | AddrSpace::addressable_none);
        if (fl != 0)
            return (fl == AddrSpace::addressable_all);
        return !nohighptr.inRange(loc, size);
    }

    inline Address createConstFromSpace(AddrSpace* spc) const {
        return Address(constantspace, (uintb)(uintp)spc);
    }
    inline int4 numSpaces(void) const { return baselist.size(); }
    inline AddrSpace* getSpace(int4 i) const { return baselist[i]; }

    inline Address resolveConstant(AddrSpace* spc, uintb val, int4 sz, const Address& point,
                                   uintb& fullEncoding) const {
        int4 ind = spc->getIndex();
        if (ind < resolvelist.size()) {
            AddressResolver* resolve = resolvelist[ind];
            if (resolve != (AddressResolver*)0)
                return resolve->resolve(val, sz, point, fullEncoding);
        }
        fullEncoding = val;
        val = AddrSpace::addressToByte(val, spc->getWordSize());
        val = spc->wrapOffset(val);
        return Address(spc, val);
    }

    inline AddrSpace* getNextSpaceInOrder(AddrSpace* spc) const {
        if (spc == (AddrSpace*)0)
            return baselist[0];
        if (spc == (AddrSpace*)~((uintp)0))
            return (AddrSpace*)0;
        int4 index = spc->getIndex() + 1;
        while (index < baselist.size()) {
            AddrSpace* res = baselist[index];
            if (res != (AddrSpace*)0)
                return res;
            index += 1;
        }
        return (AddrSpace*)~((uintp)0);
    }

    inline JoinRecord* findAddJoin(const vector<VarnodeData>& pieces, uint4 logicalsize) const {
        if (pieces.size() == 0)
            throw LowlevelError("Cannot create a join without pieces");
        if ((pieces.size() == 1) && (logicalsize == 0))
            throw LowlevelError("Cannot create a single piece join without a logical size");
        uint4 totalsize;
        if (logicalsize != 0) {
            if (pieces.size() != 1)
                throw LowlevelError("Cannot specify logical size for multiple piece join");
            totalsize = logicalsize;
        } else {
            totalsize = 0;
            for (int4 i = 0; i < pieces.size(); ++i)
                totalsize += pieces[i].size;
            if (totalsize == 0)
                throw LowlevelError("Cannot create a zero size join");
        }
        JoinRecord testnode;
        testnode.pieces = pieces;
        testnode.unified.size = totalsize;
        set<JoinRecord*, JoinRecordCompare>::const_iterator iter;
        iter = splitset.find(&testnode);
        if (iter != splitset.end())
            return *iter;
        JoinRecord* newjoin = new JoinRecord();
        newjoin->pieces = pieces;
        uint4 roundsize = (totalsize + 15) & ~((uint4)0xf);
        newjoin->unified.space = joinspace;
        newjoin->unified.offset = joinallocate;
        joinallocate += roundsize;
        newjoin->unified.size = totalsize;
        splitset.insert(newjoin);
        splitlist.push_back(newjoin);
        return splitlist.back();
    }

    inline JoinRecord* findJoin(uintb offset) const {
        int4 min = 0;
        int4 max = splitlist.size() - 1;
        while (min <= max) {
            int4 mid = (min + max) / 2;
            JoinRecord* rec = splitlist[mid];
            uintb val = rec->unified.offset;
            if (val == offset)
                return rec;
            if (val < offset)
                min = mid + 1;
            else
                max = mid - 1;
        }
        throw LowlevelError("Unlinked join address");
    }

    inline void setDeadcodeDelay(AddrSpace* spc, int4 delaydelta) {
        spc->deadcodedelay = delaydelta;
    }
    inline void truncateSpace(const TruncationTag& tag) {
        AddrSpace* spc = getSpaceByName(tag.getName());
        if (spc == (AddrSpace*)0)
            throw LowlevelError("Unknown space in <truncate_space> command: " + tag.getName());
        spc->truncateSpace(tag.getSize());
    }

    inline Address constructFloatExtensionAddress(const Address& realaddr, int4 realsize, int4 logicalsize) const {
        if (logicalsize == realsize)
            return realaddr;
        vector<VarnodeData> pieces;
        pieces.emplace_back();
        pieces.back().space = realaddr.getSpace();
        pieces.back().offset = realaddr.getOffset();
        pieces.back().size = realsize;
        JoinRecord* join = findAddJoin(pieces, logicalsize);
        return join->getUnified().getAddr();
    }

    inline Address constructJoinAddress(const Translate* translate, const Address& hiaddr, int4 hisz,
                                        const Address& loaddr, int4 losz) const {
        spacetype hitp = hiaddr.getSpace()->getType();
        spacetype lotp = loaddr.getSpace()->getType();
        bool usejoinspace = true;
        if (((hitp != IPTR_SPACEBASE) && (hitp != IPTR_PROCESSOR)) ||
            ((lotp != IPTR_SPACEBASE) && (lotp != IPTR_PROCESSOR)))
            throw LowlevelError("Trying to join in appropriate locations");
        if ((hitp == IPTR_SPACEBASE) || (lotp == IPTR_SPACEBASE) || (hiaddr.getSpace() == getDefaultCodeSpace()) ||
            (loaddr.getSpace() == getDefaultCodeSpace()))
            usejoinspace = false;
        if (hiaddr.isContiguous(hisz, loaddr, losz)) {
            if (!usejoinspace) {
                if (hiaddr.isBigEndian())
                    return hiaddr;
                return loaddr;
            } else {
                if (hiaddr.isBigEndian()) {
                    if (translateGetRegisterName(translate, hiaddr.getSpace(), hiaddr.getOffset(), (hisz + losz)).size() != 0)
                        return hiaddr;
                } else if (translateGetRegisterName(translate, loaddr.getSpace(), loaddr.getOffset(), (hisz + losz)).size() != 0)
                    return loaddr;
            }
        }
        vector<VarnodeData> pieces;
        pieces.emplace_back();
        pieces.emplace_back();
        pieces[0].space = hiaddr.getSpace();
        pieces[0].offset = hiaddr.getOffset();
        pieces[0].size = hisz;
        pieces[1].space = loaddr.getSpace();
        pieces[1].offset = loaddr.getOffset();
        pieces[1].size = losz;
        JoinRecord* join = findAddJoin(pieces, 0);
        return join->getUnified().getAddr();
    }

    inline Address constructWrappingAddress(const Address& addr, int4 size) const {
        AddrSpace* spc = addr.getSpace();
        if (!spc->isHeritaged())
            return addr;
        uintb dist = spc->getHighest() - addr.getOffset() + 1;
        if (size <= dist)
            return addr;
        if (!spc->allowsWrappedRange())
            throw LowlevelError("Trying to construct memory range beyond end of address space: " + spc->getName());
        int4 sizehi = (int4)dist;
        int4 sizelo = size - sizehi;
        vector<VarnodeData> pieces;
        pieces.emplace_back();
        pieces.emplace_back();
        int4 highIndex = spc->isBigEndian() ? 0 : 1;
        pieces[highIndex].space = spc;
        pieces[highIndex].offset = addr.getOffset();
        pieces[highIndex].size = sizehi;
        pieces[1 - highIndex].space = spc;
        pieces[1 - highIndex].offset = 0;
        pieces[1 - highIndex].size = sizelo;
        JoinRecord* join = findAddJoin(pieces, 0);
        return join->getUnified().getAddr();
    }

    inline void renormalizeJoinAddress(Address& addr, int4 size) const {
        JoinRecord* joinRecord = findJoinInternal(addr.getOffset());
        if (joinRecord == (JoinRecord*)0)
            throw LowlevelError("Join address not covered by a JoinRecord");
        if (addr.getOffset() == joinRecord->unified.offset && size == joinRecord->unified.size)
            return;
        int4 pos1;
        Address addr1 = joinRecord->getEquivalentAddress(addr.getOffset(), pos1);
        int4 pos2;
        Address addr2 = joinRecord->getEquivalentAddress(addr.getOffset() + (size - 1), pos2);
        if (addr2.isInvalid())
            throw LowlevelError("Join address range not covered");
        if (pos1 == pos2) {
            addr = addr1;
            return;
        }
        vector<VarnodeData> newPieces;
        int4 sizeTrunc1 = (int4)(addr1.getOffset() - joinRecord->pieces[pos1].offset);
        int4 sizeTrunc2 = joinRecord->pieces[pos2].size - (int4)(addr2.getOffset() - joinRecord->pieces[pos2].offset) - 1;
        if (pos2 < pos1) {
            newPieces.push_back(joinRecord->pieces[pos2]);
            pos2 += 1;
            while (pos2 <= pos1) {
                newPieces.push_back(joinRecord->pieces[pos2]);
                pos2 += 1;
            }
            newPieces.back().offset = addr1.getOffset();
            newPieces.back().size -= sizeTrunc1;
            newPieces.front().size -= sizeTrunc2;
        } else {
            newPieces.push_back(joinRecord->pieces[pos1]);
            pos1 += 1;
            while (pos1 <= pos2) {
                newPieces.push_back(joinRecord->pieces[pos1]);
                pos1 += 1;
            }
            newPieces.front().offset = addr1.getOffset();
            newPieces.front().size -= sizeTrunc1;
            newPieces.back().size -= sizeTrunc2;
        }
        JoinRecord* newJoinRecord = findAddJoin(newPieces, 0);
        addr = Address(newJoinRecord->unified.space, newJoinRecord->unified.offset);
    }

    inline const VarnodeData& stripJoinPiece(JoinRecord* join, int4 index) const {
        int4 start, end;
        if (index == 0) {
            start = 1;
            end = join->numPieces() - 1;
        } else if (index == join->numPieces() - 1) {
            start = 0;
            end = join->numPieces() - 2;
        } else
            throw LowlevelError("Stripping middle piece from JoinRecord");
        if (start == end)
            return join->getPiece(start);
        vector<VarnodeData> newPieces;
        for (int4 i = start; i <= end; ++i)
            newPieces.push_back(join->getPiece(i));
        JoinRecord* newJoinRecord = findAddJoin(newPieces, 0);
        return newJoinRecord->unified;
    }

    inline Address parseAddressSimple(const string& val) const {
        string::size_type col = val.find(':');
        AddrSpace* spc;
        if (col == string::npos) {
            spc = getDefaultDataSpace();
            col = 0;
        } else {
            string spcName = val.substr(0, col);
            spc = getSpaceByName(spcName);
            if (spc == (AddrSpace*)0)
                throw LowlevelError("Unknown address space: " + spcName);
            col += 1;
        }
        if (col + 2 <= val.size() && val[col] == '0' && val[col + 1] == 'x')
            col += 2;
        istringstream s(val.substr(col));
        uintb off;
        s >> hex >> off;
        return Address(spc, AddrSpace::addressToByte(off, spc->getWordSize()));
    }
};

/// \brief The interface to a translation engine for a processor.
class Translate : public AddrSpaceManager {
public:
    enum UniqueLayout {
        RUNTIME_BOOLEAN_INVERT = 0,
        RUNTIME_RETURN_LOCATION = 0x80,
        RUNTIME_BITRANGE_EA = 0x100,
        INJECT = 0x200,
        ANALYSIS = 0x10000000
    };

private:
    bool target_isbigendian;
    uint4 unique_base;
protected:
    int4 alignment;
    inline void setBigEndian(bool val) {
        target_isbigendian = val;
    }
    inline void setUniqueBase(uint4 val) {
        if (val > unique_base)
            unique_base = val;
    }
public:
    inline Translate(void) {
        target_isbigendian = false;
        unique_base = 0;
        alignment = 1;
    }
    inline bool isBigEndian(void) const { return target_isbigendian; }
    inline int4 getAlignment(void) const { return alignment; }
    inline uint4 getUniqueBase(void) const { return unique_base; }
    inline uint4 getUniqueStart(UniqueLayout layout) const {
        return (layout != ANALYSIS) ? layout + unique_base : layout;
    }
    virtual void initialize(const string& slaFilename) = 0;
    virtual void registerContext(const string& name, int4 sbit, int4 ebit) {}
    virtual void setContextDefault(const string& name, uintm val) {}
    virtual void allowContextSet(bool val) const {}
    virtual const VarnodeData& getRegister(const string& nm) const = 0;
    virtual string getRegisterName(AddrSpace* base, uintb off, int4 size) const = 0;
    virtual string getExactRegisterName(AddrSpace* base, uintb off, int4 size) const = 0;
    virtual void getAllRegisters(map<VarnodeData, string>& reglist) const = 0;
    virtual void getUserOpNames(vector<string>& res) const = 0;
    virtual int4 instructionLength(const Address& baseaddr) const = 0;
    virtual int4 oneInstruction(PcodeEmit& emit, const Address& baseaddr) const = 0;
    virtual int4 printAssembly(AssemblyEmit& emit, const Address& baseaddr) const = 0;
};

/// Provides the marshal partition with the address-space count without
/// introducing a circular import from marshal back to translate.
uint4 addrSpaceManagerNumSpaces(const AddrSpaceManager* manager) {
    return manager->numSpaces();
}

/// Provides indexed address-space lookup to the marshal partition.
AddrSpace* addrSpaceManagerGetSpace(const AddrSpaceManager* manager, uint4 index) {
    return manager->getSpace(index);
}

/// Provides stack-space lookup to the marshal partition.
AddrSpace* addrSpaceManagerGetStackSpace(const AddrSpaceManager* manager) {
    return manager->getStackSpace();
}

/// Provides join-space lookup to the marshal partition.
AddrSpace* addrSpaceManagerGetJoinSpace(const AddrSpaceManager* manager) {
    return manager->getJoinSpace();
}

/// Provides the address partition with the translator associated with the
/// default code space.
const Translate* addrSpaceManagerDefaultCodeTranslator(const AddrSpaceManager* manager) {
    return manager->getDefaultCodeSpace()->getTrans();
}

/// Provides named address-space lookup to the address and space partitions.
AddrSpace* addrSpaceManagerGetSpaceByName(const AddrSpaceManager* manager, const string& name) {
    return manager->getSpaceByName(name);
}

/// Provides ordered address-space lookup to the address partition.
AddrSpace* addrSpaceManagerGetNextSpaceInOrder(const AddrSpaceManager* manager, AddrSpace* space) {
    return manager->getNextSpaceInOrder(space);
}

/// Resolves a register through the translator while keeping VarnodeData
/// construction out of partitions that only need its individual fields.
void translateGetRegister(const Translate* translator, const string& name, AddrSpace*& space, uintb& offset,
                                uint4& size) {
    const VarnodeData& registerLocation = translator->getRegister(name);
    space = registerLocation.space;
    offset = registerLocation.offset;
    size = registerLocation.size;
}

/// Provides the manager's pointer feasibility calculation to Address.
bool addrSpaceHighPtrPossible(const AddrSpace* space, const Address& address, int4 size) {
    return space->manage->highPtrPossible(address, size);
}

/// Provides join-address normalization to Address.
void addrSpaceRenormalizeJoinAddress(const AddrSpace* space, Address& address, int4 size) {
    space->manage->renormalizeJoinAddress(address, size);
}

/// Provides manager lookup operations to the space partition.
JoinRecord* addrSpaceManagerFindJoin(const AddrSpaceManager* manager, uintb offset) {
    return manager->findJoin(offset);
}

/// Creates or reuses a join record for the space partition.
JoinRecord* addrSpaceManagerFindAddJoin(const AddrSpaceManager* manager, const vector<VarnodeData>& pieces,
                                               int4 logicalSize) {
    return manager->findAddJoin(pieces, logicalSize);
}

/// Looks up a space by its shortcut character for join parsing.
AddrSpace* addrSpaceManagerGetSpaceByShortcut(const AddrSpaceManager* manager, char shortcut) {
    return manager->getSpaceByShortcut(shortcut);
}

/// Returns the manager's default address size for numeric space parsing.
int4 addrSpaceManagerDefaultSize(const AddrSpaceManager* manager) {
    return manager->getDefaultSize();
}

/// Returns the target endianness used when constructing special spaces.
bool translateIsBigEndian(const Translate* translator) {
    return translator->isBigEndian();
}

/// Returns the exact register spelling for a concrete storage location.
string translateGetExactRegisterName(const Translate* translator, AddrSpace* space, uintb offset, int4 size) {
    return translator->getExactRegisterName(space, offset, size);
}

/// Returns a register spelling for a concrete storage location.
string translateGetRegisterName(const Translate* translator, AddrSpace* space, uintb offset, int4 size) {
    return translator->getRegisterName(space, offset, size);
}

/// Resolves a join record's equivalent address for overlap calculations.
Address joinRecordGetEquivalentAddress(const JoinRecord* record, uintb offset, int4& position) {
    return record->getEquivalentAddress(offset, position);
}

/// Returns the number of pieces in a join record.
int4 joinRecordNumPieces(const JoinRecord* record) {
    return record->numPieces();
}

/// Returns one storage piece from a join record.
const VarnodeData& joinRecordGetPiece(const JoinRecord* record, int4 position) {
    return record->getPiece(position);
}

/// Copies the unified storage triple from a join record.
void joinRecordGetUnified(const JoinRecord* record, AddrSpace*& space, uintb& offset, uint4& size) {
    const VarnodeData& unified = record->getUnified();
    space = unified.space;
    offset = unified.offset;
    size = unified.size;
}

AttributeId ATTRIB_CODE = AttributeId("code", 43);
AttributeId ATTRIB_CONTAIN = AttributeId("contain", 44);
AttributeId ATTRIB_DEFAULTSPACE = AttributeId("defaultspace", 45);
AttributeId ATTRIB_UNIQBASE = AttributeId("uniqbase", 46);
ElementId ELEM_OP = ElementId("op", 27);
ElementId ELEM_SLEIGH = ElementId("sleigh", 28);
ElementId ELEM_SPACE = ElementId("space", 29);
ElementId ELEM_SPACEID = ElementId("spaceid", 30);
ElementId ELEM_SPACES = ElementId("spaces", 31);
ElementId ELEM_SPACE_BASE = ElementId("space_base", 32);
ElementId ELEM_SPACE_OTHER = ElementId("space_other", 33);
ElementId ELEM_SPACE_OVERLAY = ElementId("space_overlay", 34);
ElementId ELEM_SPACE_UNIQUE = ElementId("space_unique", 35);
ElementId ELEM_TRUNCATE_SPACE = ElementId("truncate_space", 36);

} // End namespace ghidra
#endif
