module;
#include <algorithm>
#include <memory>
#include <string>
#include <utility>

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
/// \file globalcontext.cppm
/// \brief Utilities for getting address-based context to the disassembler and decompiler
#ifndef __GLOBALCONTEXT_HH__
#define __GLOBALCONTEXT_HH__

export module sleigh_runtime.ghidra:globalcontext;
export import :partmap;
export import :varnode;
export import :space;

export namespace ghidra {

extern ElementId ELEM_CONTEXT_DATA;     ///< Marshaling element \<context_data>
extern ElementId ELEM_CONTEXT_POINTS;   ///< Marshaling element \<context_points>
extern ElementId ELEM_CONTEXT_POINTSET; ///< Marshaling element \<context_pointset>
extern ElementId ELEM_CONTEXT_SET;      ///< Marshaling element \<context_set>
extern ElementId ELEM_SET;              ///< Marshaling element \<set>
extern ElementId ELEM_TRACKED_POINTSET; ///< Marshaling element \<tracked_pointset>
extern ElementId ELEM_TRACKED_SET;      ///< Marshaling element \<tracked_set>

ElementId ELEM_CONTEXT_DATA = ElementId("context_data", 120);
ElementId ELEM_CONTEXT_POINTS = ElementId("context_points", 121);
ElementId ELEM_CONTEXT_POINTSET = ElementId("context_pointset", 122);
ElementId ELEM_CONTEXT_SET = ElementId("context_set", 123);
ElementId ELEM_SET = ElementId("set", 124);
ElementId ELEM_TRACKED_POINTSET = ElementId("tracked_pointset", 125);
ElementId ELEM_TRACKED_SET = ElementId("tracked_set", 126);

/// \brief Description of a context variable within the disassembly context \e blob
///
/// Disassembly context is stored as individual (integer) values packed into a sequence of words. This class
/// represents the info for encoding or decoding a single value within this sequence.  A value is
/// a contiguous range of bits within one context word. Size can range from 1 bit up to the size of a word.
class ContextBitRange {
    int4 word;     ///< Index of word containing this context value
    int4 startbit; ///< Starting bit of the value within its word (0=most significant bit 1=least significant)
    int4 endbit;   ///< Ending bit of the value within its word
    int4 shift;    ///< Right-shift amount to apply when unpacking this value from its word
    uintm mask;    ///< Mask to apply (after shifting) when unpacking this value from its word
public:
    ContextBitRange(void) {} ///< Construct an undefined bit range

    /// Bits within the whole context blob are labeled starting with 0 as the most significant bit
    /// in the first word in the sequence. The new context value must be contained within a single
    /// word.
    /// \param sbit is the starting (most significant) bit of the new value
    /// \param ebit is the ending (least significant) bit of the new value
    ContextBitRange(int4 sbit, int4 ebit) {
        word = sbit / (8 * sizeof(uintm));
        startbit = sbit - word * 8 * sizeof(uintm);
        endbit = ebit - word * 8 * sizeof(uintm);
        shift = 8 * sizeof(uintm) - endbit - 1;
        mask = (~((uintm)0)) >> (startbit + shift);
    }

    int4 getShift(void) const {
        return shift;
    } ///< Return the shift-amount for \b this value
    uintm getMask(void) const {
        return mask;
    } ///< Return the mask for \b this value
    int4 getWord(void) const {
        return word;
    } ///< Return the word index for \b this value

    /// \brief Set \b this value within a given context blob
    ///
    /// \param vec is the given context blob to alter (as an array of uintm words)
    /// \param val is the integer value to set
    void setValue(uintm* vec, uintm val) const {
        uintm newval = vec[word];
        newval &= ~(mask << shift);
        newval |= ((val & mask) << shift);
        vec[word] = newval;
    }

    /// \brief Retrieve \b this value from a given context blob
    ///
    /// \param vec is the given context blob (as an array of uintm words)
    /// \return the recovered integer value
    uintm getValue(const uintm* vec) const {
        return ((vec[word] >> shift) & mask);
    }
};

/// \brief A tracked register (Varnode) and the value it contains
///
/// This is the object returned when querying for tracked registers,
/// via ContextDatabase::getTrackedSet().  It holds the storage details of the register and
/// the actual value it holds at the point of the query.
struct TrackedContext {
    VarnodeData loc; ///< Storage details of the register being tracked
    uintb val;       ///< The value of the register

