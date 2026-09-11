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
/// \file context.cppm
/// \brief Objects for describing the context around the parsing of an instruction by the SLEIGH engine

export module sleigh_runtime:context;
import std;
export import :globalcontext;
export import :opcodes;
export import :translate;

export namespace ghidra {

/// \brief A multiple-byte sized chunk of pattern in the instruction byte stream
class Token {
    string name;    ///< Name of the token
    int4 size;      ///< Number of bytes in token
    int4 index;     ///< Index of \b this token, for resolving offsets
    bool bigendian; ///< Set to \b true if encodings within \b this token are big endian
public:
    Token(const string& nm, int4 sz, bool be, int4 ind) : name(nm) {
        size = sz;
        bigendian = be;
        index = ind;
    } ///< Constructor
    int4 getSize(void) const {
        return size;
    } ///< Get the size in bytes
    bool isBigEndian(void) const {
        return bigendian;
    } ///< Return \b true if encodings within \b this are big endian
    int4 getIndex(void) const {
        return index;
    } ///< Get the index associated with \b this token
    const string& getName(void) const {
        return name;
    } ///< Get the name of the token
};

/// \brief A resolved version of (or pointer to) a SLEIGH defined Varnode
///
/// For a static Varnode, this is the triple (address space, offset, size) for the Varnode.
/// For a dynamic Varnode, this also encodes the pointer Varnode containing the dynamic offset
/// and a temporary storage location for the dereferenced value.
struct FixedHandle {
    AddrSpace* space;        ///< The address space of the Varnode
    uint4 size;              ///< Number of bytes in the Varnode
    AddrSpace* offset_space; ///< Null \e or the space where the dynamic offset is stored
    uintb offset_offset;     ///< The offset for the static Varnode \e or the offset for the pointer
    uint4 offset_size;       ///< Size of pointer
    AddrSpace* temp_space;   ///< Address space for temporary location for value
    uintb temp_offset;       ///< Offset of the temporary location
};

class Constructor;

/// \brief A node in a tree of subconstructors
///
/// This knows its position in the tree (parent node, child nodes) and the underlying SLEIGH constructor that was
/// matched. Child nodes correspond to the operands for the specific constructor.
struct ConstructState {
    Constructor* ct;          ///< The matched Constructor
    FixedHandle hand;         ///< Resolved Varnode associated with the Constructor
    ConstructState** resolve; ///< An array of pointers to child nodes
    ConstructState* parent;   ///< Pointer to parent node
    int4 length;              ///< Length of this instantiation of the constructor
    uint4 offset;             ///< Absolute offset (from start of instruction)

    ConstructState(void) {
        ct = (Constructor*)0;
        resolve = (ConstructState**)0;
        parent = (ConstructState*)0;
        length = 0;
        offset = 0;
    }

    /// The array holding pointers to child nodes is preallocated.
    /// \param numOperands is maximum number of children this node can have
    ConstructState(int4 numOperands) {
        ct = (Constructor*)0;
        parent = (ConstructState*)0;
        length = 0;
        offset = 0;
        resolve = new ConstructState*[numOperands];
        for (int4 i = 0; i < numOperands; ++i)
            resolve[i] = (ConstructState*)0;
    }

