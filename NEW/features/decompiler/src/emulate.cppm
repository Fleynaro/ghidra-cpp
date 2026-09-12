module ghidra.decompiler;
import std;

// Port provenance: mechanically copied from Ghidra/Features/Decompiler/src/decompile/cpp/emulate.cc.
// The implementation is preserved from the original native decompiler source.
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

namespace ghidra {

/// Provide the emitter with the containers that will hold the cached p-code ops and varnodes.
/// \param ocache is the container for cached PcodeOpRaw
/// \param vcache is the container for cached VarnodeData
/// \param in is the map of OpBehavior
/// \param uniqReserve is the starting offset for temporaries in the \e unique space
PcodeEmitCache::PcodeEmitCache(vector<PcodeOpRaw*>& ocache, vector<VarnodeData*>& vcache, const vector<OpBehavior*>& in,
                               uintb uniqReserve)
    : opcache(ocache), varcache(vcache), inst(in) {
    uniq = uniqReserve;
}

/// Create an internal copy of the VarnodeData and cache it.
/// \param var is the incoming VarnodeData being dumped
/// \return the cloned VarnodeData
VarnodeData* PcodeEmitCache::createVarnode(const VarnodeData* var)

{
    VarnodeData* res = new VarnodeData();
    *res = *var;
    varcache.push_back(res);
    return res;
}

void PcodeEmitCache::dump(const Address& addr, OpCode opc, VarnodeData* outvar, VarnodeData* vars, int4 isize)

{
    PcodeOpRaw* op = new PcodeOpRaw();
    op->setSeqNum(addr, uniq);
    opcache.push_back(op);
    op->setBehavior(inst[opc]);
    uniq += 1;
    if (outvar != (VarnodeData*)0) {
        VarnodeData* outvn = createVarnode(outvar);
        op->setOutput(outvn);
    }
    for (int4 i = 0; i < isize; ++i) {
        VarnodeData* invn = createVarnode(vars + i);
        op->addInput(invn);
    }
}

/// This method executes a single pcode operation, the current one (returned by getCurrentOp()).
/// The MemoryState of the emulator is queried and changed as needed to accomplish this.
void Emulate::executeCurrentOp(void)

{
    if (currentBehave == (OpBehavior*)0) { // Presumably a NO-OP
        fallthruOp();
        return;
    }
    if (currentBehave->isSpecial()) {
        switch (currentBehave->getOpcode()) {
            case CPUI_LOAD:
                executeLoad();
                fallthruOp();
                break;
            case CPUI_STORE:
                executeStore();
                fallthruOp();
                break;
            case CPUI_BRANCH:
                executeBranch();
                break;
            case CPUI_CBRANCH:
                if (executeCbranch())
                    executeBranch();
                else
                    fallthruOp();
                break;
            case CPUI_BRANCHIND:
                executeBranchind();
                break;
            case CPUI_CALL:
                executeCall();
                break;
            case CPUI_CALLIND:
                executeCallind();
                break;
            case CPUI_CALLOTHER:
                executeCallother();
                break;
            case CPUI_RETURN:
                executeBranchind();
                break;
            case CPUI_MULTIEQUAL:
                executeMultiequal();
                fallthruOp();
                break;
            case CPUI_INDIRECT:
                executeIndirect();
                fallthruOp();
                break;
            case CPUI_SEGMENTOP:
                executeSegmentOp();
                fallthruOp();
                break;
            case CPUI_CPOOLREF:
                executeCpoolRef();
                fallthruOp();
                break;
            case CPUI_NEW:
                executeNew();
                fallthruOp();
                break;
            default:
                throw LowlevelError("Bad special op");
        }
    } else if (currentBehave->isUnary()) { // Unary operation
        executeUnary();
        fallthruOp();
    } else { // Binary operation
        executeBinary();
        fallthruOp(); // All binary ops are fallthrus
    }
}

} // End namespace ghidra