    /// Parse a \<set> element to fill in the storage and value details.
    /// \param decoder is the stream decoder
    void decode(Decoder& decoder) {
        uint4 elemId = decoder.openElement(ELEM_SET);
        loc.decodeFromAttributes(decoder);

        val = decoder.readUnsignedInteger(ATTRIB_VAL);
        decoder.closeElement(elemId);
    }

    /// The register storage and value are encoded as a \<set> element.
    /// \param encoder is the stream encoder
    void encode(Encoder& encoder) const {
        encoder.openElement(ELEM_SET);
        loc.space->encodeAttributes(encoder, loc.offset, loc.size);
        encoder.writeUnsignedInteger(ATTRIB_VAL, val);
        encoder.closeElement(ELEM_SET);
    }
};
typedef vector<TrackedContext> TrackedSet; ///< A set of tracked registers and their values (at one code point)

/// \brief An interface to a database of disassembly/decompiler \b context information
///
/// \b Context \b information is a set of named variables that hold concrete values at specific
/// addresses in the target executable being analyzed. A variable can hold different values at
/// different addresses, but a specific value at a specific address never changes. Analysis recovers
/// these values over time, populating this database, and querying this database lets analysis
/// provides concrete values for memory locations in context.
///
/// Context variables come in two flavors:
///  - \b Low-level \b context \b variables:
///      These can affect instruction decoding. These can be as small as a single bit and need to
///      be defined in the Sleigh specification (so that Sleigh knows how they effect disassembly).
///      These variables are not mapped to normal memory locations with an address space and offset
///      (although they often have a corresponding embedding into a normal memory location).
///      The model to keep in mind is a control register with specialized bit-fields within it.
///  - \b High-level \b tracked \b variables:
///      These are normal memory locations that are to be treated as constants across some range of
///      code. These are normally registers that are being tracked by the compiler outside the
///      domain of normal local and global variables. They have a specific value established by the
///      compiler coming into a function but are not supposed to be interpreted as a high-level
///      variable. Typical examples are the direction flag (for \e string instructions) and segment
///      registers. All tracked variables are interpreted as a constant value at the start of a
///      function, although the memory location can be recycled for other calculations later in the
///      function.
///
/// Low-level context variables can be queried and set by name -- getVariable(), setVariable(),
/// setVariableRegion() -- but the disassembler accesses all the variables at an address as a group
/// via getContext(), setContextChangePoint(), setContextRegion(). In this setting, all the values
/// are packed together in an array of words, a context \e blob (See ContextBitRange).
///
/// Tracked variables are also queried as a group via getTrackedSet() and createSet(). These return
/// a list of TrackedContext objects.
class ContextDatabase {
protected:
    /// \brief Encode all tracked register values for a specific address to a stream
    ///
    /// Encode all the tracked register values associated with a specific target address
    /// as a \<tracked_pointset> tag.
    /// \param encoder is the stream encoder
    /// \param addr is the specific address we have tracked values for
    /// \param vec is the list of tracked values
    static void encodeTracked(Encoder& encoder, const Address& addr, const TrackedSet& vec) {
        if (vec.empty())
            return;
        encoder.openElement(ELEM_TRACKED_POINTSET);
        addr.getSpace()->encodeAttributes(encoder, addr.getOffset());
        for (int4 i = 0; i < vec.size(); ++i) {
            vec[i].encode(encoder);
        }
        encoder.closeElement(ELEM_TRACKED_POINTSET);
    }

    /// \brief Restore a sequence of tracked register values from the given stream decoder
    ///
    /// Parse a \<tracked_pointset> element, decoding each child in turn to populate a list of
    /// TrackedContext objects.
    /// \param decoder is the given stream decoder
    /// \param vec is the container that will hold the new TrackedContext objects
    static void decodeTracked(Decoder& decoder, TrackedSet& vec) {
        vec.clear(); // Clear out any old stuff
        while (decoder.peekElement() != 0) {
            vec.emplace_back();
            vec.back().decode(decoder);
        }
    }

    /// \brief Retrieve the context variable description object by name
    ///
    /// If the variable doesn't exist an exception is thrown.
    /// \param nm is the name of the context value
    /// \return the ContextBitRange object matching the name
    virtual ContextBitRange& getVariable(const string& nm) = 0;

    /// \brief Retrieve the context variable description object by name
    ///
    /// If the variable doesn't exist an exception is thrown.
    /// \param nm is the name of the context value
    /// \return the ContextBitRange object matching the name
    virtual const ContextBitRange& getVariable(const string& nm) const = 0;