    ~ConstructState(void) {
        if (resolve != (ConstructState**)0)
            delete[] resolve;
    }
};

class TripleSymbol;

/// \brief Command for globally setting a formal SLEIGH context value
struct ContextSet {
    TripleSymbol* sym;     ///< Symbol resolving to address where setting takes effect
    ConstructState* point; ///< Point at which context set was made
    int4 num;              ///< Index of the specific context word affected
    uintm mask;            ///< Bits within word affected
    uintm value;           ///< New setting for bits
    bool flow;             ///< Does the new context flow from its set point
};

class ParserWalker; // Forward declaration
class ParserWalkerChange;
class Translate;
class ParserContext;

/// Performs parser-state reset after ParserWalkerChange is complete.
void parserContextDeallocateState(ParserContext& context, ParserWalkerChange& walker);
/// Allocates and links a parser operand after ParserWalkerChange is complete.
void parserContextAllocateOperand(ParserContext& context, int4 index, ParserWalkerChange& walker);
/// Applies symbol-dependent context commits without importing slghsymbol here.
void parserContextApplyCommits(ParserContext& context);
/// Performs symbol-dependent walker setup without importing slghsymbol here.
void parserWalkerSetOutOfBandState(ParserWalker& walker, Constructor* constructor, int4 index,
                                   ConstructState* temporaryState, const ParserWalker& otherWalker);

/// \brief Context maintained while parsing a single instruction
///
/// This contains:
///   - the bytes encoding the instruction
///   - the tree structure of the SLEIGH Constructors encountered while parsing the instruction
///   - any formal named SLEIGH context values referenced by the instruction
class ParserContext {
    friend class ParserWalker;
    friend class ParserWalkerChange;
    friend void parserContextDeallocateState(ParserContext&, ParserWalkerChange&);
    friend void parserContextAllocateOperand(ParserContext&, int4, ParserWalkerChange&);
    friend void parserContextApplyCommits(ParserContext&);

public:
    static constexpr int4 MAX_DEPTH = 32;           ///< Maximum subconstructor depth in a single instruction
    static constexpr int4 MAX_OPERAND = 20;         ///< Maximum operands for a single constructor
    static constexpr int4 MAX_INSTRUCTION_LEN = 16; ///< Maximum number of bytes in a single instruction
    static constexpr int4 INITIAL_STATE_NUM = 64;   ///< Recommended number of initial states
    static constexpr int4 STATE_GROWTH = 64;        ///< Number of states to add for each expansion

    /// \brief Possible states of the ParserContext
    enum parse_state {
        uninitialized = 0, ///< Instruction has not been parsed at all
        disassembly = 1,   ///< Instruction is parsed in preparation for disassembly
        pcode = 2          ///< Instruction is parsed in preparation for generating p-code
    };

private:
    Translate* translate;             ///< The parent instruction parser
    parse_state parsestate;           ///< Overall state of the parse
    AddrSpace* const_space;           ///< Address space for constants
    uint1 buf[MAX_INSTRUCTION_LEN];   ///< Buffer of bytes in the instruction stream
    uintm* context;                   ///< Pointer to local context
    int4 contextsize;                 ///< Number of entries in local context array
    ContextCache* contcache;          ///< Interface for getting/setting context
    vector<ContextSet> contextcommit; ///< Changes to SLEIGH context slated by this instruction
    Address addr;                     ///< Address of start of instruction
    Address naddr;                    ///< Address of next instruction
    mutable Address n2addr;           ///< Address of instruction after the next
    Address calladdr;                 ///< For injections, this is the address of the call being overridden
    vector<ConstructState*> state;    ///< Available nodes for the constructor tree
    ConstructState* base_state;       ///< Root node of the constructor tree
    int4 alloc;                       ///< Number of unallocated ConstructState nodes remaining
    int4 delayslot;                   ///< delayslot depth

public:
    /// \param ccache is the cache to use for formal context changes
    /// \param trans is the parent parser
    ParserContext(ContextCache* ccache, Translate* trans) {
        parsestate = uninitialized;
        contcache = ccache;
        translate = trans;
        if (ccache != (ContextCache*)0) {
            contextsize = ccache->getDatabase()->getContextSize();
            context = new uintm[contextsize];
        } else {
            contextsize = 0;
            context = (uintm*)0;
        }
    }

    ~ParserContext(void) {
        if (context != (uintm*)0)
            delete[] context;
        for (int4 i = 0; i < state.size(); ++i)
            delete state[i];
    }

    uint1* getBuffer(void) {
        return buf;
    } ///< Get bytes in the stream at the point this instruction is encoded

    /// \param spc is the address space used for constants
    /// \param maxstate is the number of nodes to allocate (initially)
    void initialize(AddrSpace* spc, int4 maxstate = INITIAL_STATE_NUM) {
        const_space = spc;
        state.resize(maxstate);
        for (int4 i = 0; i < maxstate; ++i)
            state[i] = new ConstructState(MAX_OPERAND);
        base_state = state[maxstate - 1];
    }

    parse_state getParserState(void) const {
        return parsestate;
    } ///< Get the overall state of the parse
    void setParserState(parse_state st) {
        parsestate = st;
    } ///< Update the overall parse state

