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
/// \file sleigh.cppm
/// \brief Classes and utilities for the main SLEIGH engine

export module sleigh_runtime:sleigh;
import std;
export import :sleighbase;
export import :loadimage;

export namespace ghidra {

class LoadImage;

/// \brief Class for describing a relative p-code branch destination
///
/// An intra-instruction p-code branch takes a \e relative operand.
/// The actual value produced during p-code generation is calculated at
/// the last second using \b this. It stores the index of the BRANCH
/// instruction and a reference to its destination operand. This initially
/// holds a reference to a destination \e label symbol, but is later updated
/// with the final relative value.
struct RelativeRecord {
    VarnodeData* dataptr; ///< Varnode indicating relative offset
    uintb calling_index;  ///< Index of instruction containing relative offset
};

/// \brief Data for building one p-code instruction
///
/// Raw data used by the emitter to produce a single PcodeOp
struct PcodeData {
    VarnodeData* outvar; ///< Output Varnode data (or null)
    VarnodeData* invar;  ///< Array of input Varnode data
    OpCode opc;          ///< The op code
    int4 isize;          ///< Number of input Varnodes
};

/// \brief Class for caching a chunk of p-code, prior to emitting
///
/// The engine accumulates PcodeData and VarnodeData objects for
/// a single instruction.  Once the full instruction is constructed,
/// the objects are passed to the emitter (PcodeEmit) via the emit() method.
/// The class acts as a pool of memory for PcodeData and VarnodeData objects
/// that can be reused repeatedly to emit multiple instructions.
class PcodeCacher {
    std::unique_ptr<VarnodeData[]> poolstorage; ///< Owned VarnodeData pool
    VarnodeData* poolstart;                     ///< Borrowed start view of the pool
    VarnodeData* curpool;                       ///< First unused VarnodeData
    VarnodeData* endpool;                       ///< End of the pool of VarnodeData objects
    deque<PcodeData> issued;                    ///< P-code ops issued for the current instruction
    list<RelativeRecord> label_refs;            ///< References to labels
    vector<uintb> labels;                       ///< Locations of labels
    VarnodeData* expandPool(uint4 size) {
        uint4 curmax = endpool - poolstart;
        uint4 cursize = curpool - poolstart;
        if (cursize + size <= curmax)
            return curpool; // No expansion necessary
        uint4 increase = (cursize + size) - curmax;
        if (increase < 100) // Increase by at least 100
            increase = 100;

        uint4 newsize = curmax + increase;

        auto newstorage = std::make_unique<VarnodeData[]>(newsize);
        VarnodeData* newpool = newstorage.get();
        for (uint4 i = 0; i < cursize; ++i)
            newpool[i] = poolstart[i]; // Copy old data
        // Update references to the old pool
        for (deque<PcodeData>::iterator diter = issued.begin(); diter != issued.end(); ++diter) {
            VarnodeData* outvar = (*diter).outvar;
            if (outvar != (VarnodeData*)0) {
                outvar = newpool + (outvar - poolstart);
                (*diter).outvar = outvar;
            }
            VarnodeData* invar = (*diter).invar;
            if (invar != (VarnodeData*)0) {
                invar = newpool + (invar - poolstart);
                (*diter).invar = invar;
            }
        }
        list<RelativeRecord>::iterator iter;
        for (iter = label_refs.begin(); iter != label_refs.end(); ++iter) {
            VarnodeData* ref = (*iter).dataptr;
            (*iter).dataptr = newpool + (ref - poolstart);
        }

        poolstorage = std::move(newstorage);
        poolstart = poolstorage.get();
        curpool = newpool + (cursize + size);
        endpool = newpool + newsize;
        return newpool + cursize;
    } ///< Expand the memory pool
public:
    PcodeCacher(void) {
        // We aim to allocate this array only once
        const uint4 maxsize = 600;
        poolstorage = std::make_unique<VarnodeData[]>(maxsize);
        poolstart = poolstorage.get();
        endpool = poolstart + maxsize;
        curpool = poolstart;
    } ///< Constructor
    ~PcodeCacher(void) = default;

    /// \brief Allocate data objects for a new set of Varnodes
    ///
    /// \param size is the number of objects to allocate
    /// \return a pointer to the array of available VarnodeData objects
    VarnodeData* allocateVarnodes(uint4 size) {
        VarnodeData* newptr = curpool + size;
        if (newptr <= endpool) {
            VarnodeData* res = curpool;
            curpool = newptr;
            return res;
        }
        return expandPool(size);
    }