    /// \brief Grab the context blob(s) for the given address range, marking bits that will be set
    ///
    /// This is an internal routine for obtaining the actual memory regions holding context values
    /// for the address range. This also informs the system which bits are getting set. A split is forced
    /// at the first address, and at least one memory region is passed back. The second address can be
    /// invalid in which case the memory region passed back is valid from the first address to whatever
    /// the next split point is.
    /// \param res will hold pointers to memory regions for the given range
    /// \param addr1 is the starting address of the range
    /// \param addr2 is (1 past) the last address of the range or is invalid
    /// \param num is the word index for the context value that will be set
    /// \param mask is a mask of the value being set (within its word)
    virtual void getRegionForSet(vector<uintm*>& res, const Address& addr1, const Address& addr2, int4 num,
                                 uintm mask) = 0;

    /// \brief Grab the context blob(s) starting at the given address up to the first point of change
    ///
    /// This is an internal routine for obtaining the actual memory regions holding context values
    /// starting at the given address. A specific context value is specified, and all memory regions
    /// are returned up to the first address where that particular context value changes.
    /// \param res will hold pointers to memory regions being passed back
    /// \param addr is the starting address of the regions to fetch
    /// \param num is the word index for the specific context value being set
    /// \param mask is a mask of the context value being set (within its word)
    virtual void getRegionToChangePoint(vector<uintm*>& res, const Address& addr, int4 num, uintm mask) = 0;

    /// \brief Retrieve the memory region holding all default context values
    ///
    /// This fetches the active memory holding the default context values on top of which all other context
    /// values are overlaid.
    /// \return the memory region holding all the default context values
    virtual uintm* getDefaultValue(void) = 0;

    /// \brief Retrieve the memory region holding all default context values
    ///
    /// This fetches the active memory holding the default context values on top of which all other context
    /// values are overlaid.
    /// \return the memory region holding all the default context values
    virtual const uintm* getDefaultValue(void) const = 0;

public:
    virtual ~ContextDatabase() {} ///< Destructor

    /// \brief Retrieve the number of words (uintm) in a context \e blob
    ///
    /// \return the number of words
    virtual int4 getContextSize(void) const = 0;

    /// \brief Register a new named context variable (as a bit range) with the database
    ///
    /// A new variable is registered by providing a name and the range of bits the value will occupy
    /// within the context blob. The full blob size is automatically increased if necessary. The variable
    /// must be contained within a single word, and all variables must be registered before any values can
    /// be set.
    /// \param nm is the name of the new variable
    /// \param sbit is the position of the variable's most significant bit within the blob
    /// \param ebit is the position of the variable's least significant bit within the blob
    virtual void registerVariable(const string& nm, int4 sbit, int4 ebit) = 0;

    /// \brief Get the context blob of values associated with a given address
    ///
    /// \param addr is the given address
    /// \return the memory region holding the context values for the address
    virtual const uintm* getContext(const Address& addr) const = 0;

    /// \brief Get the context blob of values associated with a given address and its bounding offsets
    ///
    /// In addition to the memory region, the range of addresses for which the region is valid
    /// is passed back as offsets into the address space.
    /// \param addr is the given address
    /// \param first will hold the starting offset of the valid range
    /// \param last will hold the ending offset of the valid range
    /// \return the memory region holding the context values for the address
    virtual const uintm* getContext(const Address& addr, uintb& first, uintb& last) const = 0;

    /// \brief Get the set of default values for all tracked registers
    ///
    /// \return the list of TrackedContext objects
    virtual TrackedSet& getTrackedDefault(void) = 0;

    /// \brief Get the set of tracked register values associated with the given address
    ///
    /// \param addr is the given address
    /// \return the list of TrackedContext objects
    virtual const TrackedSet& getTrackedSet(const Address& addr) const = 0;

    /// \brief Create a tracked register set that is valid over the given range
    ///
    /// This really should be an internal routine. The created set is empty, old values are blown
    /// away. If old/default values are to be preserved, they must be copied back in.
    /// \param addr1 is the starting address of the given range
    /// \param addr2 is (1 past) the ending address of the given range
    /// \return the empty set of tracked register values
    virtual TrackedSet& createSet(const Address& addr1, const Address& addr2) = 0;

    /// \brief Encode the entire database to a stream
    ///
    /// \param encoder is the stream encoder
    virtual void encode(Encoder& encoder) const = 0;

    /// \brief Restore the state of \b this database object from the given stream decoder
    ///
    /// \param decoder is the given stream decoder
    virtual void decode(Decoder& decoder) = 0;