    /// The tree is reset to a single root node and the walker is prepared for a new parse
    /// \param walker is the walker to initialize for a traversal
    inline void deallocateState(ParserWalkerChange& walker) {
        parserContextDeallocateState(*this, walker);
    }

    /// The next available node is linked to the current active node in the walker at the given operand index.
    /// The child node becomes the new active node for the walker. The underlying constructor is not yet assigned.
    /// \param i is the operand index of the new child
    /// \param walker is the walker for the parse
    inline void allocateOperand(int4 i, ParserWalkerChange& walker) {
        parserContextAllocateOperand(*this, i, walker);
    }

    void setAddr(const Address& ad) {
        addr = ad;
        n2addr = Address();
    } ///< Set the starting address of the instruction
    void setNaddr(const Address& ad) {
        naddr = ad;
    } ///< Set the ending address of the instruction
    void setCalladdr(const Address& ad) {
        calladdr = ad;
    } ///< Set the address of the call being overridden

    /// \param sym is a symbol that resolves to the address where the setting takes effect
    /// \param num is the index of the context word being affected
    /// \param mask indicates the bits within the context word that are affected
    /// \param flow is \b true if the context change \e flows forward from the point where it is set
    /// \param point is the parse point where the change was made
    void addCommit(TripleSymbol* sym, int4 num, uintm mask, bool flow, ConstructState* point) {
        contextcommit.emplace_back();
        ContextSet& set(contextcommit.back());

        set.sym = sym;
        set.point = point; // This is the current state
        set.num = num;
        set.mask = mask;
        set.value = context[num] & mask;
        set.flow = flow;
    }

    void clearCommits(void) {
        contextcommit.clear();
    } ///< Clear all context commits

    void applyCommits(void) {
        parserContextApplyCommits(*this);
    }

    const Address& getAddr(void) const {
        return addr;
    } ///< Get the starting address of the current instruction
    const Address& getNaddr(void) const {
        return naddr;
    } ///< Get the address of the next instruction

    const Address& getN2addr(void) const {
        if (n2addr.isInvalid()) {
            if (translate == (Translate*)0 || parsestate == uninitialized)
                throw LowlevelError("inst_next2 not available in this context");
            int4 length = translate->instructionLength(naddr);
            n2addr = naddr + length;
        }
        return n2addr;
    }

    const Address& getDestAddr(void) const {
        return calladdr;
    } ///< Get the destination address (inst_dest) for the overriden call
    const Address& getRefAddr(void) const {
        return calladdr;
    } ///< Get the reference address (inst_ref) for the p-code snippet
    AddrSpace* getCurSpace(void) const {
        return addr.getSpace();
    } ///< Get the address space of the current instruction
    AddrSpace* getConstSpace(void) const {
        return const_space;
    } ///< Get the address space for constants

    /// Get bytes from the instruction stream into a packed value assuming a big endian encoding.
    /// \param bytestart is the number of bytes to skip
    /// \param size is the number of bytes to pack
    /// \param off is the number of bytes in the instruction already read
    /// \return the packed bytes from the instruction
    uintm getInstructionBytes(int4 bytestart, int4 size, uint4 off) const {
        off += bytestart;
        if (off >= MAX_INSTRUCTION_LEN)
            throw BadDataError("Instruction is using more than " + to_string(MAX_INSTRUCTION_LEN) + " bytes");
        const uint1* ptr = buf + off;
        uintm res = 0;
        for (int4 i = 0; i < size; ++i) {
            res <<= 8;
            res |= ptr[i];
        }
        return res;
    }

    /// \param bytestart is the offset of the first byte to grab
    /// \param size is the number of bytes to grab
    /// \return the context bytes in a packed value
    uintm getContextBytes(int4 bytestart, int4 size) const {
        int4 intstart = bytestart / sizeof(uintm);
        uintm res = context[intstart];
        int4 byteOffset = bytestart % sizeof(uintm);
        int4 unusedBytes = sizeof(uintm) - size;
        res <<= byteOffset * 8;
        res >>= unusedBytes * 8;
        int4 remaining = size - sizeof(uintm) + byteOffset;
        if ((remaining > 0) && (++intstart < contextsize)) { // If we extend beyond boundary of a single uintm
            uintm res2 = context[intstart];
            unusedBytes = sizeof(uintm) - remaining;
            res2 >>= unusedBytes * 8;
            res |= res2;
        }
        return res;
    }

