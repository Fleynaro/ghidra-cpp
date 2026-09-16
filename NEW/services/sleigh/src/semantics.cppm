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
/// \file semantics.cppm
/// \brief Classes describing p-code operations as parsed from a SLEIGH specification

export module sleigh_runtime:semantics;
export import :context;
export import :slaformat;

export namespace ghidra {

// These named constants preserve the legacy directive/opcode mapping without
// relying on preprocessor state across module boundaries.
inline constexpr OpCode BUILD = CPUI_MULTIEQUAL;
inline constexpr OpCode DELAY_SLOT = CPUI_INDIRECT;
inline constexpr OpCode CROSSBUILD = CPUI_PTRSUB;
inline constexpr OpCode MACROBUILD = CPUI_CAST;
inline constexpr OpCode LABELBUILD = CPUI_PTRADD;

class Translate; // Forward declaration
class ConstTpl;
class HandleTpl; // Forward declaration

/// Copies a handle value into a constant after HandleTpl is complete.
void constTplTransfer(ConstTpl& value, const vector<HandleTpl*>& params);
/// Tests a handle size without requiring its later class definition here.
bool handleTplSizeIsZero(const HandleTpl* handle);

/// \brief A constant value encountered during SLEIGH parsing
///
/// Any offset, size, address space, or literal constant encountered during parsing that ultimately resolves
/// to a constant value when disassembling or generating p-code for a specific instruction.  The object holds details
/// about how to calculate the final constant from the SLEIGH context for the instruction.
class ConstTpl {
    friend void constTplTransfer(ConstTpl&, const vector<HandleTpl*>&);

public:
    /// Types of constant values encountered during SLEIGH parsing.
    enum const_type {
        real = 0,            ///< A literal constant
        handle = 1,          ///< Placeholder for a value passed back by a sub-constructor
        j_start = 2,         ///< Address offset for the start of the current instruction
        j_next = 3,          ///< Address offset for the start of the next instruction
        j_next2 = 4,         ///< Address offset of the instruction immediately after the next instruction
        j_curspace = 5,      ///< Address space containing the current instruction
        j_curspace_size = 6, ///< Number of bytes encoding the address of the current instruction
        spaceid = 7,         ///< An address space (encoded as a constant)
        j_relative = 8,      ///< A relative p-code branch offset
        j_flowref = 9,       ///< Address offset of any call reference at site of p-code injection
        j_flowref_size = 10, ///< Number of bytes encoding address of call reference
        j_flowdest = 11,     ///< Address offset of call destination being injected
        j_flowdest_size = 12 ///< Number of bytes encoding address of call destination
    };
    /// For a \b handle (a value calculated in a sub-constructor), we can sub-select which part of the value to use.
    enum v_field {
        v_space = 0,      ///< The address space associated with the \b handle
        v_offset = 1,     ///< The offset associated with the \b handle
        v_size = 2,       ///< Number of bytes in the \b handle
        v_offset_plus = 3 ///< The offset associated with the \b handle plus an additional constant offset
    };

private:
    const_type type; ///< The type of constant
    union {
        AddrSpace* spaceid; ///< Id referring to a registered address space
        int4 handle_index;  ///< For a \b handle, the index of the specific sub-constructor passing back the value
    } value;                ///< Specialized values a ConstTpl can represent
    uintb value_real;       ///< An immediate constant value or other constant offset a ConstTpl represents
    v_field select;         ///< Assuming ConstTpl is a \b handle, the part of the \b handle to use as constant

public:
    ConstTpl(void) {
        type = real;
        value_real = 0;
    } ///< Construct a zero constant
    ConstTpl(const ConstTpl& op2) {
        type = op2.type;
        value = op2.value;
        value_real = op2.value_real;
        select = op2.select;
    } ///< Copy constructor
    ConstTpl(const_type tp, uintb val) {
        /// \param tp is constant type: either \b real or \b j_relative
        /// \param val is the constant value
        type = tp;
        value_real = val;
        value.handle_index = 0;
        select = v_space;
    }
    ConstTpl(const_type tp) {
        /// \param tp is the specialized constant type: \b j_start, \b j_next, \b j_next2, \b j_flowdest,
        /// \b j_curspace, etc.
        type = tp;
    }
    ConstTpl(AddrSpace* sid) {
        /// \param sid is the address space to encode
        type = spaceid;
        value.spaceid = sid;
    }
    ConstTpl(const_type tp, int4 ht, v_field vf) {
        /// \param tp is the constant type:  must be \b handle
        /// \param ht is the index of the sub-constructor computing the \b handle value
        /// \param vf is the part (space, offset, or size) of the computed value to encode as the constant
        type = handle;
        value.handle_index = ht;
        select = vf;
        value_real = 0;
    }
    ConstTpl(const_type tp, int4 ht, v_field vf, uintb plus) {
        /// \param tp is the constant type: must be \b handle
        /// \param ht is the index of the sub-constructor computing the \b handle value
        /// \param vf is the part of the computed value to encode: must be \b v_offset_plus
        /// \param plus is the additional constant value to add to the \b handle offset
        type = handle;
        value.handle_index = ht;
        select = vf;
        value_real = plus;
    }
    bool isConstSpace(void) const {
        if (type == spaceid)
            return (value.spaceid->getType() == IPTR_CONSTANT);
        return false;
    }
    bool isUniqueSpace(void) const {
        if (type == spaceid)
            return (value.spaceid->getType() == IPTR_INTERNAL);
        return false;
    }
    bool operator==(const ConstTpl& op2) const {
        /// The constants must be equal in both value and type.
        /// \param op2 is the constant to compare with \b this
        /// \return \b true if \b this is equal to \b op2, \b false otherwise
        if (type != op2.type)
            return false;
        switch (type) {
            case real:
                return (value_real == op2.value_real);
            case handle:
                if (value.handle_index != op2.value.handle_index)
                    return false;
                if (select != op2.select)
                    return false;
                break;
            case spaceid:
                return (value.spaceid == op2.value.spaceid);
            default: // Nothing additional to compare
                break;
        }
        return true;
    }
    bool operator<(const ConstTpl& op2) const {
        /// \param op2 is the constant to compare with \b this
        /// \return \b true if \b this should be ordered before \b op2
        if (type != op2.type)
            return (type < op2.type);
        switch (type) {
            case real:
                return (value_real < op2.value_real);
            case handle:
                if (value.handle_index != op2.value.handle_index)
                    return (value.handle_index < op2.value.handle_index);
                if (select != op2.select)
                    return (select < op2.select);
                break;
            case spaceid:
                return (value.spaceid < op2.value.spaceid);
            default: // Nothing additional to compare
                break;
        }
        return false;
    }
    uintb getReal(void) const {
        return value_real;
    } ///< Get the literal constant value associated with \b this
    AddrSpace* getSpace(void) const {
        return value.spaceid;
    } ///< Get the address space \b this is encoding
    int4 getHandleIndex(void) const {
        return value.handle_index;
    } ///< Get the index of the sub-constructor computing \b this
    const_type getType(void) const {
        return type;
    } ///< Get the constant type
    v_field getSelect(void) const {
        return select;
    } ///< Get the type of \b handle piece \b this is encoding
    uintb fix(const ParserWalker& walker) const {
        /// If this is a \b handle associated with a dynamically computed value, this method returns
        /// the properties of the temporary storage used to hold the computed value.
        /// \param walker is the context for the parse of a single instruction
        /// \return the computed constant value
        switch (type) {
            case j_start:
                return walker.getAddr().getOffset(); // Fill in starting address placeholder with real address
            case j_next:
                return walker.getNaddr().getOffset(); // Fill in next address placeholder with real address
            case j_next2:
                return walker.getN2addr().getOffset(); // Fill in next2 address placeholder with real address
            case j_flowref:
                return walker.getRefAddr().getOffset();
            case j_flowref_size:
                return walker.getRefAddr().getAddrSize();
            case j_flowdest:
                return walker.getDestAddr().getOffset();
            case j_flowdest_size:
                return walker.getDestAddr().getAddrSize();
            case j_curspace_size:
                return walker.getCurSpace()->getAddrSize();
            case j_curspace:
                return (uintb)(uintp)walker.getCurSpace();
            case handle: {
                const FixedHandle& hand(walker.getFixedHandle(value.handle_index));
                switch (select) {
                    case v_space:
                        if (hand.offset_space == (AddrSpace*)0)
                            return (uintb)(uintp)hand.space;
                        return (uintb)(uintp)hand.temp_space;
                    case v_offset:
                        if (hand.offset_space == (AddrSpace*)0)
                            return hand.offset_offset;
                        return hand.temp_offset;
                    case v_size:
                        return hand.size;
                    case v_offset_plus:
                        if (hand.space != walker.getConstSpace()) { // If we are not a constant
                            if (hand.offset_space == (AddrSpace*)0)
                                return hand.offset_offset + (value_real & 0xffff); // Adjust offset by truncation amount
                            return hand.temp_offset + (value_real & 0xffff);
                        } else { // If we are a constant, we want to return a shifted value
                            uintb val;
                            if (hand.offset_space == (AddrSpace*)0)
                                val = hand.offset_offset;
                            else
                                val = hand.temp_offset;
                            val >>= 8 * (value_real >> 16);
                            return val;
                        }
                }
                break;
            }
            case j_relative:
            case real:
                return value_real;
            case spaceid:
                return (uintb)(uintp)value.spaceid;
        }
        return 0; // Should never reach here
    }
    AddrSpace* fixSpace(const ParserWalker& walker) const {
        // Get the value of the ConstTpl in context
        // when we know it is a space
        switch (type) {
            case j_curspace:
                return walker.getCurSpace();
            case handle: {
                const FixedHandle& hand(walker.getFixedHandle(value.handle_index));
                switch (select) {
                    case v_space:
                        if (hand.offset_space == (AddrSpace*)0)
                            return hand.space;
                        return hand.temp_space;
                    default:
                        break;
                }
                break;
            }
            case spaceid:
                return value.spaceid;
            case j_flowref:
                return walker.getRefAddr().getSpace();
            default:
                break;
        }
        throw LowlevelError("ConstTpl is not a spaceid as expected");
    }
    void transfer(const vector<HandleTpl*>& params) {
        constTplTransfer(*this, params);
    }
    bool isZero(void) const {
        return ((type == real) && (value_real == 0));
    } ///< Return \b true if \b this is a literal zero
    void changeHandleIndex(const vector<int4>& handmap) {
        /// This is used to help reorder sub-constructors.
        /// If \b this is a \b handle, its index is translated by looking up a new value in the given array of indices.
        /// \param handmap is the given array of new \b handle indices
        if (type == handle)
            value.handle_index = handmap[value.handle_index];
    }
    void fillinSpace(FixedHandle& hand, const ParserWalker& walker) const {
        /// If \b this represents an address space, including if \b this is the address space
        /// piece of a \b handle, replace the address space portion of the FixedHandle with \b this.
        /// \param hand is the FixedHandle to replace
        /// \param walker is the current context
        switch (type) {
            case j_curspace:
                hand.space = walker.getCurSpace();
                return;
            case handle: {
                const FixedHandle& otherhand(walker.getFixedHandle(value.handle_index));
                switch (select) {
                    case v_space:
                        hand.space = otherhand.space;
                        return;
                    default:
                        break;
                }
                break;
            }
            case spaceid:
                hand.space = value.spaceid;
                return;
            default:
                break;
        }
        throw LowlevelError("ConstTpl is not a spaceid as expected");
    }
    void fillinOffset(FixedHandle& hand, const ParserWalker& walker) const {
        /// First, \b this is \e fixed, based on the current context.  Then the final fixed
        /// value is copied into the FixedHandle.  If \b this itself is a \b handle, the
        /// entire fixed handle is copied into the FixedHandle.
        /// \param hand is the FixedHandle to copy into
        /// \param walker is the context to fix against
        // If the offset value is dynamic, indicate this in the handle
        // we don't just fill in the temporary variable offset
        // we assume hand.space is already filled in
        if (type == handle) {
            const FixedHandle& otherhand(walker.getFixedHandle(value.handle_index));
            hand.offset_space = otherhand.offset_space;
            hand.offset_offset = otherhand.offset_offset;
            hand.offset_size = otherhand.offset_size;
            hand.temp_space = otherhand.temp_space;
            hand.temp_offset = otherhand.temp_offset;
        } else {
            hand.offset_space = (AddrSpace*)0;
            hand.offset_offset = hand.space->wrapOffset(fix(walker));
        }
    }
    void encode(Encoder& encoder) const {
        /// \param encoder is the output stream
        switch (type) {
            case real:
                encoder.openElement(sla::ELEM_CONST_REAL);
                encoder.writeUnsignedInteger(sla::ATTRIB_VAL, value_real);
                encoder.closeElement(sla::ELEM_CONST_REAL);
                break;
            case handle:
                encoder.openElement(sla::ELEM_CONST_HANDLE);
                encoder.writeSignedInteger(sla::ATTRIB_VAL, value.handle_index);
                encoder.writeSignedInteger(sla::ATTRIB_S, select);
                if (select == v_offset_plus)
                    encoder.writeUnsignedInteger(sla::ATTRIB_PLUS, value_real);
                encoder.closeElement(sla::ELEM_CONST_HANDLE);
                break;
            case j_start:
                encoder.openElement(sla::ELEM_CONST_START);
                encoder.closeElement(sla::ELEM_CONST_START);
                break;
            case j_next:
                encoder.openElement(sla::ELEM_CONST_NEXT);
                encoder.closeElement(sla::ELEM_CONST_NEXT);
                break;
            case j_next2:
                encoder.openElement(sla::ELEM_CONST_NEXT2);
                encoder.closeElement(sla::ELEM_CONST_NEXT2);
                break;
            case j_curspace:
                encoder.openElement(sla::ELEM_CONST_CURSPACE);
                encoder.closeElement(sla::ELEM_CONST_CURSPACE);
                break;
            case j_curspace_size:
                encoder.openElement(sla::ELEM_CONST_CURSPACE_SIZE);
                encoder.closeElement(sla::ELEM_CONST_CURSPACE_SIZE);
                break;
            case spaceid:
                encoder.openElement(sla::ELEM_CONST_SPACEID);
                encoder.writeSpace(sla::ATTRIB_SPACE, value.spaceid);
                encoder.closeElement(sla::ELEM_CONST_SPACEID);
                break;
            case j_relative:
                encoder.openElement(sla::ELEM_CONST_RELATIVE);
                encoder.writeUnsignedInteger(sla::ATTRIB_VAL, value_real);
                encoder.closeElement(sla::ELEM_CONST_RELATIVE);
                break;
            case j_flowref:
                encoder.openElement(sla::ELEM_CONST_FLOWREF);
                encoder.closeElement(sla::ELEM_CONST_FLOWREF);
                break;
            case j_flowref_size:
                encoder.openElement(sla::ELEM_CONST_FLOWREF_SIZE);
                encoder.closeElement(sla::ELEM_CONST_FLOWREF_SIZE);
                break;
            case j_flowdest:
                encoder.openElement(sla::ELEM_CONST_FLOWDEST);
                encoder.closeElement(sla::ELEM_CONST_FLOWDEST);
                break;
            case j_flowdest_size:
                encoder.openElement(sla::ELEM_CONST_FLOWDEST_SIZE);
                encoder.closeElement(sla::ELEM_CONST_FLOWDEST_SIZE);
                break;
        }
    }
    void decode(Decoder& decoder) {
        /// \param decoder is the input stream
        uint4 el = decoder.openElement();
        if (el == sla::ELEM_CONST_REAL) {
            type = real;
            value_real = decoder.readUnsignedInteger(sla::ATTRIB_VAL);
        } else if (el == sla::ELEM_CONST_HANDLE) {
            type = handle;
            value.handle_index = decoder.readSignedInteger(sla::ATTRIB_VAL);
            uint4 selectInt = decoder.readSignedInteger(sla::ATTRIB_S);
            if (selectInt > v_offset_plus)
                throw DecoderError("Bad handle selector encoding");
            select = (v_field)selectInt;
            if (select == v_offset_plus) {
                value_real = decoder.readUnsignedInteger(sla::ATTRIB_PLUS);
            }
        } else if (el == sla::ELEM_CONST_START) {
            type = j_start;
        } else if (el == sla::ELEM_CONST_NEXT) {
            type = j_next;
        } else if (el == sla::ELEM_CONST_NEXT2) {
            type = j_next2;
        } else if (el == sla::ELEM_CONST_CURSPACE) {
            type = j_curspace;
        } else if (el == sla::ELEM_CONST_CURSPACE_SIZE) {
            type = j_curspace_size;
        } else if (el == sla::ELEM_CONST_SPACEID) {
            type = spaceid;
            value.spaceid = decoder.readSpace(sla::ATTRIB_SPACE);
        } else if (el == sla::ELEM_CONST_RELATIVE) {
            type = j_relative;
            value_real = decoder.readUnsignedInteger(sla::ATTRIB_VAL);
        } else if (el == sla::ELEM_CONST_FLOWREF) {
            type = j_flowref;
        } else if (el == sla::ELEM_CONST_FLOWREF_SIZE) {
            type = j_flowref_size;
        } else if (el == sla::ELEM_CONST_FLOWDEST) {
            type = j_flowdest;
        } else if (el == sla::ELEM_CONST_FLOWDEST_SIZE) {
            type = j_flowdest_size;
        } else
            throw LowlevelError("Bad constant type");
        decoder.closeElement(el);
    }
};

/// \brief A (partially) resolved Varnode in a SLEIGH specification
///
/// Variable symbols in SLEIGH are represented using \b this object.  It encodes the
/// address space, offset, and size associated with the variable.  In the context of
/// a specific instruction, \b this is translated into a Varnode object.
class VarnodeTpl {
    friend class OpTpl;
    friend class HandleTpl;
    ConstTpl space;    ///< Address space associated with \b this variable
    ConstTpl offset;   ///< Offset into the address space
    ConstTpl size;     ///< Number of bytes in \b this variable
    bool unnamed_flag; ///< Set to \b true if \b this is an unnamed temporary register
public:
    VarnodeTpl(int4 hand, bool zerosize)
        : space(ConstTpl::handle, hand, ConstTpl::v_space), offset(ConstTpl::handle, hand, ConstTpl::v_offset),
          size(ConstTpl::handle, hand, ConstTpl::v_size) {
        /// \param hand is the index of the sub-constructor computing the \b handle value
        /// \param zerosize is \b true if the size of \b this should be disassociated from the computed \b handle
        /// and forced to zero
        if (zerosize)
            size = ConstTpl(ConstTpl::real, 0); // if zerosize is true, set the size constant to zero
        unnamed_flag = false;
    }
    VarnodeTpl(void) : space(), offset(), size() {
        unnamed_flag = false;
    } ///< Construct an uninitialized VarnodeTpl
    VarnodeTpl(const ConstTpl& sp, const ConstTpl& off, const ConstTpl& sz) : space(sp), offset(off), size(sz) {
        /// \param sp represents the address space
        /// \param off represents the offset into the address space
        /// \param sz represents the number of bytes
        unnamed_flag = false;
    }
    VarnodeTpl(const VarnodeTpl& vn) : space(vn.space), offset(vn.offset), size(vn.size) {
        /// \param vn is the VarnodeTpl to copy
        unnamed_flag = vn.unnamed_flag;
    }
    const ConstTpl& getSpace(void) const {
        return space;
    } ///< Get the address space
    const ConstTpl& getOffset(void) const {
        return offset;
    } ///< Get the offset
    const ConstTpl& getSize(void) const {
        return size;
    } ///< Get the size
    bool isDynamic(const ParserWalker& walker) const {
        /// If the offset is computed by a sub-constructor using a p-code LOAD into a temporary register,
        /// then return \b true.  If the sub-constructor does not use a LOAD, or \b this is not a \b handle,
        /// return \b false.
        /// \param walker is the instruction context (used to look-up the specific sub-constructor)
        /// \return \b true if \b this is a dynamic value computed by a sub-constructor
        if (offset.getType() != ConstTpl::handle)
            return false;
        // Technically we should probably check all three
        // ConstTpls for dynamic handles, but in all cases
        // if there is any dynamic piece then the offset is
        const FixedHandle& hand(walker.getFixedHandle(offset.getHandleIndex()));
        return (hand.offset_space != (AddrSpace*)0);
    }
    int4 transfer(const vector<HandleTpl*>& params) {
        /// For any piece of \b this (address space,offset,size) that is a handle, copy the HandleTpl with matching
        /// index. If \b this needs an additional constant added to its final offset piece, return that constant.
        /// \param params is the given array of HandleTpl to match against
        /// \return any additional constant that still needs to be added in, or -1 otherwise
        bool doesOffsetPlus = false;
        int4 handleIndex;
        int4 plus;
        if ((offset.getType() == ConstTpl::handle) && (offset.getSelect() == ConstTpl::v_offset_plus)) {
            handleIndex = offset.getHandleIndex();
            plus = (int4)offset.getReal();
            doesOffsetPlus = true;
        }
        space.transfer(params);
        offset.transfer(params);
        size.transfer(params);
        if (doesOffsetPlus) {
            if (isLocalTemp())
                return plus; // A positive number indicates truncation of a local temp
            if (handleTplSizeIsZero(params[handleIndex]))
                return plus; //    or a zerosize object
        }
        return -1;
    }
    bool isZeroSize(void) const {
        return size.isZero();
    } ///< Return \b true if \b this currently has a size of zero
    bool operator==(const VarnodeTpl& op2) const {
        /// \param op2 is the VarnodeTpl to compare with \b this
        /// \return \b true if address space, offset, and size or all equal
        return space == op2.space && offset == op2.offset && size == op2.size;
    }
    bool operator!=(const VarnodeTpl& op2) const {
        /// \param op2 is the VarnodeTpl to compare with \b this
        /// \return \b true if address space, offset, or size is not equal
        return !(*this == op2);
    }
    bool operator<(const VarnodeTpl& op2) const {
        /// Order by address space, then offset, then size
        /// \param op2 is the VarnodeTpl to order with \b this
        /// \return \b true if \b this should come before \b op2
        if (!(space == op2.space))
            return (space < op2.space);
        if (!(offset == op2.offset))
            return (offset < op2.offset);
        if (!(size == op2.size))
            return (size < op2.size);
        return false;
    }
    void setOffset(uintb constVal) {
        offset = ConstTpl(ConstTpl::real, constVal);
    } ///< Set the offset piece to a literal constant
    void setRelative(uintb constVal) {
        offset = ConstTpl(ConstTpl::j_relative, constVal);
    } ///< Set the offset piece to a relative branch offset
    void setSize(const ConstTpl& sz) {
        size = sz;
    } ///< Set the size piece
    bool isUnnamed(void) const {
        return unnamed_flag;
    } ///< Return \b true if \b this is an unnamed temporary register
    void setUnnamed(bool val) {
        unnamed_flag = val;
    } ///< Mark \b this as an unnamed temporary register
    bool isLocalTemp(void) const {
        if (space.getType() != ConstTpl::spaceid)
            return false;
        if (space.getSpace()->getType() != IPTR_INTERNAL)
            return false;
        return true;
    }
    bool isRelative(void) const {
        return (offset.getType() == ConstTpl::j_relative);
    } ///< Return \b true if \b this is a relative branch offset
    void changeHandleIndex(const vector<int4>& handmap) {
        /// This is used to help reorder sub-constructors.
        /// For each piece of \b this, if it is a \b handle, its index is translated by looking up a new value in the
        /// given array of indices.
        /// \param handmap is the given array of new \b handle indices
        space.changeHandleIndex(handmap);
        offset.changeHandleIndex(handmap);
        size.changeHandleIndex(handmap);
    }
    bool adjustTruncation(int4 sz, bool isbigendian) {
        /// The offset piece must be \b v_offset_plus, indicating \b this is truncated.
        /// Compute the final form of the truncation given the final size and endianness.
        /// Also check that the truncation is in bounds for the given final size.
        /// \param sz is the final size of the Varnode in bytes
        /// \param isbigendian is \b true if the address space is big endian.
        /// \return \b true if the truncation is in bounds
        if (size.getType() != ConstTpl::real)
            return false;
        int4 numbytes = (int4)size.getReal();
        int4 byteoffset = (int4)offset.getReal();
        if (numbytes + byteoffset > sz)
            return false;

        // Encode the original truncation amount with the plus value
        uintb val = byteoffset;
        val <<= 16;
        if (isbigendian) {
            val |= (uintb)(sz - (numbytes + byteoffset));
        } else {
            val |= (uintb)byteoffset;
        }

        offset = ConstTpl(ConstTpl::handle, offset.getHandleIndex(), ConstTpl::v_offset_plus, val);
        return true;
    }
    void encode(Encoder& encoder) const {
        /// \param encoder is the output stream
        encoder.openElement(sla::ELEM_VARNODE_TPL);
        space.encode(encoder);
        offset.encode(encoder);
        size.encode(encoder);
        encoder.closeElement(sla::ELEM_VARNODE_TPL);
    }
    void decode(Decoder& decoder) {
        /// \param decoder is the input stream
        uint4 el = decoder.openElement(sla::ELEM_VARNODE_TPL);
        space.decode(decoder);
        offset.decode(decoder);
        size.decode(decoder);
        decoder.closeElement(el);
    }
};

/// \brief An \e exported value of a sub-constructor in a SLEIGH specification
///
/// For an output value that is equivalent to a VarnodeTpl, \b space, \b ptroffset, and \b size correspond to
/// Varnode::space, Varnode::offset, and Varnode::size.  But a HandleTpl can also represent a dynamic value loaded
/// at run-time.  In this case, the final value is stored in a temporary register specified by
/// \b temp_space, \b temp_offset (and \b size), and the pointer used to load the final value is specified by
/// \b ptrspace, \b ptroffset, and \b ptrsize.
class HandleTpl {
    ConstTpl space;       ///< The address space of the value
    ConstTpl size;        ///< The size of the value
    ConstTpl ptrspace;    ///< (If dynamic) the address space of the pointer
    ConstTpl ptroffset;   ///< If dynamic, the offset of the pointer, or the offset of the value otherwise
    ConstTpl ptrsize;     ///< (If dynamic) the size of the pointer
    ConstTpl temp_space;  ///< (If dynamic) the address space of the temporary register holding the final value
    ConstTpl temp_offset; ///< (If dynamic) the offset of the temporary register
public:
    HandleTpl(void) {} ///< Construct an uninitialized HandleTpl
    HandleTpl(const VarnodeTpl* vn) {
        /// The constructed HandleTpl is not dynamic and matches the given VarnodeTpl
        /// \param vn is the given VarnodeTpl
        space = vn->getSpace();
        size = vn->getSize();
        ptrspace = ConstTpl(ConstTpl::real, 0);
        ptroffset = vn->getOffset();
    }
    HandleTpl(const ConstTpl& spc, const ConstTpl& sz, const VarnodeTpl* vn, AddrSpace* t_space, uintb t_offset)
        : space(spc), size(sz), ptrspace(vn->getSpace()), ptroffset(vn->getOffset()), ptrsize(vn->getSize()),
          temp_space(t_space), temp_offset(ConstTpl::real, t_offset) {
        /// \param spc is the address space
        /// \param sz is the size
        /// \param vn is the varnode representing the dynamic pointer
        /// \param t_space is the address space of the temporary register
        /// \param t_offset is the offset of the temporary register
    }
    const ConstTpl& getSpace(void) const {
        return space;
    } ///< Get the address space
    const ConstTpl& getPtrSpace(void) const {
        return ptrspace;
    } ///< Get the pointer address space
    const ConstTpl& getPtrOffset(void) const {
        return ptroffset;
    } ///< Get the offset (or the pointer offset)
    const ConstTpl& getPtrSize(void) const {
        return ptrsize;
    } ///< Get the pointer size
    const ConstTpl& getSize(void) const {
        return size;
    } ///< Get the size
    const ConstTpl& getTempSpace(void) const {
        return temp_space;
    } ///< Get the temporary register address space
    const ConstTpl& getTempOffset(void) const {
        return temp_offset;
    } ///< Get the temporary register offset
    void setSize(const ConstTpl& sz) {
        size = sz;
    } ///< Set the size
    void setPtrSize(const ConstTpl& sz) {
        ptrsize = sz;
    } ///< Set the pointer size
    void setPtrOffset(uintb val) {
        ptroffset = ConstTpl(ConstTpl::real, val);
    } ///< Set the pointer offset
    void setTempOffset(uintb val) {
        temp_offset = ConstTpl(ConstTpl::real, val);
    } ///< Set the temporary register offset
    void fix(FixedHandle& hand, const ParserWalker& walker) const {
        /// The final constant values for \b this are computed in context and stored in the given FixedHandle object.
        /// \param hand is FixedHandle holding the final \b handle constants
        /// \param walker is the context used to fix constants
        if (ptrspace.getType() == ConstTpl::real) {
            // The export is unstarred, but this doesn't mean the varnode
            // being exported isn't dynamic
            space.fillinSpace(hand, walker);
            hand.size = size.fix(walker);
            ptroffset.fillinOffset(hand, walker);
        } else {
            hand.space = space.fixSpace(walker);
            hand.size = size.fix(walker);
            hand.offset_offset = ptroffset.fix(walker);
            hand.offset_space = ptrspace.fixSpace(walker);
            if (hand.offset_space->getType() == IPTR_CONSTANT) {
                // Handle could have been dynamic but wasn't
                hand.offset_space = (AddrSpace*)0;
                hand.offset_offset = AddrSpace::addressToByte(hand.offset_offset, hand.space->getWordSize());
                hand.offset_offset = hand.space->wrapOffset(hand.offset_offset);
            } else {
                hand.offset_size = ptrsize.fix(walker);
                hand.temp_space = temp_space.fixSpace(walker);
                hand.temp_offset = temp_offset.fix(walker);
            }
        }
    }
    void changeHandleIndex(const vector<int4>& handmap) {
        /// This is used to help reorder sub-constructors.
        /// For each piece of \b this, if it is a \b handle, its index is translated by looking up a new value in the
        /// given array of indices.
        /// \param handmap is the given array of new \b handle indices
        space.changeHandleIndex(handmap);
        size.changeHandleIndex(handmap);
        ptrspace.changeHandleIndex(handmap);
        ptroffset.changeHandleIndex(handmap);
        ptrsize.changeHandleIndex(handmap);
        temp_space.changeHandleIndex(handmap);
        temp_offset.changeHandleIndex(handmap);
    }
    void encode(Encoder& encoder) const {
        /// \param encoder is the output stream
        encoder.openElement(sla::ELEM_HANDLE_TPL);
        space.encode(encoder);
        size.encode(encoder);
        ptrspace.encode(encoder);
        ptroffset.encode(encoder);
        ptrsize.encode(encoder);
        temp_space.encode(encoder);
        temp_offset.encode(encoder);
        encoder.closeElement(sla::ELEM_HANDLE_TPL);
    }
    void decode(Decoder& decoder) {
        /// \param decoder is the input stream
        uint4 el = decoder.openElement(sla::ELEM_HANDLE_TPL);
        space.decode(decoder);
        size.decode(decoder);
        ptrspace.decode(decoder);
        ptroffset.decode(decoder);
        ptrsize.decode(decoder);
        temp_space.decode(decoder);
        temp_offset.decode(decoder);
        decoder.closeElement(el);
    }
};

/// Copies the selected component from a resolved handle into a ConstTpl.
inline void constTplTransfer(ConstTpl& value, const vector<HandleTpl*>& params) {
    if (value.type != ConstTpl::handle)
        return;
    HandleTpl* handle = params[value.value.handle_index];
    switch (value.select) {
        case ConstTpl::v_space:
            value = handle->getSpace();
            break;
        case ConstTpl::v_offset:
            value = handle->getPtrOffset();
            break;
        case ConstTpl::v_offset_plus: {
            uintb previous = value.value_real;
            *(&value) = handle->getPtrOffset();
            if (value.type == ConstTpl::real)
                value.value_real += (previous & 0xffff);
            else if ((value.type == ConstTpl::handle) && (value.select == ConstTpl::v_offset)) {
                value.select = ConstTpl::v_offset_plus;
                value.value_real = previous;
            } else
                throw LowlevelError("Cannot truncate macro input in this way");
            break;
        }
        case ConstTpl::v_size:
            value = handle->getSize();
            break;
    }
}

/// Tests whether a handle's resolved size is the zero-size sentinel.
inline bool handleTplSizeIsZero(const HandleTpl* handle) {
    return handle->getSize().isZero();
}

/// \brief A p-code operation in a SLEIGH specification
///
/// Each input and output to the operation is a VarnodeTpl.
class OpTpl {
    VarnodeTpl* output;        ///< The output variable of the operation, or NULL
    OpCode opc;                ///< The code describing the operation
    vector<VarnodeTpl*> input; ///< Inputs to the operation
public:
    OpTpl(void) : output(nullptr) {} ///< Construct an uninitialized OpTpl
    OpTpl(OpCode oc) : output(nullptr) {
        opc = oc;
    } ///< Construct an OpTpl with not inputs or output
    ~OpTpl(void) {
        /// An OpTpl owns its VarnodeTpl
        if (output != (VarnodeTpl*)0)
            delete output;
        vector<VarnodeTpl*>::iterator iter;
        for (iter = input.begin(); iter != input.end(); ++iter)
            delete *iter;
    }
    VarnodeTpl* getOut(void) const {
        return output;
    } ///< Get the output VarnodeTpl (or NULL)
    int4 numInput(void) const {
        return input.size();
    } ///< Return the number of inputs to \b this
    VarnodeTpl* getIn(int4 i) const {
        return input[i];
    } ///< Get the i-th input VarnodeTpl
    OpCode getOpcode(void) const {
        return opc;
    } ///< Get the operation code
    bool isZeroSize(void) const {
        vector<VarnodeTpl*>::const_iterator iter;

        if (output != (VarnodeTpl*)0)
            if (output->isZeroSize())
                return true;
        for (iter = input.begin(); iter != input.end(); ++iter)
            if ((*iter)->isZeroSize())
                return true;
        return false;
    }
    void setOpcode(OpCode o) {
        opc = o;
    } ///< Set the operation code
    void setOutput(VarnodeTpl* vt) {
        output = vt;
    } ///< Set the output VarnodeTpl
    void clearOutput(void) {
        delete output;
        output = (VarnodeTpl*)0;
    } ///< Remove the existing output VarnodeTpl
    void addInput(VarnodeTpl* vt) {
        input.push_back(vt);
    } ///< Add an input VarnodeTpl
    void setInput(VarnodeTpl* vt, int4 slot) {
        input[slot] = vt;
    } ///< Set the VarnodeTpl for a specific input slot
    void removeInput(int4 index) {
        /// \param index is the index of the input to remove
        delete input[index];
        for (int4 i = index; i < input.size() - 1; ++i)
            input[i] = input[i + 1];
        input.pop_back();
    }
    void changeHandleIndex(const vector<int4>& handmap) {
        /// This is used to help reorder sub-constructors.
        /// Each input and output VarnodeTpl is remapped using the given array of \b handle indices
        /// \param handmap is the given array of new \b handle indices
        if (output != (VarnodeTpl*)0)
            output->changeHandleIndex(handmap);
        vector<VarnodeTpl*>::const_iterator iter;

        for (iter = input.begin(); iter != input.end(); ++iter)
            (*iter)->changeHandleIndex(handmap);
    }
    void encode(Encoder& encoder) const {
        /// \param encoder is the output stream
        encoder.openElement(sla::ELEM_OP_TPL);
        encoder.writeOpcode(sla::ATTRIB_CODE, opc);
        if (output == (VarnodeTpl*)0) {
            encoder.openElement(sla::ELEM_NULL);
            encoder.closeElement(sla::ELEM_NULL);
        } else
            output->encode(encoder);
        for (int4 i = 0; i < input.size(); ++i)
            input[i]->encode(encoder);
        encoder.closeElement(sla::ELEM_OP_TPL);
    }
    void decode(Decoder& decoder) {
        /// \param decoder is the input stream
        uint4 el = decoder.openElement(sla::ELEM_OP_TPL);
        opc = decoder.readOpcode(sla::ATTRIB_CODE);
        uint4 subel = decoder.peekElement();
        if (subel == sla::ELEM_NULL) {
            decoder.openElement();
            decoder.closeElement(subel);
            output = (VarnodeTpl*)0;
        } else {
            output = new VarnodeTpl();
            output->decode(decoder);
        }
        while (decoder.peekElement() != 0) {
            VarnodeTpl* vn = new VarnodeTpl();
            input.push_back(vn);
            vn->decode(decoder);
        }
        decoder.closeElement(el);
    }
};

/// \brief P-code semantics for Constructor in a SLEIGH specification
///
/// This encodes a sequence of OpTpl representing the semantic action of the Constructor, and
/// if present, the HandleTpl representing the final \e exported value.
class ConstructTpl {
    friend class SleighCompile;

protected:
    uint4 delayslot;    ///< (Minimum) number of bytes in the delay slot
    uint4 numlabels;    ///< Number of label templates
    vector<OpTpl*> vec; ///< Sequence of operations performed by the Constructor
    HandleTpl* result;  ///< Final \e exported value (or NULL)
    void setOpvec(vector<OpTpl*>& opvec) {
        vec = opvec;
    } ///< Set the sequence of OpTpl
    void setNumLabels(uint4 val) {
        numlabels = val;
    } ///< Set the number of labels
public:
    ConstructTpl(void) {
        delayslot = 0;
        numlabels = 0;
        result = (HandleTpl*)0;
    } ///< Construct an empty ConstructTpl
    ~ConstructTpl(void) {
        /// ConstructTpl owns any OpTpl and HandleTpl
        vector<OpTpl*>::iterator oiter;
        for (oiter = vec.begin(); oiter != vec.end(); ++oiter)
            delete *oiter;
        if (result != (HandleTpl*)0)
            delete result;
    }
    uint4 delaySlot(void) const {
        return delayslot;
    } ///< Return the number of bytes in the delay slot
    uint4 numLabels(void) const {
        return numlabels;
    } ///< Get the number of labels
    const vector<OpTpl*>& getOpvec(void) const {
        return vec;
    } ///< Get the sequence of p-code operations
    HandleTpl* getResult(void) const {
        return result;
    } ///< Get the \e export result
    bool addOp(OpTpl* ot) {
        /// The added OpTpl can be a normal operation, which will be executed directly, or it can be a directive, like a
        /// \b build or \b delayslot, which may ultimately decode to multiple operations.  Additionally, an OpTpl
        /// can represent a \e label, used to resolve internal p-code branches.
        /// \param ot is the OpTpl to add
        /// \return \b true if the operation was successfully added and did not violate a compile time rules
        if (ot->getOpcode() == DELAY_SLOT) {
            if (delayslot != 0)
                return false; // Cannot have multiple delay slots
            delayslot = ot->getIn(0)->getOffset().getReal();
        } else if (ot->getOpcode() == LABELBUILD)
            numlabels += 1; // Count labels
        vec.push_back(ot);
        return true;
    }
    bool addOpList(const vector<OpTpl*>& oplist) {
        /// \param oplist is the list of operations to add
        /// \return \b true if all operations were successfully added
        for (int4 i = 0; i < oplist.size(); ++i)
            if (!addOp(oplist[i]))
                return false;
        return true;
    }
    void setResult(HandleTpl* t) {
        result = t;
    } ///< Set the \e export HandleTpl for \b this
    int4 fillinBuild(vector<int4>& check, AddrSpace* const_space) {
        /// For any sub-constructor that does not already one, a new \b build directive is added to the front
        /// of the operation sequence.
        /// \param check is an array of integers, initialized to 0, used to mark sub-constructors with a directive.
        /// \param const_space is the \e constant address space
        /// \return 0 upon success, 1 if there is a duplicate \b build, 2 if there is a \b build for a non-subtable
        vector<OpTpl*>::iterator iter;
        OpTpl* op;
        VarnodeTpl* indvn;

        for (iter = vec.begin(); iter != vec.end(); ++iter) {
            op = *iter;
            if (op->getOpcode() == BUILD) {
                int4 index = op->getIn(0)->getOffset().getReal();
                if (check[index] != 0)
                    return check[index]; // Duplicate BUILD statement or non-subtable
                check[index] = 1;        // Mark to avoid future duplicate build
            }
        }
        for (int4 i = 0; i < check.size(); ++i) {
            if (check[i] == 0) { // Didn't see a BUILD statement
                op = new OpTpl(BUILD);
                indvn = new VarnodeTpl(ConstTpl(const_space), ConstTpl(ConstTpl::real, i), ConstTpl(ConstTpl::real, 4));
                op->addInput(indvn);
                vec.insert(vec.begin(), op);
            }
        }
        return 0;
    }
    bool buildOnly(void) const {
        /// \return \b true if every operation is a \b build directive
        vector<OpTpl*>::const_iterator iter;
        OpTpl* op;
        for (iter = vec.begin(); iter != vec.end(); ++iter) {
            op = *iter;
            if (op->getOpcode() != BUILD)
                return false;
        }
        return true;
    }
    void changeHandleIndex(const vector<int4>& handmap) {
        /// Each OpTpl operation is remapped using the given array of \b handle indices
        /// \param handmap is an array of new \b handle indices
        vector<OpTpl*>::const_iterator iter;
        OpTpl* op;

        for (iter = vec.begin(); iter != vec.end(); ++iter) {
            op = *iter;
            if (op->getOpcode() == BUILD) {
                int4 index = op->getIn(0)->getOffset().getReal();
                index = handmap[index];
                op->getIn(0)->setOffset(index);
            } else
                op->changeHandleIndex(handmap);
        }
        if (result != (HandleTpl*)0)
            result->changeHandleIndex(handmap);
    }
    void setInput(VarnodeTpl* vn, int4 index, int4 slot) {
        /// For use with optimization routines.
        /// \param vn is the new input
        /// \param index is the position of the operation within the sequence
        /// \param slot is the input slot to replace with the new input
        OpTpl* op = vec[index];
        VarnodeTpl* oldvn = op->getIn(slot);
        op->setInput(vn, slot);
        if (oldvn != (VarnodeTpl*)0)
            delete oldvn;
    }
    void setOutput(VarnodeTpl* vn, int4 index) {
        /// For use with optimization routines.
        /// \param vn is the new output
        /// \param index is the position of the operation within the sequence
        OpTpl* op = vec[index];
        VarnodeTpl* oldvn = op->getOut();
        op->setOutput(vn);
        if (oldvn != (VarnodeTpl*)0)
            delete oldvn;
    }
    void deleteOps(const vector<int4>& indices) {
        /// \param indices is an array of the indices indicating the positions of the OpTpl to be deleted
        for (uint4 i = 0; i < indices.size(); ++i) {
            delete vec[indices[i]];
            vec[indices[i]] = (OpTpl*)0;
        }
        uint4 poscur = 0;
        for (uint4 i = 0; i < vec.size(); ++i) {
            OpTpl* op = vec[i];
            if (op != (OpTpl*)0) {
                vec[poscur] = op;
                poscur += 1;
            }
        }
        while (vec.size() > poscur)
            vec.pop_back();
    }
    void encode(Encoder& encoder, int4 sectionid) const {
        /// \param encoder is the output stream
        /// \param sectionid is the id of the specific Constructor section to associate with \b this sequence
        encoder.openElement(sla::ELEM_CONSTRUCT_TPL);
        if (sectionid >= 0)
            encoder.writeSignedInteger(sla::ATTRIB_SECTION, sectionid);
        if (delayslot != 0)
            encoder.writeSignedInteger(sla::ATTRIB_DELAY, delayslot);
        if (numlabels != 0)
            encoder.writeSignedInteger(sla::ATTRIB_LABELS, numlabels);
        if (result != (HandleTpl*)0)
            result->encode(encoder);
        else {
            encoder.openElement(sla::ELEM_NULL);
            encoder.closeElement(sla::ELEM_NULL);
        }
        for (int4 i = 0; i < vec.size(); ++i)
            vec[i]->encode(encoder);
        encoder.closeElement(sla::ELEM_CONSTRUCT_TPL);
    }
    int4 decode(Decoder& decoder) {
        /// \param decoder is the stream to decode from
        /// \return the Constructor section id associated with the sequence
        uint4 el = decoder.openElement(sla::ELEM_CONSTRUCT_TPL);
        int4 sectionid = -1;
        uint4 attrib = decoder.getNextAttributeId();
        while (attrib != 0) {
            if (attrib == sla::ATTRIB_DELAY) {
                delayslot = decoder.readSignedInteger();
            } else if (attrib == sla::ATTRIB_LABELS) {
                numlabels = decoder.readSignedInteger();
            } else if (attrib == sla::ATTRIB_SECTION) {
                sectionid = decoder.readSignedInteger();
            }
            attrib = decoder.getNextAttributeId();
        }
        uint4 subel = decoder.peekElement();
        if (subel == sla::ELEM_NULL) {
            decoder.openElement();
            decoder.closeElement(subel);
            result = (HandleTpl*)0;
        } else {
            result = new HandleTpl();
            result->decode(decoder);
        }
        while (decoder.peekElement() != 0) {
            OpTpl* op = new OpTpl();
            vec.push_back(op);
            op->decode(decoder);
        }
        decoder.closeElement(el);
        return sectionid;
    }
};

class PcodeEmit; // Forward declaration for emitter

/// \brief An abstract, SLEIGH specific, p-code generator
///
/// This is a base class for output, filtering, or otherwise processing sequences of p-code for a single
/// instruction decoded by the SLEIGH engine.  The ConstructTpl of the root constructor for the instruction is fed to
/// the build() method.  Each normal p-code operation then makes it to the dump() method in sequence for processing.
/// Hook points are provided to recurse into different instructions/constructors via \b build, \b delayslot, and
/// \b crossbuild directives.
class PcodeBuilder {
    uint4 labelbase;  ///< Starting label index for this builder
    uint4 labelcount; ///< Current number of defined labels
protected:
    ParserWalker* walker; ///< Current instruction context