    /// \brief Add initial context state from elements in the compiler/processor specifications
    ///
    /// Parse a \<context_data> element from the given stream decoder from either the compiler
    /// or processor specification file for the architecture, initializing this database.
    /// \param decoder is the given stream decoder
    virtual void decodeFromSpec(Decoder& decoder) = 0;

    /// The default value is returned for addresses that have not been overlaid with other values.
    /// \param nm is the name of the context variable
    /// \param val is the default value to establish
    void setVariableDefault(const string& nm, uintm val) {
        ContextBitRange& var(getVariable(nm));
        var.setValue(getDefaultValue(), val);
    }

    /// This will return the default value used for addresses that have not been overlaid with other values.
    /// \param nm is the name of the context variable
    /// \return the variable's default value
    uintm getDefaultValue(const string& nm) const {
        const ContextBitRange& var(getVariable(nm));
        return var.getValue(getDefaultValue());
    }

    /// The variable will be changed to the new value, starting at the given address up to the next
    /// point of change.
    /// \param nm is the name of the context variable
    /// \param addr is the given address
    /// \param value is the new value to set
    void setVariable(const string& nm, const Address& addr, uintm value) {
        const ContextBitRange& bitrange(getVariable(nm));
        int4 num = bitrange.getWord();
        uintm mask = bitrange.getMask() << bitrange.getShift();

        vector<uintm*> contvec;
        getRegionToChangePoint(contvec, addr, num, mask);
        for (uint4 i = 0; i < contvec.size(); ++i)
            bitrange.setValue(contvec[i], value);
    }

    /// If a value has not been explicit set for an address range containing the given address,
    /// the default value for the variable is returned
    /// \param nm is the name of the context variable
    /// \param addr is the address for which the specific value is needed
    /// \return the context variable value for the address
    uintm getVariable(const string& nm, const Address& addr) const {
        const ContextBitRange& bitrange(getVariable(nm));

        const uintm* context = getContext(addr);
        return bitrange.getValue(context);
    }

    /// \brief Set a specific context value starting at the given address
    ///
    /// The new value is \e painted across an address range starting, starting with the given address
    /// up to the point where another change for the variable was specified. No other context variable
    /// is changed, inside (or outside) the range.
    /// \param addr is the given starting address
    /// \param num is the index of the word (within the context blob) of the context variable
    /// \param mask is the mask delimiting the context variable (within its word)
    /// \param value is the (already shifted) value being set
    void setContextChangePoint(const Address& addr, int4 num, uintm mask, uintm value) {
        vector<uintm*> contvec;
        getRegionToChangePoint(contvec, addr, num, mask);
        for (uint4 i = 0; i < contvec.size(); ++i) {
            uintm* newcontext = contvec[i];
            uintm val = newcontext[num];
            val &= ~mask; // Clear range to zero
            val |= value;
            newcontext[num] = val;
        }
    }

    /// \brief Set a context variable value over a given range of addresses
    ///
    /// The new value is \e painted over an explicit range of addresses. No other context variable is
    /// changed inside (or outside) the range.
    /// \param addr1 is the starting address of the given range
    /// \param addr2 is the ending address of the given range
    /// \param num is the index of the word (within the context blob) of the context variable
    /// \param mask is the mask delimiting the context variable (within its word)
    /// \param value is the (already shifted) value being set
    void setContextRegion(const Address& addr1, const Address& addr2, int4 num, uintm mask, uintm value) {
        vector<uintm*> vec;
        getRegionForSet(vec, addr1, addr2, num, mask);
        for (uint4 i = 0; i < vec.size(); ++i)
            vec[i][num] = (vec[i][num] & ~mask) | value;
    }

    /// \brief Set a context variable by name over a given range of addresses
    ///
    /// The new value is \e painted over an explicit range of addresses. No other context variable is
    /// changed inside (or outside) the range.
    /// \param nm is the name of the context variable to set
    /// \param begad is the starting address of the given range
    /// \param endad is the ending address of the given range
    /// \param value is the new value to set
    void setVariableRegion(const string& nm, const Address& begad, const Address& endad, uintm value) {
        const ContextBitRange& bitrange(getVariable(nm));

        vector<uintm*> vec;
        getRegionForSet(vec, begad, endad, bitrange.getWord(), bitrange.getMask() << bitrange.getShift());
        for (int4 i = 0; i < vec.size(); ++i)
            bitrange.setValue(vec[i], value);
    }

