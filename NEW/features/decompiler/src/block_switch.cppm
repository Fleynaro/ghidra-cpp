// Port provenance: BlockSwitch declarations and definitions originate from
// Ghidra/Features/Decompiler/src/decompile/cpp/block.hh and block.cc.
export module ghidra.decompiler.block_switch;
import std;
import ghidra.decompiler;

export namespace ghidra {
export class BlockSwitch : public BlockGraph {
    JumpTable* jump; ///< Jump table associated with this switch
    /// \brief A class for annotating and sorting the individual cases of the switch
    struct CaseOrder {
        FlowBlock* block;            ///< The structured \e case block
        const FlowBlock* basicblock; ///< The first basic-block to execute within the \e case block
        uintb label;                 ///< The \e label for this case, as an untyped constant
        int4 depth;                  ///< How deep in a fall-thru chain we are
        int4 chain;                  ///< Who we immediately chain to, expressed as caseblocks index, -1 for no chaining
        int4 outindex;               ///< Index coming out of switch to this case
        uint4 gototype;              ///< (If non-zero) What type of unstructured \e case is this?
        bool isexit;                 ///< Does this case flow to the \e exit block
        bool isdefault;              ///< True if this is formal \e default case for the switch
        static bool compare(const CaseOrder& a, const CaseOrder& b); ///< Compare two cases
    };
    mutable vector<CaseOrder> caseblocks;                       ///< Blocks associated with switch cases
    void addCase(FlowBlock* switchbl, FlowBlock* bl, uint4 gt); ///< Add a new \e case to this switch
public:
    BlockSwitch(FlowBlock* ind); ///< Construct given the multi-exit root block
    void grabCaseBasic(FlowBlock* switchbl, const vector<FlowBlock*>& cs); ///< Build annotated CaseOrder objects
    FlowBlock* getSwitchBlock(void) const {
        return getBlock(0);
    } ///< Get the root switch component
    int4 getNumCaseBlocks(void) const {
        return caseblocks.size();
    } ///< Get the number of cases
    FlowBlock* getCaseBlock(int4 i) const {
        return caseblocks[i].block;
    } ///< Get the i-th \e case FlowBlock

    /// \brief Get the number of labels associated with one \e case block
    ///
    /// \param i is the index of the \e case block
    /// \return the number of labels put on the associated block
    int4 getNumLabels(int4 i) const {
        return jump->numIndicesByBlock(caseblocks[i].basicblock);
    }

    /// \brief Get a specific label associated with a \e case block
    ///
    /// \param i is the index of the \e case block
    /// \param j is the index of the specific label
    /// \return the label as an untyped constant
    uintb getLabel(int4 i, int4 j) const {
        return jump->getLabelByIndex(jump->getIndexByBlock(caseblocks[i].basicblock, j));
    }

    bool isDefaultCase(int4 i) const {
        return caseblocks[i].isdefault;
    } ///< Is the i-th \e case the \e default case
    uint4 getGotoType(int4 i) const {
        return caseblocks[i].gototype;
    } ///< Get the edge type for the i-th \e case block
    bool isExit(int4 i) const {
        return caseblocks[i].isexit;
    } ///< Does the i-th \e case block exit the switch?
    const Datatype* getSwitchType(void) const; ///< Get the data-type of the switch variable
    uint4 getDisplayFormat(void) const {
        return jump->getDisplayFormat();
    } ///< Get any integer display format for cases
    virtual block_type getType(void) const {
        return t_switch;
    }
    virtual void markUnstructured(void);
    virtual void scopeBreak(int4 curexit, int4 curloopexit);
    virtual void printHeader(ostream& s) const;
    virtual void emit(PrintLanguage* lng) const {
        lng->emitBlockSwitch(this);
    }
    virtual FlowBlock* nextFlowAfter(const FlowBlock* bl) const;
    virtual void finalizePrinting(Funcdata& data) const;
};
} // namespace ghidra