    /// \brief Output/build the given p-code operation
    ///
    /// This is the main hook point for different p-code generation strategies.  It is called once, in sequence, for
    /// each p-code operation generated for a specific instruction.  Directives and labels have had the opportunity to
    /// be expanded or filtered by other methods.
    /// \param op is the p-code operation to process
    virtual void dump(OpTpl* op) = 0;

public:
    PcodeBuilder(uint4 lbcnt) {
        labelbase = labelcount = lbcnt;
    } ///< Construct with a starting label index
    virtual ~PcodeBuilder(void) {} ///< Destructor

    uint4 getLabelBase(void) const {
        return labelbase;
    } ///< Get the starting label index for \b this builder
    ParserWalker* getCurrentWalker() const {
        return walker;
    } ///< Get the current instruction context
    void build(ConstructTpl* construct, int4 secnum) {
        // Process all the semantic operations in the given ConstructTpl, handling directives and labels.
        // \param construct is the given ConstructTpl sequence
        // \param secnum is the section number associated with the sequence
        if (construct == (ConstructTpl*)0)
            throw UnimplError("", 0); // P-code is not implemented for this constructor

        uint4 oldbase = labelbase;            // Recursively store old labelbase
        labelbase = labelcount;               // Set the newbase
        labelcount += construct->numLabels(); // Add labels from this template

        vector<OpTpl*>::const_iterator iter;
        OpTpl* op;
        const vector<OpTpl*>& ops(construct->getOpvec());

        for (iter = ops.begin(); iter != ops.end(); ++iter) {
            op = *iter;
            switch (op->getOpcode()) {
                case BUILD:
                    appendBuild(op, secnum);
                    break;
                case DELAY_SLOT:
                    delaySlot(op);
                    break;
                case LABELBUILD:
                    setLabel(op);
                    break;
                case CROSSBUILD:
                    appendCrossBuild(op, secnum);
                    break;
                default:
                    dump(op);
                    break;
            }
        }
        labelbase = oldbase; // Restore old labelbase
    }

    /// \brief Execute or filter a \b build directive in sequence
    ///
    /// Any recursion through \b build directives into ConstructTpl for sub-constructors happens here.
    /// \param bld is the \b build directive, which encodes the particular sub-construction
    /// \param secnum is the current section number of the constructor
    virtual void appendBuild(OpTpl* bld, int4 secnum) = 0;

    /// \brief Execute or filter a \b delayslot directive in sequence
    ///
    /// Any recursion through \b delayslot directives into new instructions happens here.
    /// \param op is the \b delayslot directive, which encodes the number of bytes in the particular delay slot.
    virtual void delaySlot(OpTpl* op) = 0;

    /// \brief Process or filter a label in sequence
    ///
    /// The builder has the opportunity to record exactly where in sequence the given label occurs.
    /// \param op is the OpTpl encoding the label index
    virtual void setLabel(OpTpl* op) = 0;

    /// \brief Process or filter a \b crossbuild directive in sequence
    ///
    /// Any recursion through \b crossbuild directives into new instructions and sections happens here.
    /// \param bld is the \b crossbuild directive
    /// \param secnum is the current section number being processed
    virtual void appendCrossBuild(OpTpl* bld, int4 secnum) = 0;
};

} // End namespace ghidra