    /// \brief Get the value of a tracked register at a specific address
    ///
    /// A specific storage region and code address is given. If the region is tracked the value at
    /// the address is retrieved. If the specified storage region is contained in the tracked region,
    /// the retrieved value is trimmed to match the containment before returning it. If the region is not
    /// tracked, a value of 0 is returned.
    /// \param mem is the specified storage region
    /// \param point is the code address
    /// \return the tracked value or zero
    uintb getTrackedValue(const VarnodeData& mem, const Address& point) const {
        const TrackedSet& tset(getTrackedSet(point));
        uintb endoff = mem.offset + mem.size - 1;
        uintb tendoff;
        for (int4 i = 0; i < tset.size(); ++i) {
            const TrackedContext& tcont(tset[i]);
            // tcont must contain -mem-
            if (tcont.loc.space != mem.space)
                continue;
            if (tcont.loc.offset > mem.offset)
                continue;
            tendoff = tcont.loc.offset + tcont.loc.size - 1;
            if (tendoff < endoff)
                continue;
            uintb res = tcont.val;
            // If we have proper containment, trim value based on endianness
            if (tcont.loc.space->isBigEndian()) {
                if (endoff != tendoff)
                    res >>= (8 * (tendoff - mem.offset));
            } else {
                if (mem.offset != tcont.loc.offset)
                    res >>= (8 * (mem.offset - tcont.loc.offset));
            }
            res &= calc_mask(mem.size); // Final trim based on size
            return res;
        }
        return (uintb)0;
    }
};

/// \brief An in-memory implementation of the ContextDatabase interface
///
/// Context blobs are held in a partition map on addresses. Any address within the map
/// indicates a \e split point, where the value of a context variable was explicitly changed.
/// Sets of tracked registers are held in a separate partition map.
class ContextInternal : public ContextDatabase {
    /// \brief A context blob, holding context values across some range of code addresses
    ///
    /// This is an internal object that allocates the actual "array of words" for a context blob.
    /// An associated mask array holds 1-bits for context variables that were explicitly set for the
    /// specific split point.
    struct FreeArray {
        std::unique_ptr<uintm[]> array; ///< Context values owned by this blob
        std::unique_ptr<uintm[]> mask;  ///< Explicit-value bits owned by this blob
        int4 size = 0;                  ///< Number of words in both arrays

        /// Creates an empty context blob.
        FreeArray() = default;

        /// Deep-copies values while intentionally clearing explicit-value bits.
        FreeArray(const FreeArray& other) : size(0) {
            *this = other;
        }

        /// Transfers both arrays without copying their contents.
        FreeArray(FreeArray&&) noexcept = default;

        /// Clone a context blob into \b this.
        /// \param other is the context blob being cloned/copied
        /// \return a reference to \b this
        FreeArray& operator=(const FreeArray& other) {
            if (this == &other)
                return *this;

            std::unique_ptr<uintm[]> new_array;
            std::unique_ptr<uintm[]> new_mask;
            if (other.size != 0) {
                new_array = std::make_unique<uintm[]>(other.size);
                new_mask = std::make_unique<uintm[]>(other.size);
                for (int4 i = 0; i < other.size; ++i) {
                    new_array[i] = other.array[i]; // Copy value at split point
                    new_mask[i] = 0;               // but not fact that value is being set
                }
            }
            array = std::move(new_array);
            mask = std::move(new_mask);
            size = other.size;
            return *this;
        }

        /// Transfers both arrays without copying their contents.
        FreeArray& operator=(FreeArray&&) noexcept = default;

        /// The "array of words" and mask array are resized to the given value. Old values are preserved,
        /// chopping off the last values, or appending zeroes, as needed.
        /// \param new_size is the new number of words to resize array to
        void reset(int4 new_size) {
            std::unique_ptr<uintm[]> new_array;
            std::unique_ptr<uintm[]> new_mask;
            if (new_size != 0) {
                new_array = std::make_unique<uintm[]>(new_size);
                new_mask = std::make_unique<uintm[]>(new_size);
                const int4 preserved = std::min(new_size, size);
                for (int4 i = 0; i < preserved; ++i) {
                    new_array[i] = array[i];
                    new_mask[i] = mask[i];
                }
                for (int4 i = preserved; i < new_size; ++i) {
                    new_array[i] = 0;
                    new_mask[i] = 0;
                }
            }
            array = std::move(new_array);
            mask = std::move(new_mask);
            size = new_size;
        }
    };

