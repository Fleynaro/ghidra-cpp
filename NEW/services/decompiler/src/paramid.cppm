// Port provenance: declarations and implementation originate from
// Ghidra/Features/Decompiler/src/decompile/cpp/paramid.hh and paramid.cc.
export module ghidra.decompiler.paramid;
import std;
import ghidra.decompiler;

export namespace ghidra {
extern ElementId ELEM_PARAMMEASURES;
extern ElementId ELEM_PROTO;
extern ElementId ELEM_RANK;

/// Calculate and serialize the data-flow rank of one formal parameter or return value.
class ParamMeasure {
public:
    enum ParamIDIO { INPUT = 0, OUTPUT = 1 };
    enum ParamRank {
        BESTRANK = 1,
        DIRECTWRITEWITHOUTREAD = 1,
        DIRECTREAD = 2,
        DIRECTWRITEWITHREAD = 2,
        DIRECTWRITEUNKNOWNREAD = 3,
        SUBFNPARAM = 4,
        THISFNPARAM = 4,
        SUBFNRETURN = 5,
        THISFNRETURN = 5,
        INDIRECT = 6,
        WORSTRANK = 7
    };
    struct WalkState {
        bool best;
        int4 depth;
        ParamRank terminalrank;
    };

private:
    VarnodeData vndata;
    Datatype* vntype;
    ParamRank rank;
    ParamIDIO io;
    int4 numcalls;
    void walkforward(WalkState& state, PcodeOp* ignoreop, Varnode* vn);
    void walkbackward(WalkState& state, PcodeOp* ignoreop, Varnode* vn);
    void updaterank(ParamRank rank_in, bool best) {
        rank = best ? min(rank, rank_in) : max(rank, rank_in);
    }

public:
    ParamMeasure(const Address& addr, int4 sz, Datatype* dt, ParamIDIO io_in) {
        vndata.space = addr.getSpace();
        vndata.offset = addr.getOffset();
        vndata.size = sz;
        vntype = dt;
        io = io_in;
        rank = WORSTRANK;
    }
    void calculateRank(bool best, Varnode* basevn, PcodeOp* ignoreop);
    void encode(Encoder& encoder, ElementId& tag, bool moredetail) const;
    void savePretty(ostream& s, bool moredetail) const;
    int4 getMeasure(void) const {
        return static_cast<int4>(rank);
    }
};

/// Collect parameter and return-value measurements for one function.
class ParamIDAnalysis {
    Funcdata* fd;
    list<ParamMeasure> InputParamMeasures;
    list<ParamMeasure> OutputParamMeasures;

public:
    ParamIDAnalysis(Funcdata* fd_in, bool justproto);
    void encode(Encoder& encoder, bool moredetail) const;
    void savePretty(ostream& s, bool moredetail) const;
};
} // namespace ghidra

#define __PARAMID_HH__
#include "../../../../Ghidra/Features/Decompiler/src/decompile/cpp/paramid.cc"
#undef __PARAMID_HH__