    /// \brief Allocate a data object for a new p-code operation
    ///
    /// \return the new PcodeData object
    PcodeData* allocateInstruction(void) {
        issued.emplace_back();
        PcodeData* res = &issued.back();
        res->outvar = (VarnodeData*)0;
        res->invar = (VarnodeData*)0;
        return res;
    }
    void addLabelRef(VarnodeData* ptr) {
        label_refs.emplace_back();
        label_refs.back().dataptr = ptr;
        label_refs.back().calling_index = issued.size();
    } ///< Denote a Varnode holding a \e relative \e branch offset
    void addLabel(uint4 id) {
        while (labels.size() <= id)
            labels.push_back(0xbadbeef);
        labels[id] = issued.size();
    } ///< Attach a label to the \e next p-code instruction
    void clear(void) {
        curpool = poolstart;
        issued.clear();
        label_refs.clear();
        labels.clear();
    } ///< Reset the cache so that all objects are unallocated
    void resolveRelatives(void) {
        list<RelativeRecord>::const_iterator iter;
        for (iter = label_refs.begin(); iter != label_refs.end(); ++iter) {
            VarnodeData* ptr = (*iter).dataptr;
            uint4 id = ptr->offset;
            if ((id >= labels.size()) || (labels[id] == 0xbadbeef))
                throw LowlevelError("Reference to non-existant sleigh label");
            // Calculate the relative index given the two absolute indices
            uintb res = labels[id] - (*iter).calling_index;
            res &= calc_mask(ptr->size);
            ptr->offset = res;
        }
    } ///< Rewrite branch target Varnodes as \e relative offsets
    void emit(const Address& addr, PcodeEmit* emt) const {
        deque<PcodeData>::const_iterator iter;

        for (iter = issued.begin(); iter != issued.end(); ++iter)
            emt->dump(addr, (*iter).opc, (*iter).outvar, (*iter).invar, (*iter).isize);
    } ///< Pass the cached p-code data to the emitter
};

/// \brief A container for disassembly context used by the SLEIGH engine
///
/// This acts as a factor for the ParserContext objects which are used to disassemble
/// a single instruction.  These all share a ContextCache which is a front end for
/// accessing the ContextDatabase and resolving context variables from the SLEIGH spec.
/// ParserContext objects are stored in a hash-table keyed by the address of the instruction.
class DisassemblyCache {
    Translate* translate;       ///< The Translate object that owns this cache
    ContextCache* contextcache; ///< Cached values from the ContextDatabase
    AddrSpace* constspace;      ///< The constant address space
    int4 minimumreuse;          ///< Can call getParserContext this many times, before a ParserContext is reused
    uint4 mask;                 ///< Size of the hashtable in form 2^n-1
    vector<std::unique_ptr<ParserContext>> contexts; ///< Stable ownership of cached parser contexts
    int4 nextfree;                                   ///< Current end/beginning of circular list
    vector<ParserContext*> hashtable;                ///< Non-owning lookup table keyed by address
    void initialize(int4 min, int4 hashsize) {
        minimumreuse = min;
        mask = hashsize - 1;
        uintb masktest = coveringmask((uintb)mask);
        if (masktest != (uintb)mask) // -hashsize- must be a power of 2
            throw LowlevelError("Bad windowsize for disassembly cache");
        contexts.clear();
        contexts.reserve(minimumreuse);
        nextfree = 0;
        hashtable.assign(hashsize, nullptr);
        for (int4 i = 0; i < minimumreuse; ++i) {
            auto parser = std::make_unique<ParserContext>(contextcache, translate);
            ParserContext* pos = parser.get();
            contexts.push_back(std::move(parser));
            pos->initialize(constspace);
        }
        ParserContext* pos = contexts[0].get();
        for (int4 i = 0; i < hashsize; ++i)
            hashtable[i] = pos; // Make sure all hashtable positions point to a real ParserContext
    } ///< Initialize the hash-table of ParserContexts
public:
    DisassemblyCache(Translate* trans, ContextCache* ccache, AddrSpace* cspace, int4 cachesize, int4 windowsize) {
        translate = trans;
        contextcache = ccache;
        constspace = cspace;
        initialize(cachesize, windowsize); // Set default settings for the cache
    } ///< Constructor
    /// Releases cached parser contexts and their lookup views.
    ~DisassemblyCache(void) = default;
    ParserContext* getParserContext(const Address& addr) {
        int4 hashindex = ((int4)addr.getOffset()) & mask;
        ParserContext* res = hashtable[hashindex];
        if (res->getAddr() == addr)
            return res;
        res = contexts[nextfree].get();
        nextfree += 1; // Advance the circular index
        if (nextfree >= minimumreuse)
            nextfree = 0;
        res->setAddr(addr);
        res->setParserState(ParserContext::uninitialized); // Need to start over with parsing
        hashtable[hashindex] = res;                        // Stick it into the hashtable
        return res;
    } ///< Get the parser for a particular Address
};

/// \brief Build p-code from a pre-parsed instruction
///
/// Through the build() method, \b this walks the parse tree and prepares data
/// for final emission as p-code.  (The final emitting is done separately through the
/// PcodeCacher.emit() method).  Generally, only p-code for one instruction is prepared.
/// But, through the \b delay-slot mechanism, build() may recursively visit
/// additional instructions.
class SleighBuilder : public PcodeBuilder {
    virtual void dump(OpTpl* op) { // Dump on op through low-level dump interface
                                   // filling in dynamic loads and stores if necessary
        PcodeData* thisop;
        VarnodeData* invars;
        VarnodeData* loadvars;
        VarnodeData* storevars;
        VarnodeTpl *vn, *outvn;
        int4 isize = op->numInput();
        // First build all the inputs
        invars = cache->allocateVarnodes(isize);
        for (int4 i = 0; i < isize; ++i) {
            vn = op->getIn(i);
            if (vn->isDynamic(*walker)) {
                generateLocation(vn, invars[i]); // Input of -op- is really temporary storage
                PcodeData* load_op = cache->allocateInstruction();
                load_op->opc = CPUI_LOAD;
                load_op->outvar = invars + i;
                load_op->isize = 2;
                loadvars = load_op->invar = cache->allocateVarnodes(2);
                AddrSpace* spc = generatePointer(vn, loadvars[1]);
                loadvars[0].space = const_space;
                loadvars[0].offset = (uintb)(uintp)spc;
                loadvars[0].size = sizeof(spc);
                if (vn->getOffset().getSelect() == ConstTpl::v_offset_plus)
                    generatePointerAdd(load_op, vn);
            } else
                generateLocation(vn, invars[i]);
        }
        if ((isize > 0) && (op->getIn(0)->isRelative())) {
            invars->offset += getLabelBase();
            cache->addLabelRef(invars);
        }
        thisop = cache->allocateInstruction();
        thisop->opc = op->getOpcode();
        thisop->invar = invars;
        thisop->isize = isize;
        outvn = op->getOut();
        if (outvn != (VarnodeTpl*)0) {
            if (outvn->isDynamic(*walker)) {
                storevars = cache->allocateVarnodes(3);
                generateLocation(outvn, storevars[2]); // Output of -op- is really temporary storage
                thisop->outvar = storevars + 2;
                PcodeData* store_op = cache->allocateInstruction();
                store_op->opc = CPUI_STORE;
                store_op->isize = 3;
                // store_op->outvar = (VarnodeData *)0;
                store_op->invar = storevars;
                AddrSpace* spc = generatePointer(outvn, storevars[1]); // pointer
                storevars[0].space = const_space;
                storevars[0].offset = (uintb)(uintp)spc; // space in which to store
                storevars[0].size = sizeof(spc);
                if (outvn->getOffset().getSelect() == ConstTpl::v_offset_plus)
                    generatePointerAdd(store_op, outvn);
            } else {
                thisop->outvar = cache->allocateVarnodes(1);
                generateLocation(outvn, *thisop->outvar);
            }
        }
    }
    AddrSpace* const_space;     ///< The constant address space
    AddrSpace* uniq_space;      ///< The unique address space
    uintb uniquemask;           ///< Mask of address bits to use to uniquify temporary registers
    uintb uniqueoffset;         ///< Uniquifier bits for \b this instruction
    DisassemblyCache* discache; ///< Cache of disassembled instructions
    PcodeCacher* cache;         ///< Cache accumulating p-code data for the instruction
    void buildEmpty(Constructor* ct, int4 secnum) {
        int4 numops = ct->getNumOperands();

        for (int4 i = 0; i < numops; ++i) {
            SubtableSymbol* sym = (SubtableSymbol*)ct->getOperand(i)->getDefiningSymbol();
            if (sym == (SubtableSymbol*)0)
                continue;
            if (sym->getType() != SleighSymbol::subtable_symbol)
                continue;

            walker->pushOperand(i);
            ConstructTpl* construct = walker->getConstructor()->getNamedTempl(secnum);
            if (construct == (ConstructTpl*)0)
                buildEmpty(walker->getConstructor(), secnum);
            else
                build(construct, secnum);
            walker->popOperand();
        }
    }
    void generateLocation(const VarnodeTpl* vntpl, VarnodeData& vn) {
        vn.space = vntpl->getSpace().fixSpace(*walker);
        vn.size = vntpl->getSize().fix(*walker);
        if (vn.space == const_space)
            vn.offset = vntpl->getOffset().fix(*walker) & calc_mask(vn.size);
        else if (vn.space == uniq_space) {
            vn.offset = vntpl->getOffset().fix(*walker);
            vn.offset |= uniqueoffset;
        } else
            vn.offset = vn.space->wrapOffset(vntpl->getOffset().fix(*walker));
    }
    AddrSpace* generatePointer(const VarnodeTpl* vntpl, VarnodeData& vn) {
        const FixedHandle& hand(walker->getFixedHandle(vntpl->getOffset().getHandleIndex()));
        vn.space = hand.offset_space;
        vn.size = hand.offset_size;
        if (vn.space == const_space)
            vn.offset = hand.offset_offset & calc_mask(vn.size);
        else if (vn.space == uniq_space)
            vn.offset = hand.offset_offset | uniqueoffset;
        else
            vn.offset = vn.space->wrapOffset(hand.offset_offset);
        return hand.space;
    }
    void generatePointerAdd(PcodeData* op, const VarnodeTpl* vntpl) {
        uintb offsetPlus = vntpl->getOffset().getReal() & 0xffff;
        if (offsetPlus == 0) {
            return;
        }
        PcodeData* nextop = cache->allocateInstruction();
        nextop->opc = op->opc;
        nextop->invar = op->invar;
        nextop->isize = op->isize;
        nextop->outvar = op->outvar;
        op->isize = 2;
        op->opc = CPUI_INT_ADD;
        VarnodeData* newparams = op->invar = cache->allocateVarnodes(2);
        newparams[0] = nextop->invar[1];
        newparams[1].space = const_space; // Add in V_OFFSET_PLUS
        newparams[1].offset = offsetPlus;
        newparams[1].size = newparams[0].size;
        op->outvar = nextop->invar + 1; // Output of ADD is input to original op
        op->outvar->space = uniq_space; // Result of INT_ADD in special runtime temp
        op->outvar->offset = uniq_space->getTrans()->getUniqueStart(Translate::RUNTIME_BITRANGE_EA);
    }
    void setUniqueOffset(const Address& addr) {
        uniqueoffset = (addr.getOffset() & uniquemask) << 8;
    } ///< Set uniquifying bits for the current instruction
public:
    SleighBuilder(ParserWalker* w, DisassemblyCache* dcache, PcodeCacher* pc, AddrSpace* cspc, AddrSpace* uspc,
                  uint4 umask)
        : PcodeBuilder(0) {
        walker = w;
        discache = dcache;
        cache = pc;
        const_space = cspc;
        uniq_space = uspc;
        uniquemask = umask;
        uniqueoffset = (walker->getAddr().getOffset() & uniquemask) << 8;
    }
    virtual void appendBuild(OpTpl* bld, int4 secnum) {
        // Append p-code for a particular build statement
        int4 index = bld->getIn(0)->getOffset().getReal(); // Recover operand index from build statement
                                                           // Check if operand is a subtable
        SubtableSymbol* sym = (SubtableSymbol*)walker->getConstructor()->getOperand(index)->getDefiningSymbol();
        if ((sym == (SubtableSymbol*)0) || (sym->getType() != SleighSymbol::subtable_symbol))
            return;

        walker->pushOperand(index);
        Constructor* ct = walker->getConstructor();
        if (secnum >= 0) {
            ConstructTpl* construct = ct->getNamedTempl(secnum);
            if (construct == (ConstructTpl*)0)
                buildEmpty(ct, secnum);
            else
                build(construct, secnum);
        } else {
            ConstructTpl* construct = ct->getTempl();
            build(construct, -1);
        }
        walker->popOperand();
    }
    virtual void delaySlot(OpTpl* op) {
        // Append pcode for an entire instruction (delay slot)
        // in the middle of the current instruction
        ParserWalker* tmp = walker;
        uintb olduniqueoffset = uniqueoffset;

        Address baseaddr = tmp->getAddr();
        int4 fallOffset = tmp->getLength();
        int4 delaySlotByteCnt = tmp->getParserContext()->getDelaySlot();
        int4 bytecount = 0;
        do {
            Address newaddr = baseaddr + fallOffset;
            setUniqueOffset(newaddr);
            const ParserContext* pos = discache->getParserContext(newaddr);
            if (pos->getParserState() != ParserContext::pcode)
                throw LowlevelError("Could not obtain cached delay slot instruction");
            int4 len = pos->getLength();

            ParserWalker newwalker(pos);
            walker = &newwalker;
            walker->baseState();
            build(walker->getConstructor()->getTempl(), -1); // Build the whole delay slot
            fallOffset += len;
            bytecount += len;
        } while (bytecount < delaySlotByteCnt);
        walker = tmp; // Restore original context
        uniqueoffset = olduniqueoffset;
    }
    virtual void setLabel(OpTpl* op) {
        cache->addLabel(op->getIn(0)->getOffset().getReal() + getLabelBase());
    }
    virtual void appendCrossBuild(OpTpl* bld, int4 secnum) {
        // Weave in the p-code section from an instruction at another address
        // bld-param(0) contains the address of the instruction
        // bld-param(1) contains the section number
        if (secnum >= 0)
            throw LowlevelError("CROSSBUILD directive within a named section");
        secnum = bld->getIn(1)->getOffset().getReal();
        VarnodeTpl* vn = bld->getIn(0);
        AddrSpace* spc = vn->getSpace().fixSpace(*walker);
        uintb addr = spc->wrapOffset(vn->getOffset().fix(*walker));

        ParserWalker* tmp = walker;
        uintb olduniqueoffset = uniqueoffset;

        Address newaddr(spc, addr);
        setUniqueOffset(newaddr);
        const ParserContext* pos = discache->getParserContext(newaddr);
        if (pos->getParserState() != ParserContext::pcode)
            throw LowlevelError("Could not obtain cached crossbuild instruction");

        ParserWalker newwalker(pos, tmp->getParserContext());
        walker = &newwalker;

        walker->baseState();
        Constructor* ct = walker->getConstructor();
        ConstructTpl* construct = ct->getNamedTempl(secnum);
        if (construct == (ConstructTpl*)0)
            buildEmpty(ct, secnum);
        else
            build(construct, secnum);
        walker = tmp;
        uniqueoffset = olduniqueoffset;
    }
};

/// \brief A full SLEIGH engine
///
/// Its provided with a LoadImage of the bytes to be disassembled and
/// a ContextDatabase.
///
/// Assembly is produced via the printAssembly() method, provided with an
/// AssemblyEmit object and an Address.
///
/// P-code is produced via the oneInstruction() method, provided with a PcodeEmit
/// object and an Address.
class Sleigh : public SleighBase {
    LoadImage* loader;                                  ///< The mapped bytes in the program
    ContextDatabase* context_db;                        ///< Database of context values steering disassembly
    std::unique_ptr<ContextCache> cache;                ///< Owned cache of recently used context values
    mutable std::unique_ptr<DisassemblyCache> discache; ///< Owned cache of recently parsed instructions
    mutable PcodeCacher pcode_cache;                    ///< Cache of p-code data just prior to emitting
    void clearForDelete(void) {
        discache.reset();
        cache.reset();
    } ///< Delete the context and disassembly caches
protected:
    ParserContext* obtainContext(const Address& addr, ParserContext::parse_state state) const {
        ParserContext* pos = discache->getParserContext(addr);
        ParserContext::parse_state curstate = pos->getParserState();
        if (curstate >= state)
            return pos;
        if (curstate == ParserContext::uninitialized) {
            resolve(*pos);
            if (state == ParserContext::disassembly)
                return pos;
        }
        // If we reach here,  state must be ParserContext::pcode
        resolveHandles(*pos);
        return pos;
    }
    void resolve(ParserContext& pos) const {
        loader->loadFill(pos.getBuffer(), 16, pos.getAddr());
        ParserWalkerChange walker(&pos);
        pos.deallocateState(walker); // Clear the previous resolve and initialize the walker
        Constructor *ct, *subct;
        uint4 off;
        int4 oper, numoper;

        pos.setDelaySlot(0);
        walker.setOffset(0);        // Initial offset
        pos.clearCommits();         // Clear any old context commits
        pos.loadContext();          // Get context for current address
        ct = root->resolve(walker); // Base constructor
        walker.setConstructor(ct);
        ct->applyContext(walker);
        while (walker.isState()) {
            ct = walker.getConstructor();
            oper = walker.getOperand();
            numoper = ct->getNumOperands();
            while (oper < numoper) {
                OperandSymbol* sym = ct->getOperand(oper);
                off = walker.getOffset(sym->getOffsetBase()) + sym->getRelativeOffset();
                pos.allocateOperand(oper, walker); // Descend into new operand and reserve space
                walker.setOffset(off);
                TripleSymbol* tsym = sym->getDefiningSymbol();
                if (tsym != (TripleSymbol*)0) {
                    subct = tsym->resolve(walker);
                    if (subct != (Constructor*)0) {
                        walker.setConstructor(subct);
                        subct->applyContext(walker);
                        break;
                    }
                }
                walker.setCurrentLength(sym->getMinimumLength());
                walker.popOperand();
                oper += 1;
            }
            if (oper >= numoper) { // Finished processing constructor
                walker.calcCurrentLength(ct->getMinimumLength(), numoper);
                walker.popOperand();
                // Check for use of delayslot
                ConstructTpl* templ = ct->getTempl();
                if ((templ != (ConstructTpl*)0) && (templ->delaySlot() > 0))
                    pos.setDelaySlot(templ->delaySlot());
            }
        }
        pos.setNaddr(pos.getAddr() + pos.getLength()); // Update Naddr to pointer after instruction
        pos.setParserState(ParserContext::disassembly);
    } ///< Generate a parse tree suitable for disassembly
    void resolveHandles(ParserContext& pos) const {
        TripleSymbol* triple;
        Constructor* ct;
        int4 oper, numoper;

        ParserWalker walker(&pos);
        walker.baseState();
        while (walker.isState()) {
            ct = walker.getConstructor();
            oper = walker.getOperand();
            numoper = ct->getNumOperands();
            while (oper < numoper) {
                OperandSymbol* sym = ct->getOperand(oper);
                walker.pushOperand(oper); // Descend into node
                triple = sym->getDefiningSymbol();
                if (triple != (TripleSymbol*)0) {
                    if (triple->getType() == SleighSymbol::subtable_symbol)
                        break;
                    else // Some other kind of symbol as an operand
                        triple->getFixedHandle(walker.getParentHandle(), walker);
                } else { // Must be an expression
                    PatternExpression* patexp = sym->getDefiningExpression();
                    intb res = patexp->getValue(walker);
                    FixedHandle& hand(walker.getParentHandle());
                    hand.space = pos.getConstSpace(); // Result of expression is a constant
                    hand.offset_space = (AddrSpace*)0;
                    hand.offset_offset = (uintb)res;
                    hand.size = 0; // This size should not get used
                }
                walker.popOperand();
                oper += 1;
            }
            if (oper >= numoper) { // Finished processing constructor
                ConstructTpl* templ = ct->getTempl();
                if (templ != (ConstructTpl*)0) {
                    HandleTpl* res = templ->getResult();
                    if (res != (HandleTpl*)0) // Pop up handle to containing operand
                        res->fix(walker.getParentHandle(), walker);
                    // If we need an indicator that the constructor exports nothing try
                    // else
                    //   walker.getParentHandle().setInvalid();
                }
                walker.popOperand();
            }
        }
        pos.setParserState(ParserContext::pcode);
    } ///< Prepare the parse tree for p-code generation
public:
    Sleigh(LoadImage* ld, ContextDatabase* c_db)
        : SleighBase(), loader(ld), context_db(c_db), cache(std::make_unique<ContextCache>(c_db)), discache(nullptr) {
    } ///< Constructor
    virtual ~Sleigh(void) = default; ///< Destructor
    void reset(LoadImage* ld, ContextDatabase* c_db) {
        clearForDelete();
        pcode_cache.clear();
        loader = ld;
        context_db = c_db;
        cache = std::make_unique<ContextCache>(c_db);
        discache.reset();
    } ///< Reset the engine for a new program
    virtual void initialize(const string& slaFilename) {
        if (!isInitialized()) { // Initialize the base if not already
            sla::FormatDecode decoder(this);
            ifstream s(slaFilename, std::ios_base::binary);
            if (!s)
                throw LowlevelError("Could not open .sla file: " + slaFilename);
            decoder.ingestStream(s);
            s.close();
            decode(decoder);
        } else
            reregisterContext();
        uint4 parser_cachesize = 2;
        uint4 parser_windowsize = 32;
        if ((maxdelayslotbytes > 1) || (unique_allocatemask != 0)) {
            parser_cachesize = 8;
            parser_windowsize = 256;
        }
        discache = std::make_unique<DisassemblyCache>(this, cache.get(), getConstantSpace(), parser_cachesize,
                                                      parser_windowsize);
    }
    virtual void registerContext(const string& name, int4 sbit, int4 ebit) {
        context_db->registerVariable(name, sbit, ebit);
    }
    virtual void setContextDefault(const string& name, uintm val) {
        context_db->setVariableDefault(name, val);
    }
    virtual void allowContextSet(bool val) const {
        cache->allowSet(val);
    }
    virtual int4 instructionLength(const Address& baseaddr) const {
        ParserContext* pos = obtainContext(baseaddr, ParserContext::disassembly);
        return pos->getLength();
    }
    virtual int4 oneInstruction(PcodeEmit& emit, const Address& baseaddr) const {
        int4 fallOffset;
        if (alignment != 1) {
            if ((baseaddr.getOffset() % alignment) != 0) {
                ostringstream s;
                s << "Instruction address not aligned: " << baseaddr;
                throw UnimplError(s.str(), 0);
            }
        }

        ParserContext* pos = obtainContext(baseaddr, ParserContext::pcode);
        pos->applyCommits();
        fallOffset = pos->getLength();

        if (pos->getDelaySlot() > 0) {
            int4 bytecount = 0;
            do {
                // Do not pass pos->getNaddr() to obtainContext, as pos may have been previously cached and had naddr
                // adjusted
                ParserContext* delaypos = obtainContext(pos->getAddr() + fallOffset, ParserContext::pcode);
                delaypos->applyCommits();
                int4 len = delaypos->getLength();
                fallOffset += len;
                bytecount += len;
            } while (bytecount < pos->getDelaySlot());
            pos->setNaddr(pos->getAddr() + fallOffset);
        }
        ParserWalker walker(pos);
        walker.baseState();
        pcode_cache.clear();
        SleighBuilder builder(&walker, discache.get(), &pcode_cache, getConstantSpace(), getUniqueSpace(),
                              unique_allocatemask);
        try {
            builder.build(walker.getConstructor()->getTempl(), -1);
            pcode_cache.resolveRelatives();
            pcode_cache.emit(baseaddr, &emit);
        } catch (UnimplError& err) {
            ostringstream s;
            s << "Instruction not implemented in pcode:\n ";
            ParserWalker* cur = builder.getCurrentWalker();
            cur->baseState();
            Constructor* ct = cur->getConstructor();
            cur->getAddr().printRaw(s);
            s << ": ";
            ct->printMnemonic(s, *cur);
            s << "  ";
            ct->printBody(s, *cur);
            err.explain = s.str();
            err.instruction_length = fallOffset;
            throw err;
        }
        return fallOffset;
    }
    virtual int4 printAssembly(AssemblyEmit& emit, const Address& baseaddr) const {
        int4 sz;

        ParserContext* pos = obtainContext(baseaddr, ParserContext::disassembly);
        ParserWalker walker(pos);
        walker.baseState();

        Constructor* ct = walker.getConstructor();
        ostringstream mons;
        ct->printMnemonic(mons, walker);
        ostringstream body;
        ct->printBody(body, walker);
        emit.dump(baseaddr, mons.str(), body.str());
        sz = pos->getLength();
        return sz;
    }
};

/** \page sleigh SLEIGH

  \section sleightoc Table of Contents

    - \ref sleighoverview
    - \ref sleighbuild
    - \ref sleighuse
    - \subpage sleighAPIbasic
    - \subpage sleighAPIemulate

  \b Key \b Classes
    - \ref Translate
    - \ref AssemblyEmit
    - \ref PcodeEmit
    - \ref LoadImage
    - \ref ContextDatabase

  \section sleighoverview Overview

  Welcome to \b SLEIGH, a machine language translation and
  dissassembly engine.  SLEIGH is both a processor
  specification language and the associated library and
  tools for using such a specification to generate assembly
  and to generate \b pcode, a reverse engineering Register
  Transfer Language (RTL), from binary machine instructions.

  SLEIGH was originally based on \b SLED, a
  \e Specification \e Language \e for \e Encoding \e and
  \e Decoding, designed by Norman Ramsey and Mary F. Fernandez,
  which performed disassembly (and assembly).  SLEIGH
  extends SLED by providing semantic descriptions (via the
  RTL) of machine instructions and other practical enhancements
  for doing real world reverse engineering.

  SLEIGH is part of Project \b GHIDRA. It provides the core
  of the GHIDRA disassembler and the data-flow and
  decompilation analysis.  However, SLEIGH can serve as a
  standalone library for use in other applications for
  providing a generic disassembly and RTL translation interface.

  \section sleighbuild Building SLEIGH

  There are a couple of \e make targets for building the SLEIGH
  library from source.  These are:

  \code
     make libsla.a               # Build the main library

     make libsla_dbg.a           # Build the library with debug symbols
  \endcode

  The source code file \e sleighexample.cppm has a complete example
  of initializing the Translate engine and using it
  to generate assembly and pcode.  The source has a hard-coded file name,
  \e x86testcode, as the example binary executable it attempts
  to decode, but this can easily be changed.  It also needs
  a SLEIGH specification file (\e .sla) to be present.

  Building the example application can be done with something
  similar to the following makefile fragment.

  \code
    # The C compiler
    CXX=g++

    # Debug flags
    DBG_CXXFLAGS=-g -Wall -Wno-sign-compare

    OPT_CXXFLAGS=-O2 -Wall -Wno-sign-compare

    # libraries
    INCLUDES=-I./src

    LNK=src/libsla_dbg.a

    sleighexample.obj:    sleighexample.cppm
          $(CXX) -c $(DBG_CXXFLAGS) -o sleighexample sleighexample.o $(LNK)

    clean:
          rm -rf *.o sleighexample
  \endcode

  \section sleighuse Using SLEIGH

  SLEIGH is a generic reverse engineering tool in the sense
  that the API is designed to be completely processor
  independent.  In order to process binary executables for a
  specific processor, The library reads in a \e
  specification \e file, which describes how instructions
  are encoded and how they are interpreted by the processor.
  An application which needs to do disassembly or generate
  \b pcode can design to the SLEIGH API once, and then the
  application will automatically support any processor for
  which there is a specification.

  For working with a single processor, the SLEIGH library
  needs to load a single \e compiled form of the processor
  specification, which is traditionally given a ".sla" suffix.
  Most common processors already have a ".sla" file available.
  So to use SLEIGH with these processors, the library merely
  needs to be made aware of the desired file.  This documentation
  covers the use of the SLEIGH API, assuming that this
  specification file is available.

  The ".sla" files themselves are created by running
  the \e compiler on a file written in the formal SLEIGH
  language.  These files traditionally have the suffix ".slaspec"
  For those who want to design such a specification for a new
  processor, please refer to the document, "SLEIGH: A Language
  for Rapid Processor Specification."

 */

/**
 \page sleighAPIbasic The Basic SLEIGH Interface

 To use SLEIGH as a library within an application, there
 are basically five classes that you need to be aware of.

   - \ref sleightranslate
   - \ref sleighassememit
   - \ref sleighpcodeemit
   - \ref sleighloadimage
   - \ref sleighcontext

 \section sleightranslate Translate (or Sleigh)

 The core SLEIGH class is Sleigh, which is derived from the
 interface, Translate.  In order to instantiate it in your code,
 you need a LoadImage object, and a ContextDatabase object.
 The load image is responsible for retrieving instruction
 bytes, based on address, from a binary executable. The context
 database provides the library extra mode information that may
 be necessary to do the disassembly or translation.  This can
 be used, for instance, to specify that an x86 binary is running
 in 32-bit mode, or to specify that an ARM processor is running
 in THUMB mode.  Once these objects are built, the Sleigh
 object can be immediately instantiated.

 \code
 LoadImageBfd *loader;
 ContextDatabase *context;
 Translate *trans;

 // Set up the loadimage
 // Providing an executable name and architecture
 string loadimagename = "x86testcode";
 string bfdtarget= "default";

 loader = new LoadImageBfd(loadimagename,bfdtarget);
 loader->open();       // Load the executable from file

 context = new ContextInternal();   // Create a processor context

 trans = new Sleigh(loader,context);  // Instantiate the translator
 \endcode

  Once the Sleigh object is in hand, initialize it with the path
  to the compiled binary ".sla" specification.

 \section sleighassememit AssemblyEmit

 In order to do disassembly, you need to derive a class from
 AssemblyEmit, and implement the method \e dump.  The library
 will call this method exactly once, for each instruction
 disassembled.

 This routine simply needs to decide how (and where) to print
 the corresponding portion of the disassembly.  For instance,

 \code
 class AssemblyRaw : public AssemblyEmit {
 public:
   virtual void dump(const Address &addr,const string &mnem,const string &body) {
     addr.printRaw(cout);
     cout << ": " << mnem << ' ' << body << endl;
   }
 };
 \endcode

 This is a minimal implementation that simply dumps the
 disassembly straight to standard out.  Once this object is
 instantiated, the Sleigh object can use it to write out
 assembly via the Translate::printAssembly() method.

 \code
 AssemblyEmit *assememit = new AssemblyRaw();

 Address addr(trans->getDefaultCodeSpace(),0x80484c0);
 int4 length;                  // Length of instruction in bytes

 length = trans->printAssembly(*assememit,addr);
 addr = addr + length;        // Advance to next instruction
 length = trans->printAssembly(*assememit,addr);
 addr = addr + length;
 length = trans->printAssembly(*assememit,addr);
 \endcode

 \section sleighpcodeemit PcodeEmit

 In order to generate a \b pcode translation of a machine
 instruction, you need to derive a class from PcodeEmit and
 implement the virtual method \e dump. This method will be
 invoked once for each \b pcode operation in the translation
 of a machine instruction.  There will likely be multiple calls
 per instruction.  Each call passes in a single \b pcode
 operation, complete with its possible varnode output, and
 all of its varnode inputs.  Here is an example of a PcodeEmit
 object that simply prints out the \b pcode.

 \code
 class PcodeRawOut : public PcodeEmit {
 public:
   virtual void dump(const Address &addr,OpCode opc,VarnodeData *outvar,VarnodeData *vars,int4 isize);
 };

 static void print_vardata(ostream &s,VarnodeData &data)

 {
   s << '(' << data.space->getName() << ',';
   data.space->printOffset(s,data.offset);
   s << ',' << dec << data.size << ')';
 }

 void PcodeRawOut::dump(const Address &addr,OpCode opc,VarnodeData *outvar,VarnodeData *vars,int4 isize)

 {
   if (outvar != (VarnodeData *)0) {     // The output is optional
     print_vardata(cout,*outvar);
     cout << " = ";
   }
   cout << opcode_name[opc];
   // Possibly check for a code reference or a space reference
   for(int4 i=0;i<isize;++i) {
     cout << ' ';
     print_vardata(cout,vars[i]);
   }
   cout << endl;
 }
 \endcode

  Notice that the \e dump routine uses the opcode name table
  to find a string version of the opcode.  Each
 varnode is
 defined in terms of the VarnodeData object, which is defined simply:

 \code
 struct VarnodeData {
   AddrSpace *space;          // The address space
   uintb offset;              // The offset within the space
   uint4 size;                // The number of bytes at that location
 };
 \endcode

 Once the PcodeEmit object is instantiated, the Sleigh object can
 use it to generate pcode, one instruction at a time, using the
 Translate::oneInstruction() const method.

 \code
 PcodeEmit *pcodeemit = new PcodeRawOut();

 Address addr(trans->getDefaultCodeSpace(),0x80484c0);
 int4 length;                   // Length of instruction in bytes

 length = trans->oneInstruction(*pcodeemit,addr);
 addr = addr + length;         // Advance to next instruction
 length = trans->oneInstruction(*pcodeemit,addr);
 addr = addr + length;
 length = trans->oneInstruction(*pcodeemit,addr);
 \endcode

 For an application to properly \e follow \e flow, while translating
 machine instructions into pcode, the emitted pcode must be
 inspected for the various branch operations.

 \section sleighloadimage LoadImage

 A LoadImage holds all the binary data from an executable file
 in the format similar to how it would exist when being executed
 by a real processor.  The interface to this from SLEIGH is
 actually very simple, although it can hide a complicated
 structure.  One method does most of the work, LoadImage::loadFill().
 It takes a byte pointer, a size, and an Address. The method
 is expected to fill in the \e ptr array with \e size bytes
 taken from the load image, corresponding to the address \e addr.
 There are two more virtual methods that are required for a
 complete implementation of LoadImage, \e getArchType and
 \e adjustVma, but these do not need to be implemented fully.

 \code
 class MyLoadImage : public LoadImage {
 public:
   MyLoadImage() : LoadImage() {}
   virtual void loadFill(uint1 *ptr,int4 size,const Address &addr);
   virtual string getArchType(void) const { return "mytype"; }
   virtual void adjustVma(long adjust) {}
 };
 \endcode

 \section sleighcontext ContextDatabase

 The ContextDatabase needs to keep track of any possible
 context variable and its value, over different address ranges.
 In most cases, you probably don't need to override the class
 yourself, but can use the built-in class, ContextInternal.
 This provides the basic functionality required and will work
 for different architectures.  What you may need to do is
 set values for certain variables, depending on the processor
 and the environment it is running in.  For instance, for
 the x86 platform, you need to set the \e addrsize and \e opsize
 bits, to indicate the processor would be running in 32-bit
 mode.  The context variables specific to a particular processor
 are established by the SLEIGH spec.  So the variables can
 only be set \e after the spec has been loaded.

 \code
   ...
   context = new ContextInternal();
   trans = new Sleigh(loader,context);
    trans->initialize("specfiles/x86.sla");

   context->setVariableDefault("addrsize",1);  // Address size is 32-bits
   context->setVariableDefault("opsize",1);    // Operand size is 32-bits
 \endcode


*/

} // End namespace ghidra