    int4 size;                              ///< Number of words in a context blob (for this architecture)
    map<string, ContextBitRange> variables; ///< Map from context variable name to description object
    partmap<Address, FreeArray> database;   ///< Partition map of context blobs (FreeArray)
    partmap<Address, TrackedSet> trackbase; ///< Partition map of tracked register sets

    /// \brief Encode a single context block to a stream
    ///
    /// The blob is broken up into individual values and written out as a series
    /// of \<set> elements within a parent \<context_pointset> element.
    /// \param encoder is the stream encoder
    /// \param addr is the address of the split point where the blob is valid
    /// \param vec is the array of words holding the blob values
    void encodeContext(Encoder& encoder, const Address& addr, const uintm* vec) const {
        encoder.openElement(ELEM_CONTEXT_POINTSET);
        addr.getSpace()->encodeAttributes(encoder, addr.getOffset());
        map<string, ContextBitRange>::const_iterator iter;
        for (iter = variables.begin(); iter != variables.end(); ++iter) {
            uintm val = (*iter).second.getValue(vec);
            encoder.openElement(ELEM_SET);
            encoder.writeString(ATTRIB_NAME, (*iter).first);
            encoder.writeUnsignedInteger(ATTRIB_VAL, val);
            encoder.closeElement(ELEM_SET);
        }
        encoder.closeElement(ELEM_CONTEXT_POINTSET);
    }

    /// \brief Restore a context blob for given address range from a stream decoder
    ///
    /// Parse either a \<context_pointset> or \<context_set> element. In either case,
    /// children are parsed to get context variable values. Then a context blob is
    /// reconstructed from the values. The new blob is added to the interval map based
    /// on the address range. If the start address is invalid, the default value of
    /// the context variables are painted. The second address can be invalid, if
    /// only a split point is known.
    /// \param decoder is the stream decoder
    /// \param addr1 is the starting address of the given range
    /// \param addr2 is the ending address of the given range
    void decodeContext(Decoder& decoder, const Address& addr1, const Address& addr2) {
        for (;;) {
            uint4 subId = decoder.openElement();
            if (subId != ELEM_SET)
                break;
            uintm val = decoder.readUnsignedInteger(ATTRIB_VAL);
            ContextBitRange& var(getVariable(decoder.readString(ATTRIB_NAME)));
            vector<uintm*> vec;
            if (addr1.isInvalid()) { // Invalid addr1, indicates we should set default value
                uintm* defaultBuffer = getDefaultValue();
                for (int4 i = 0; i < size; ++i)
                    defaultBuffer[i] = 0;
                vec.push_back(defaultBuffer);
            } else
                getRegionForSet(vec, addr1, addr2, var.getWord(), var.getMask() << var.getShift());
            for (int4 i = 0; i < vec.size(); ++i)
                var.setValue(vec[i], val);
            decoder.closeElement(subId);
        }
    }

    /// Retrieve a mutable context variable description by name.
    virtual ContextBitRange& getVariable(const string& nm) {
        map<string, ContextBitRange>::iterator iter;

        iter = variables.find(nm);
        if (iter == variables.end())
            throw LowlevelError("Non-existent context variable: " + nm);
        return (*iter).second;
    }

    /// Retrieve an immutable context variable description by name.
    virtual const ContextBitRange& getVariable(const string& nm) const {
        map<string, ContextBitRange>::const_iterator iter;

        iter = variables.find(nm);
        if (iter == variables.end())
            throw LowlevelError("Non-existent context variable: " + nm);
        return (*iter).second;
    }

    /// Grab regions that receive an explicitly set context value.
    virtual void getRegionForSet(vector<uintm*>& res, const Address& addr1, const Address& addr2, int4 num,
                                 uintm mask) {
        database.split(addr1);
        partmap<Address, FreeArray>::iterator aiter, biter;

        aiter = database.begin(addr1);
        if (!addr2.isInvalid()) {
            database.split(addr2);
            biter = database.begin(addr2);
        } else
            biter = database.end();
        while (aiter != biter) {
            uintm* context = (*aiter).second.array.get();
            uintm* maskPtr = (*aiter).second.mask.get();
            res.push_back(context);
            maskPtr[num] |= mask; // Mark that this value is being definitely set
            ++aiter;
        }
    }