    /// Get bits from the instruction stream assuming big endian encoding.
    /// \param startbit is the offset of the first bit (within the instruction stream)
    /// \param size is the number of bits to grab
    /// \param off is the number of bytes in the instruction already read
    /// \return the requested range of bits (in the least significant positions and padded out with zero bits)
    uintm getInstructionBits(int4 startbit, int4 size, uint4 off) const {
        off += (startbit / 8);
        if (off >= MAX_INSTRUCTION_LEN)
            throw BadDataError("Instruction is using more than " + to_string(MAX_INSTRUCTION_LEN) + " bytes");
        const uint1* ptr = buf + off;
        startbit = startbit % 8;
        int4 bytesize = (startbit + size - 1) / 8 + 1;
        uintm res = 0;
        for (int4 i = 0; i < bytesize; ++i) {
            res <<= 8;
            res |= ptr[i];
        }
        res <<= 8 * (sizeof(uintm) - bytesize) + startbit; // Move starting bit to highest position
        res >>= 8 * sizeof(uintm) - size;                  // Shift to bottom of intm
        return res;
    }

    /// \param startbit is the offset of the first bit
    /// \param size is the number of bits to return
    /// \return the requested range of bits (in the least significant positions and padded out with zero bits)
    uintm getContextBits(int4 startbit, int4 size) const {
        int4 intstart = startbit / (8 * sizeof(uintm));
        uintm res = context[intstart]; // Get intm containing highest bit
        int4 bitOffset = startbit % (8 * sizeof(uintm));
        int4 unusedBits = 8 * sizeof(uintm) - size;
        res <<= bitOffset; // Shift startbit to highest position
        res >>= unusedBits;
        int4 remaining = size - 8 * sizeof(uintm) + bitOffset;
        if ((remaining > 0) && (++intstart < contextsize)) {
            uintm res2 = context[intstart];
            unusedBits = 8 * sizeof(uintm) - remaining;
            res2 >>= unusedBits;
            res |= res2;
        }
        return res;
    }

    void setContextWord(int4 i, uintm val, uintm mask) {
        context[i] = (context[i] & (~mask)) | (mask & val);
    } ///< Modify a context word, using given mask and value
    void loadContext(void) {
        contcache->getContext(addr, context);
    } ///< Pull context words associated with the starting address into the local array
    int4 getLength(void) const {
        return base_state->length;
    } ///< Get the length of the current instruction
    void setDelaySlot(int4 val) {
        delayslot = val;
    } ///< Set (the number of instruction bytes) in the delay slot
    int4 getDelaySlot(void) const {
        return delayslot;
    } ///< Get the number of instruction bytes in the delay slot

    /// This can be called in the middle of a parse to accommodate larger constructor trees.
    /// \param amount is the number of additional nodes to add
    void expandState(int4 amount) {
        state.insert(state.begin(), amount, (ConstructState*)0);
        for (int4 i = 0; i < amount; ++i)
            state[i] = new ConstructState(MAX_OPERAND);

        alloc += amount;
    }
};

/// \brief A class for walking the constructor tree (ParserContext)
class ParserWalker {
    friend void parserWalkerSetOutOfBandState(ParserWalker&, Constructor*, int4, ConstructState*, const ParserWalker&);

private:
    const ParserContext* const_context; ///< Context for the main instruction parse
    const ParserContext*
        cross_context; ///< Context for an additional instruction parse needed to resolve a \e crossbuild
protected:
    ConstructState* point;                     ///< The current node being visited
    int4 depth;                                ///< Depth of the current node
    int4 breadcrumb[ParserContext::MAX_DEPTH]; ///< Path of operands from root
public:
    ParserWalker(const ParserContext* c) {
        const_context = c;
        cross_context = (const ParserContext*)0;
    } ///< Constructor
    ParserWalker(const ParserContext* c, const ParserContext* cross) {
        const_context = c;
        cross_context = cross;
    } ///< Constructor for crossbuilds
    const ParserContext* getParserContext(void) const {
        return const_context;
    } ///< Get the current context
    void baseState(void) {
        point = const_context->base_state;
        depth = 0;
        breadcrumb[0] = 0;
    } ///< Initialize for a new walk

