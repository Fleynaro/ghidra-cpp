// Port provenance: declarations and implementation originate from
// Ghidra/Features/Decompiler/src/decompile/cpp/signature.hh and signature.cc.
export module ghidra.decompiler.signature;
import std;
import ghidra.decompiler;

export namespace ghidra {
using hashword = uint8;

extern AttributeId ATTRIB_BADDATA;
extern AttributeId ATTRIB_HASH;
extern AttributeId ATTRIB_UNIMPL;
extern ElementId ELEM_BLOCKSIG;
extern ElementId ELEM_CALL;
extern ElementId ELEM_GENSIG;
extern ElementId ELEM_MAJOR;
extern ElementId ELEM_MINOR;
extern ElementId ELEM_COPYSIG;
extern ElementId ELEM_SETTINGS;
extern ElementId ELEM_SIG;
extern ElementId ELEM_SIGNATUREDESC;
extern ElementId ELEM_SIGNATURES;
extern ElementId ELEM_SIGSETTINGS;
extern ElementId ELEM_VARSIG;

/// A hashed feature extracted from a function's data-flow or control-flow.
class Signature {
    uint4 sig;

public:
    explicit Signature(hashword h) : sig(static_cast<uint4>(h)) {}
    uint4 getHash(void) const {
        return sig;
    }
    void print(ostream& s) const;
    int4 compare(const Signature* op2) const;
    virtual ~Signature() = default;
    virtual void encode(Encoder& encoder) const;
    virtual void decode(Decoder& decoder);
    virtual void printOrigin(ostream& s) const {
        s << hex << "0x" << setfill('0') << setw(8) << sig;
    }
    static bool comparePtr(Signature* a, Signature* b) {
        return a->sig < b->sig;
    }
};

/// Overlay node used while hashing the data-flow graph.
class SignatureEntry {
    enum SignatureFlags {
        SIG_NODE_TERMINAL = 0x1,
        SIG_NODE_COMMUTATIVE = 0x2,
        SIG_NODE_NOT_EMITTED = 0x4,
        SIG_NODE_STANDALONE = 0x8,
        VISITED = 0x10,
        MARKER_ROOT = 0x20
    };
    struct DFSNode {
        SignatureEntry* entry;
        list<PcodeOp*>::const_iterator iter;
    };
    Varnode* vn;
    uint4 flags;
    hashword hash[2];
    const PcodeOp* op;
    int4 startvn;
    int4 inSize;
    int4 index;
    SignatureEntry* shadow;
    hashword getOpHash(uint4 modifiers);
    bool isVisited(void) const {
        return (flags & VISITED) != 0;
    }
    void setVisited(void) {
        flags |= VISITED;
    }
    int4 markerSizeIn(void) const {
        return (flags & MARKER_ROOT) != 0 ? 1 : numInputs();
    }
    SignatureEntry* getMarkerIn(int4 i, SignatureEntry* vRoot, const map<int4, SignatureEntry*>& sigMap) const {
        if ((flags & MARKER_ROOT) != 0)
            return vRoot;
        return mapToEntry(op->getIn(i + startvn), sigMap);
    }
    void standaloneCopyHash(uint4 modifiers);
    static bool testStandaloneCopy(Varnode* vn);
    static void noisePostOrder(const vector<SignatureEntry*>& rootlist, vector<SignatureEntry*>& postOrder,
                               map<int4, SignatureEntry*>& sigMap);
    static void noiseDominator(vector<SignatureEntry*>& postOrder, map<int4, SignatureEntry*>& sigMap);

public:
    SignatureEntry(Varnode* v, uint4 modifiers);
    explicit SignatureEntry(int4 ind);
    bool isTerminal(void) const {
        return (flags & SIG_NODE_TERMINAL) != 0;
    }
    bool isNotEmitted(void) const {
        return (flags & SIG_NODE_NOT_EMITTED) != 0;
    }
    bool isCommutative(void) const {
        return (flags & SIG_NODE_COMMUTATIVE) != 0;
    }
    bool isStandaloneCopy(void) const {
        return (flags & SIG_NODE_STANDALONE) != 0;
    }
    int4 numInputs(void) const {
        return inSize;
    }
    SignatureEntry* getIn(int4 i, const map<int4, SignatureEntry*>& sigMap) const {
        return mapToEntryCollapse(op->getIn(i + startvn), sigMap);
    }
    void calculateShadow(const map<int4, SignatureEntry*>& sigMap);
    void localHash(uint4 modifiers);
    void flip(void) {
        hash[1] = hash[0];
    }
    void hashIn(vector<SignatureEntry*>& neigh);
    Varnode* getVarnode(void) const {
        return vn;
    }
    hashword getHash(void) const {
        return hash[0];
    }
    static SignatureEntry* mapToEntry(const Varnode* vn, const map<int4, SignatureEntry*>& sigMap);
    static SignatureEntry* mapToEntryCollapse(const Varnode* vn, const map<int4, SignatureEntry*>& sigMap);
    static void removeNoise(map<int4, SignatureEntry*>& sigMap);
    static hashword hashSize(Varnode* vn, uint4 modifiers);
};

/// Overlay node used while hashing the control-flow graph.
class BlockSignatureEntry {
    BlockBasic* bl;
    hashword hash[2];

public:
    explicit BlockSignatureEntry(BlockBasic* b) : bl(b) {}
    void localHash(uint4 modifiers);
    void flip(void) {
        hash[1] = hash[0];
    }
    void hashIn(vector<BlockSignatureEntry*>& neigh);
    BlockBasic* getBlock(void) const {
        return bl;
    }
    hashword getHash(void) const {
        return hash[0];
    }
};

/// Feature rooted at a data-flow Varnode.
class VarnodeSignature : public Signature {
    const Varnode* vn;

public:
    VarnodeSignature(const Varnode* v, hashword h) : Signature(h), vn(v) {}
    void encode(Encoder& encoder) const override;
    void printOrigin(ostream& s) const override {
        vn->printRaw(s);
    }
};

/// Feature rooted at one or two operations in a basic block.
class BlockSignature : public Signature {
    const BlockBasic* bl;
    const PcodeOp* op1;
    const PcodeOp* op2;

public:
    BlockSignature(const BlockBasic* b, hashword h, const PcodeOp* o1, const PcodeOp* o2)
        : Signature(h), bl(b), op1(o1), op2(o2) {}
    void encode(Encoder& encoder) const override;
    void printOrigin(ostream& s) const override {
        bl->printHeader(s);
    }
};

/// Feature representing stand-alone copies in one basic block.
class CopySignature : public Signature {
    const BlockBasic* bl;

public:
    CopySignature(const BlockBasic* b, hashword h) : Signature(h), bl(b) {}
    void encode(Encoder& encoder) const override;
    void printOrigin(ostream& s) const override;
};

/// Base manager for collecting a function feature vector.
class SigManager {
    static uint4 settings;
    vector<Signature*> sigs;
    void clearSignatures(void);

protected:
    const Funcdata* fd;
    void addSignature(Signature* sig) {
        sigs.push_back(sig);
    }

public:
    SigManager() : fd(nullptr) {}
    virtual ~SigManager() {
        clearSignatures();
    }
    virtual void clear(void);
    virtual void initializeFromStream(istream& s) = 0;
    virtual void setCurrentFunction(const Funcdata* f);
    virtual void generate(void) = 0;
    int4 numSignatures(void) const {
        return static_cast<int4>(sigs.size());
    }
    Signature* getSignature(int4 i) const {
        return sigs[i];
    }
    void getSignatureVector(vector<uint4>& feature) const;
    hashword getOverallHash(void) const;
    void sortByHash(void) {
        sort(sigs.begin(), sigs.end(), Signature::comparePtr);
    }
    void print(ostream& s) const;
    void encode(Encoder& encoder) const;
    static uint4 getSettings(void) {
        return settings;
    }
    static void setSettings(uint4 newvalue);
};

/// Manager that generates features from function data-flow and control-flow.
class GraphSigManager : public SigManager {
public:
    enum Mods {
        SIG_COLLAPSE_SIZE = 0x1,
        SIG_COLLAPSE_INDNOISE = 0x2,
        SIG_DONOTUSE_CONST = 0x10,
        SIG_DONOTUSE_INPUT = 0x20,
        SIG_DONOTUSE_PERSIST = 0x40
    };

private:
    uint4 sigmods;
    int4 maxiter;
    int4 maxblockiter;
    int4 maxvarnode;
    map<int4, SignatureEntry*> sigmap;
    map<int4, BlockSignatureEntry*> blockmap;
    void signatureIterate(void);
    void signatureBlockIterate(void);
    void collectVarnodeSigs(void);
    void collectBlockSigs(void);
    void varnodeClear(void);
    void blockClear(void);
    void initializeBlocks(void);
    void flipVarnodes(void);
    void flipBlocks(void);

public:
    void clear(void) override;
    GraphSigManager(void);
    ~GraphSigManager() override {
        varnodeClear();
    }
    void setMaxIteration(int4 val) {
        maxiter = val;
    }
    void setMaxBlockIteration(int4 val) {
        maxblockiter = val;
    }
    void setMaxVarnode(int4 val) {
        maxvarnode = val;
    }
    void initializeFromStream(istream& s) override;
    void setCurrentFunction(const Funcdata* f) override;
    void generate(void) override;
    static bool testSettings(uint4 val);
};

inline SignatureEntry* SignatureEntry::mapToEntry(const Varnode* vn, const map<int4, SignatureEntry*>& sigMap) {
    return sigMap.find(vn->getCreateIndex())->second;
}

inline SignatureEntry* SignatureEntry::mapToEntryCollapse(const Varnode* vn, const map<int4, SignatureEntry*>& sigMap) {
    SignatureEntry* result = mapToEntry(vn, sigMap);
    return result->shadow == nullptr ? result : result->shadow;
}

inline hashword SignatureEntry::hashSize(Varnode* vn, uint4 modifiers) {
    hashword value = static_cast<hashword>(vn->getSize());
    if ((modifiers & GraphSigManager::SIG_COLLAPSE_SIZE) != 0 && value > 4)
        value = 4;
    return value ^ (value << 7) ^ (value << 14) ^ (value << 21);
}

void simpleSignature(Funcdata* fd, Encoder& encoder);
void debugSignature(Funcdata* fd, Encoder& encoder);
} // namespace ghidra

#define __SIGNATURE_HH__
#define __CRC32_HH__
#include "../../../../Ghidra/Features/Decompiler/src/decompile/cpp/signature.cc"
#undef __CRC32_HH__
#undef __SIGNATURE_HH__