    /// Grab regions from an address through the next explicit change point.
    virtual void getRegionToChangePoint(vector<uintm*>& res, const Address& addr, int4 num, uintm mask) {
        database.split(addr);
        partmap<Address, FreeArray>::iterator aiter, biter;
        uintm *maskArray, *vecArray;

        aiter = database.begin(addr);
        biter = database.end();
        if (aiter == biter)
            return;
        vecArray = (*aiter).second.array.get();
        res.push_back(vecArray);
        maskArray = (*aiter).second.mask.get();
        maskArray[num] |= mask;
        ++aiter;
        while (aiter != biter) {
            vecArray = (*aiter).second.array.get();
            maskArray = (*aiter).second.mask.get();
            if ((maskArray[num] & mask) != 0)
                break; // Reached point where this value was definitively set before
            res.push_back(vecArray);
            ++aiter;
        }
    }

    virtual uintm* getDefaultValue(void) {
        return database.defaultValue().array.get();
    }

    virtual const uintm* getDefaultValue(void) const {
        return database.defaultValue().array.get();
    }

public:
    ContextInternal(void) {
        size = 0;
    }
    virtual ~ContextInternal(void) {}

    virtual int4 getContextSize(void) const {
        return size;
    }

    /// Register a named context variable and grow the default context blob if necessary.
    virtual void registerVariable(const string& nm, int4 sbit, int4 ebit) {
        if (!database.empty())
            throw LowlevelError("Cannot register new context variables after database is initialized");

        ContextBitRange bitrange(sbit, ebit);
        int4 sz = sbit / (8 * sizeof(uintm)) + 1;
        if ((ebit / (8 * sizeof(uintm)) + 1) != sz)
            throw LowlevelError("Context variable does not fit in one word");
        if (sz > size) {
            size = sz;
            database.defaultValue().reset(size);
        }
        variables[nm] = bitrange;
    }

    virtual const uintm* getContext(const Address& addr) const {
        return database.getValue(addr).array.get();
    }

    /// Retrieve a context blob and its valid address bounds.
    virtual const uintm* getContext(const Address& addr, uintb& first, uintb& last) const {
        int4 valid;
        Address before, after;
        const uintm* res = database.bounds(addr, before, after, valid).array.get();
        if (((valid & 1) != 0) || (before.getSpace() != addr.getSpace()))
            first = 0;
        else
            first = before.getOffset();
        if (((valid & 2) != 0) || (after.getSpace() != addr.getSpace()))
            last = addr.getSpace()->getHighest();
        else
            last = after.getOffset() - 1;
        return res;
    }

    virtual TrackedSet& getTrackedDefault(void) {
        return trackbase.defaultValue();
    }

    virtual const TrackedSet& getTrackedSet(const Address& addr) const {
        return trackbase.getValue(addr);
    }

    /// Create and clear a tracked register set for an address range.
    virtual TrackedSet& createSet(const Address& addr1, const Address& addr2) {
        TrackedSet& res(trackbase.clearRange(addr1, addr2));
        res.clear();
        return res;
    }

    virtual void encode(Encoder& encoder) const {
        if (database.empty() && trackbase.empty())
            return;

        encoder.openElement(ELEM_CONTEXT_POINTS);

        partmap<Address, FreeArray>::const_iterator fiter, fenditer;
        fiter = database.begin();
        fenditer = database.end();
        for (; fiter != fenditer; ++fiter) // Save context at each changepoint
            encodeContext(encoder, (*fiter).first, (*fiter).second.array.get());

        partmap<Address, TrackedSet>::const_iterator titer, tenditer;
        titer = trackbase.begin();
        tenditer = trackbase.end();
        for (; titer != tenditer; ++titer)
            encodeTracked(encoder, (*titer).first, (*titer).second);

        encoder.closeElement(ELEM_CONTEXT_POINTS);
    }

    virtual void decode(Decoder& decoder) {
        uint4 elemId = decoder.openElement(ELEM_CONTEXT_POINTS);
        for (;;) {
            uint4 subId = decoder.openElement();
            if (subId == 0)
                break;
            if (subId == ELEM_CONTEXT_POINTSET) {
                uint4 attribId = decoder.getNextAttributeId();
                decoder.rewindAttributes();
                if (attribId == 0) {
                    decodeContext(decoder, Address(), Address()); // Restore the default value
                } else {
                    VarnodeData vData;
                    vData.decodeFromAttributes(decoder);
                    decodeContext(decoder, vData.getAddr(), Address());
                }
            } else if (subId == ELEM_TRACKED_POINTSET) {
                VarnodeData vData;
                vData.decodeFromAttributes(decoder);
                decodeTracked(decoder, trackbase.split(vData.getAddr()));
            } else
                throw LowlevelError("Bad <context_points> tag");
            decoder.closeElement(subId);
        }
        decoder.closeElement(elemId);
    }