    /// \brief Initialize \b this from another walker assuming a given constructor and operand is the current position
    /// in the walk
    ///
    /// The constructor tree state is simulated using only a single provided node.
    /// This allows TokenField to behave as if it were just parsed so its getValue() will return the correct value.
    /// \param ct is the given constructor
    /// \param index is the index of the operand
    /// \param tempstate is provided storage used to simulate the mid-walk tree node
    /// \param otherwalker is the walker with the complete parse state
    void setOutOfBandState(Constructor* ct, int4 index, ConstructState* tempstate, const ParserWalker& otherwalker) {
        parserWalkerSetOutOfBandState(*this, ct, index, tempstate, otherwalker);
    }

    bool isState(void) const {
        return (point != (ConstructState*)0);
    } ///< Return \b true if there are more nodes to traverse

    /// \param i is the index of child/operand
    inline void pushOperand(int4 i) {
        if (depth > ParserContext::MAX_DEPTH - 2)
            throw LowlevelError("SLEIGH exceeded maximum parse depth");
        breadcrumb[depth++] = i + 1;
        point = point->resolve[i];
        breadcrumb[depth] = 0;
    }

    void popOperand(void) {
        point = point->parent;
        depth -= 1;
    } ///< Make the parent constructor the current node

    uint4 getOffset(int4 i) const {
        if (i < 0)
            return point->offset;
        ConstructState* op = point->resolve[i];
        return op->offset + op->length;
    } ///< Get the byte offset of the indicated operand within the instruction stream
    Constructor* getConstructor(void) const {
        return point->ct;
    } ///< Get the current constructor
    int4 getOperand(void) const {
        return breadcrumb[depth];
    } ///< Get the operand index of the next constructor in the walk
    FixedHandle& getParentHandle(void) {
        return point->hand;
    } ///< Get the resolved value associated with the current constructor
    const FixedHandle& getFixedHandle(int4 i) const {
        return point->resolve[i]->hand;
    } ///< Get the resolved value associated with the indicated child operand
    AddrSpace* getCurSpace(void) const {
        return const_context->getCurSpace();
    } ///< Get the address space associated with the instruction stream
    AddrSpace* getConstSpace(void) const {
        return const_context->getConstSpace();
    } ///< Get the constant address space

    /// \brief Get the starting address of the instruction
    const Address& getAddr(void) const {
        if (cross_context != (const ParserContext*)0) {
            return cross_context->getAddr();
        }
        return const_context->getAddr();
    }

    /// \brief Get the address of the next instruction
    const Address& getNaddr(void) const {
        if (cross_context != (const ParserContext*)0) {
            return cross_context->getNaddr();
        }
        return const_context->getNaddr();
    }

    /// \brief Get the address of the instruction after next
    const Address& getN2addr(void) const {
        if (cross_context != (const ParserContext*)0) {
            return cross_context->getN2addr();
        }
        return const_context->getN2addr();
    }

    /// \brief Get the reference address (inst_ref) for the p-code snippet
    const Address& getRefAddr(void) const {
        if (cross_context != (const ParserContext*)0) {
            return cross_context->getRefAddr();
        }
        return const_context->getRefAddr();
    }

    /// \brief Get the destination address (inst_dest) for the overridden call
    const Address& getDestAddr(void) const {
        if (cross_context != (const ParserContext*)0) {
            return cross_context->getDestAddr();
        }
        return const_context->getDestAddr();
    }

    int4 getLength(void) const {
        return const_context->getLength();
    } ///< Get the length of the instruction in bytes

    /// \brief Get packed instruction bytes associated with the current constructor
    ///
    /// \param byteoff is an offset from the starting point associated with the constructor
    /// \param numbytes is the number of bytes to pack
    /// \return the packed instruction bytes in big endian encoding
    uintm getInstructionBytes(int4 byteoff, int4 numbytes) const {
        return const_context->getInstructionBytes(byteoff, numbytes, point->offset);
    }