namespace ghidra {
BlockSwitch::BlockSwitch(FlowBlock* ind)

{
    jump = ind->getJumptable();
}

/// Associate a structured block as a full \e case of \b this switch.
/// \param switchbl is the underlying switch statement block
/// \param bl is the new block to make into a case
/// \param gt gives the unstructured branch type if the switch edge to the new case was unstructured (zero otherwise)
void BlockSwitch::addCase(FlowBlock* switchbl, FlowBlock* bl, uint4 gt)

{
    caseblocks.emplace_back();
    CaseOrder& curcase(caseblocks.back());
    const FlowBlock* basicbl = bl->getFrontLeaf()->subBlock(0);
    curcase.block = bl;
    curcase.basicblock = basicbl;
    curcase.label = 0;
    curcase.depth = 0;
    curcase.chain = -1;
    int4 inindex = basicbl->getInIndex(switchbl);
    if (inindex == -1)
        throw LowlevelError("Case block has become detached from switch");
    curcase.outindex = basicbl->getInRevIndex(inindex);
    curcase.gototype = gt;
    if (gt != 0)
        curcase.isexit = false;
    else
        curcase.isexit = (bl->sizeOut() == 1);
    curcase.isdefault = switchbl->isDefaultBranch(curcase.outindex);
}

/// Given the list of components for the switch structure, build the annotated descriptions
/// of the cases.  Work out flow between cases and if there are any unstructured cases.
/// The first FlowBlock in the component list is the switch component itself.  All other
/// FlowBlocks in the list are the \e case components.
/// \param switchbl is the underlying basic block, with multiple outgoing edges, for the switch
/// \param cs is the list of switch and case components
void BlockSwitch::grabCaseBasic(FlowBlock* switchbl, const vector<FlowBlock*>& cs)

{
    vector<int4> casemap(switchbl->sizeOut(), -1); // Map from switchtarget's outindex to position in caseblocks
    caseblocks.clear();
    for (int4 i = 1; i < cs.size(); ++i) {
        FlowBlock* casebl = cs[i];
        addCase(switchbl, casebl, 0);
        casemap[caseblocks[i - 1].outindex] = i - 1; // Build map from outindex to caseblocks index
    }
    // Fillin fallthru chaining
    for (int4 i = 0; i < caseblocks.size(); ++i) {
        CaseOrder& curcase(caseblocks[i]);
        FlowBlock* casebl = curcase.block;
        if (casebl->getType() == t_goto) { // All fall-thru blocks are plain gotos
            FlowBlock* targetbl = ((BlockGoto*)casebl)->getGotoTarget();
            const FlowBlock* basicbl = targetbl->getFrontLeaf()->subBlock(0);
            int4 inindex = basicbl->getInIndex(switchbl);
            if (inindex == -1)
                continue; // Goto target is not another switch case
            curcase.chain = casemap[basicbl->getInRevIndex(inindex)];
        }
    }

    if (cs[0]->getType() == t_multigoto) { // Check if some of the main switch edges were marked as goto
        BlockMultiGoto* gotoedgeblock = (BlockMultiGoto*)cs[0];
        int4 numgoto = gotoedgeblock->numGotos();
        for (int4 i = 0; i < numgoto; ++i)
            addCase(switchbl, gotoedgeblock->getGoto(i), f_goto_goto);
    }
}

void BlockSwitch::finalizePrinting(Funcdata& data) const

{
    BlockGraph::finalizePrinting(data); // Make sure to still recurse
    // We need to order the cases based on the label
    // First populate the label and depth fields of the CaseOrder objects
    for (int4 i = 0; i < caseblocks.size(); ++i) { // Construct the depth parameter, to sort fall-thru cases
        CaseOrder& curcase(caseblocks[i]);
        int4 j = curcase.chain;
        while (j != -1) { // Run through the fall-thru chain
            if (caseblocks[j].depth != 0)
                break;                // Break any possible loops (already visited this node)
            caseblocks[j].depth = -1; // Mark non-roots of chains
            j = caseblocks[j].chain;
        }
    }
    for (int4 i = 0; i < caseblocks.size(); ++i) {
        CaseOrder& curcase(caseblocks[i]);
        if (jump->numIndicesByBlock(curcase.basicblock) > 0) {
            if (curcase.depth == 0) { // Only set label on chain roots
                int4 ind = jump->getIndexByBlock(curcase.basicblock, 0);
                curcase.label = jump->getLabelByIndex(ind);
                int4 j = curcase.chain;
                int4 depthcount = 1;
                while (j != -1) {
                    if (caseblocks[j].depth > 0)
                        break; // Has this node had its depth set. Break any possible loops.
                    caseblocks[j].depth = depthcount++;
                    caseblocks[j].label = curcase.label;
                    j = caseblocks[j].chain;
                }
            }
        } else
            curcase.label = 0; // Should never happen
    }
    // Do actual sort of the cases based on label
    stable_sort(caseblocks.begin(), caseblocks.end(), CaseOrder::compare);
}

/// Drill down to the variable associated with the BRANCHIND itself, and return its data-type
/// \return the Datatype associated with the switch variable
const Datatype* BlockSwitch::getSwitchType(void) const

{
    PcodeOp* op = jump->getIndirectOp();
    return op->getIn(0)->getHighTypeReadFacing(op);
}

void BlockSwitch::markUnstructured(void)

{
    BlockGraph::markUnstructured(); // Recurse
    for (int4 i = 0; i < caseblocks.size(); ++i) {
        if (caseblocks[i].gototype == f_goto_goto)
            markCopyBlock(caseblocks[i].block, f_unstructured_targ);
    }
}

void BlockSwitch::scopeBreak(int4 curexit, int4 curloopexit)

{
    // New scope, current loop exit = curexit
    getBlock(0)->scopeBreak(-1, curexit); // Top block has multiple exits
    for (int4 i = 0; i < caseblocks.size(); ++i) {
        FlowBlock* bl = caseblocks[i].block;
        if (caseblocks[i].gototype != 0) {
            if (bl->getIndex() == curexit) // A goto that goes straight to exit, print is (empty) break
                caseblocks[i].gototype = f_break_goto;
        } else {
            // All case blocks are either plaingotos (curexit doesn't matter)
            //                            exitpoints (exit to switches exit   curexit = curexit)
            bl->scopeBreak(curexit, curexit);
        }
    }
}

void BlockSwitch::printHeader(ostream& s) const

{
    s << "Switch block ";
    FlowBlock::printHeader(s);
}

FlowBlock* BlockSwitch::nextFlowAfter(const FlowBlock* bl) const

{
    if (getBlock(0) == bl)
        return (FlowBlock*)0; // Don't know what will execute

    // Can only evaluate this if bl is a case block that falls through to another case block.
    // Otherwise there is a break statement in the flow
    if (bl->getType() != t_goto) // Fallthru must be a goto block
        return (FlowBlock*)0;
    int4 i;
    // Look for block to find flow after
    for (i = 0; i < caseblocks.size(); ++i)
        if (caseblocks[i].block == bl)
            break;
    if (i == caseblocks.size())
        return (FlowBlock*)0; // Didn't find block

    i = i + 1; // Blocks are printed in fallthru order, "flow" is to next block in this order
    if (i < caseblocks.size())
        return caseblocks[i].block->getFrontLeaf();
    // Otherwise we are at last block of switch, flow is to exit of switch
    if (getParent() == (const FlowBlock*)0)
        return (FlowBlock*)0;
    return getParent()->nextFlowAfter(this);
}

inline bool BlockSwitch::CaseOrder::compare(const CaseOrder& a, const CaseOrder& b)

{
    if (a.label != b.label)
        return (a.label < b.label);
    return (a.depth < b.depth);
}
} // namespace ghidra