    virtual void decodeFromSpec(Decoder& decoder) {
        uint4 elemId = decoder.openElement(ELEM_CONTEXT_DATA);
        for (;;) {
            uint4 subId = decoder.openElement();
            if (subId == 0)
                break;
            Range range;
            range.decodeFromAttributes(decoder); // There MUST be a range
            Address addr1 = range.getFirstAddr();
            Address addr2 = range.getLastAddrOpen(decoder.getAddrSpaceManager());
            if (subId == ELEM_CONTEXT_SET) {
                decodeContext(decoder, addr1, addr2);
            } else if (subId == ELEM_TRACKED_SET) {
                decodeTracked(decoder, createSet(addr1, addr2));
            } else
                throw LowlevelError("Bad <context_data> tag");
            decoder.closeElement(subId);
        }
        decoder.closeElement(elemId);
    }
};

/// \brief A helper class for caching the active context blob to minimize database lookups
///
/// This merely caches the last retrieved context blob ("array of words") and the range of
/// addresses over which the blob is valid. It encapsulates the ContextDatabase itself and
/// exposes a minimal interface (getContext() and setContext()).
class ContextCache {
    ContextDatabase* database;    ///< The encapsulated context database
    bool allowset;                ///< If set to \b false, and setContext() call is dropped
    mutable AddrSpace* curspace;  ///< Address space of the current valid range
    mutable uintb first;          ///< Starting offset of the current valid range
    mutable uintb last;           ///< Ending offset of the current valid range
    mutable const uintm* context; ///< The current cached context blob
public:
    /// \param db is the context database that will be encapsulated
    ContextCache(ContextDatabase* db) {
        database = db;
        curspace = (AddrSpace*)0; // Mark cache as invalid
        allowset = true;
    }

    ContextDatabase* getDatabase(void) const {
        return database;
    } ///< Retrieve the encapsulated database object
    void allowSet(bool val) {
        allowset = val;
    } ///< Toggle whether setContext() calls are ignored

    /// Check if the address is in the current valid range. If it is, return the cached
    /// blob. Otherwise, make a call to the database and cache a new block and valid range.
    /// \param addr is the given address
    /// \param buf is where the blob should be stored
    void getContext(const Address& addr, uintm* buf) const {
        if ((addr.getSpace() != curspace) || (first > addr.getOffset()) || (last < addr.getOffset())) {
            curspace = addr.getSpace();
            context = database->getContext(addr, first, last);
        }
        for (int4 i = 0; i < database->getContextSize(); ++i)
            buf[i] = context[i];
    }

    /// \brief Change the value of a context variable at the given address with no bound
    ///
    /// The context value is set starting at the given address and \e paints memory up
    /// to the next explicit change point.
    /// \param addr is the given starting address
    /// \param num is the word index of the context variable
    /// \param mask is the mask delimiting the context variable
    /// \param value is the (already shifted) value to set
    void setContext(const Address& addr, int4 num, uintm mask, uintm value) {
        if (!allowset)
            return;
        database->setContextChangePoint(addr, num, mask, value);
        if ((addr.getSpace() == curspace) && (first <= addr.getOffset()) && (last >= addr.getOffset()))
            curspace = (AddrSpace*)0; // Invalidate cache
    }

    /// \brief Change the value of a context variable across an explicit address range
    ///
    /// The context value is \e painted across the range. The context variable is marked as
    /// explicitly changing at the starting address of the range.
    /// \param addr1 is the starting address of the given range
    /// \param addr2 is the ending address of the given range
    /// \param num is the word index of the context variable
    /// \param mask is the mask delimiting the context variable
    /// \param value is the (already shifted) value to set
    void setContext(const Address& addr1, const Address& addr2, int4 num, uintm mask, uintm value) {
        if (!allowset)
            return;
        database->setContextRegion(addr1, addr2, num, mask, value);
        if ((addr1.getSpace() == curspace) && (first <= addr1.getOffset()) && (last >= addr1.getOffset()))
            curspace = (AddrSpace*)0; // Invalidate cache
        if ((first <= addr2.getOffset()) && (last >= addr2.getOffset()))
            curspace = (AddrSpace*)0; // Invalidate cache
        if ((first >= addr1.getOffset()) && (first <= addr2.getOffset()))
            curspace = (AddrSpace*)0; // Invalidate cache
    }
};

} // End namespace ghidra
#endif