    /// \brief Get packed context bytes from the local context
    ///
    /// \param byteoff is the offset of the first byte to grab
    /// \param numbytes is the number of bytes to grab
    /// \return the context bytes in a packed value
    uintm getContextBytes(int4 byteoff, int4 numbytes) const {
        return const_context->getContextBytes(byteoff, numbytes);
    }

    /// \brief Get bits from the instruction stream associated with the current constructor
    ///
    /// \param startbit is the offset of the first bit (relative to the starting point associated with the constructor)
    /// \param size is the number of bits to grab
    /// \return the requested range of bits (in the least significant positions and padded out with zero bits)
    uintm getInstructionBits(int4 startbit, int4 size) const {
        return const_context->getInstructionBits(startbit, size, point->offset);
    }

    /// \brief Get a range of bits from the local context
    ///
    /// \param startbit is the offset of the first bit
    /// \param size is the number of bits to return
    /// \return the requested range of bits (in the least significant positions and padded out with zero bits)
    uintm getContextBits(int4 startbit, int4 size) const {
        return const_context->getContextBits(startbit, size);
    }
};

/// \brief A walker extension that allows for on the fly modifications to the constructor tree
///
/// This is used to build the constructor tree as the instruction is parsed (Sleigh::resolve)
class ParserWalkerChange : public ParserWalker {
    friend class ParserContext;
    friend void parserContextDeallocateState(ParserContext&, ParserWalkerChange&);
    friend void parserContextAllocateOperand(ParserContext&, int4, ParserWalkerChange&);
    ParserContext* context; ///< The (currently active) context
public:
    ParserWalkerChange(ParserContext* c) : ParserWalker(c) {
        context = c;
    } ///< Constructor
    ParserContext* getParserContext(void) {
        return context;
    } ///< Get the currently active context
    ConstructState* getPoint(void) {
        return point;
    } ///< Get the current
    void setOffset(uint4 off) {
        point->offset = off;
    } ///< Get the current node in the constructor tree
    void setConstructor(Constructor* c) {
        point->ct = c;
    } ///< Set the underlying Constructor for the current node
    void setCurrentLength(int4 len) {
        point->length = len;
    } ///< Set the length associated with the current constructor

    /// This assumes all the current nodes operands have been parsed into the tree.
    /// \param length is the minimum length of the current constructor
    /// \param numopers is the number of operands
    void calcCurrentLength(int4 length, int4 numopers) {
        length += point->offset; // Convert relative length to absolute length
        for (int4 i = 0; i < numopers; ++i) {
            ConstructState* subpoint = point->resolve[i];
            int4 sublength = subpoint->length + subpoint->offset;
            // Since subpoint->offset is an absolutee (relative to beginning of instruction)
            if (sublength > length) // sublength is absolute and must be compared to absolute length
                length = sublength;
        }
        point->length = length - point->offset; // Convert back to relative length
    }
};

/// Resets the parser state after both participating walker classes are complete.
inline void parserContextDeallocateState(ParserContext& context, ParserWalkerChange& walker) {
    context.alloc = context.state.size() - 2;
    walker.context = &context;
    walker.baseState();
}

/// Allocates and links one operand in the parser tree.
inline void parserContextAllocateOperand(ParserContext& context, int4 index, ParserWalkerChange& walker) {
    if (index >= ParserContext::MAX_OPERAND)
        throw LowlevelError("SLEIGH parser out of state space");
    if (context.alloc < 0)
        context.expandState(ParserContext::STATE_GROWTH);
    ConstructState* operandState = context.state[context.alloc--];
    operandState->parent = walker.point;
    operandState->ct = (Constructor*)0;
    walker.point->resolve[index] = operandState;
    if (walker.depth > ParserContext::MAX_DEPTH - 2)
        throw LowlevelError("SLEIGH exceeded maximum parse depth");
    walker.breadcrumb[walker.depth++] += 1;
    walker.point = operandState;
    walker.breadcrumb[walker.depth] = 0;
}

/// \brief Exception thrown by the SLEIGH engine
struct SleighError : public LowlevelError {
    SleighError(const string& s) : LowlevelError(s) {} ///< Constructor
};

} // End namespace ghidra
