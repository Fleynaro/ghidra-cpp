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
export module sleigh_runtime.ghidra:slghpattern;
export import :context;
export import :slaformat;

export namespace ghidra {

class PatternBlock;
class Pattern;
class DisjointPattern;
class InstructionPattern;
class ContextPattern;
class CombinePattern;
class OrPattern;

bool resolveIntersectBlock(PatternBlock* bl1, PatternBlock* bl2, PatternBlock* thisblock);
DisjointPattern* decodeDisjointPattern(Decoder& decoder);
Pattern* instructionDoAnd(const InstructionPattern* self, const Pattern* b, int4 sa);
Pattern* instructionCommonSubPattern(const InstructionPattern* self, const Pattern* b, int4 sa);
Pattern* instructionDoOr(const InstructionPattern* self, const Pattern* b, int4 sa);
Pattern* orSimplifyClone(const OrPattern* self);

// A mask/value pair viewed as two bitstreams
class PatternBlock {
    int4 offset;           // Offset to non-zero byte of mask
    int4 nonzerosize;      // Last byte(+1) containing nonzero mask
    vector<uintm> maskvec; // Mask
    vector<uintm> valvec;  // Value

    void normalize(void) {
        if (nonzerosize <= 0) { // Check if alwaystrue or alwaysfalse
            offset = 0;         // in which case we don't need mask and value
            maskvec.clear();
            valvec.clear();
            return;
        }
        vector<uintm>::iterator iter1, iter2;

        iter1 = maskvec.begin(); // Cut zeros from beginning of mask
        iter2 = valvec.begin();
        while ((iter1 != maskvec.end()) && ((*iter1) == 0)) {
            iter1++;
            iter2++;
            offset += sizeof(uintm);
        }
        maskvec.erase(maskvec.begin(), iter1);
        valvec.erase(valvec.begin(), iter2);

        if (!maskvec.empty()) {
            int4 suboff = 0; // Cut off unaligned zeros from beginning of mask
            uintm tmp = maskvec[0];
            while (tmp != 0) {
                suboff += 1;
                tmp >>= 8;
            }
            suboff = sizeof(uintm) - suboff;
            if (suboff != 0) {
                offset += suboff; // Slide up maskvec by suboff bytes
                for (int4 i = 0; i < maskvec.size() - 1; ++i) {
                    tmp = maskvec[i] << (suboff * 8);
                    tmp |= (maskvec[i + 1] >> ((sizeof(uintm) - suboff) * 8));
                    maskvec[i] = tmp;
                }
                maskvec.back() <<= suboff * 8;
                for (int4 i = 0; i < valvec.size() - 1; ++i) { // Slide up valvec by suboff bytes
                    tmp = valvec[i] << (suboff * 8);
                    tmp |= (valvec[i + 1] >> ((sizeof(uintm) - suboff) * 8));
                    valvec[i] = tmp;
                }
                valvec.back() <<= suboff * 8;
            }

            iter1 = maskvec.end(); // Cut zeros from end of mask
            iter2 = valvec.end();
            while (iter1 != maskvec.begin()) {
                --iter1;
                --iter2;
                if ((*iter1) != 0)
                    break; // Find last non-zero
            }
            if (iter1 != maskvec.end()) {
                iter1++; // Find first zero, in last zero chain
                iter2++;
            }
            maskvec.erase(iter1, maskvec.end());
            valvec.erase(iter2, valvec.end());
        }

        if (maskvec.empty()) {
            offset = 0;
            nonzerosize = 0; // Always true
            return;
        }
        nonzerosize = maskvec.size() * sizeof(uintm);
        uintm tmp = maskvec.back(); // tmp must be nonzero
        while ((tmp & 0xff) == 0) {
            nonzerosize -= 1;
            tmp >>= 8;
        }
    }

public:
    PatternBlock(int4 off, uintm msk, uintm val) {
        // Define mask and value pattern, confined to one uintm
        offset = off;
        maskvec.push_back(msk);
        valvec.push_back(val);
        nonzerosize = sizeof(uintm); // Assume all non-zero bytes before normalization
        normalize();
    }

    PatternBlock(bool tf) {
        offset = 0;
        if (tf)
            nonzerosize = 0;
        else
            nonzerosize = -1;
    }

    PatternBlock(const PatternBlock* a, const PatternBlock* b) {
        // Construct PatternBlock by ANDing two others together
        PatternBlock* res = a->intersect(b);
        offset = res->offset;
        nonzerosize = res->nonzerosize;
        maskvec = res->maskvec;
        valvec = res->valvec;
        delete res;
    }

    PatternBlock(vector<PatternBlock*>& list) {
        // AND several blocks together to construct new block
        PatternBlock *res, *next;

        if (list.empty()) { // If not ANDing anything
            offset = 0;     // make constructed block always true
            nonzerosize = 0;
            return;
        }
        res = list[0];
        for (int4 i = 1; i < list.size(); ++i) {
            next = res->intersect(list[i]);
            delete res;
            res = next;
        }
        offset = res->offset;
        nonzerosize = res->nonzerosize;
        maskvec = res->maskvec;
        valvec = res->valvec;
        delete res;
    }

    PatternBlock* commonSubPattern(const PatternBlock* b) const {
        // The resulting pattern has a 1-bit in the mask
        // only if the two pieces have a 1-bit and the
        // values agree
        PatternBlock* res = new PatternBlock(true);
        int4 maxlength = (getLength() > b->getLength()) ? getLength() : b->getLength();

        res->offset = 0;
        int4 offset = 0;
        uintm mask1, val1, mask2, val2;
        uintm resmask, resval;
        while (offset < maxlength) {
            mask1 = getMask(offset * 8, sizeof(uintm) * 8);
            val1 = getValue(offset * 8, sizeof(uintm) * 8);
            mask2 = b->getMask(offset * 8, sizeof(uintm) * 8);
            val2 = b->getValue(offset * 8, sizeof(uintm) * 8);
            resmask = mask1 & mask2 & ~(val1 ^ val2);
            resval = val1 & val2 & resmask;
            res->maskvec.push_back(resmask);
            res->valvec.push_back(resval);
            offset += sizeof(uintm);
        }
        res->nonzerosize = maxlength;
        res->normalize();
        return res;
    }

    PatternBlock* intersect(const PatternBlock* b) const {
        // Construct the intersecting pattern
        if (alwaysFalse() || b->alwaysFalse())
            return new PatternBlock(false);
        PatternBlock* res = new PatternBlock(true);
        int4 maxlength = (getLength() > b->getLength()) ? getLength() : b->getLength();

        res->offset = 0;
        int4 offset = 0;
        uintm mask1, val1, mask2, val2, commonmask;
        uintm resmask, resval;
        while (offset < maxlength) {
            mask1 = getMask(offset * 8, sizeof(uintm) * 8);
            val1 = getValue(offset * 8, sizeof(uintm) * 8);
            mask2 = b->getMask(offset * 8, sizeof(uintm) * 8);
            val2 = b->getValue(offset * 8, sizeof(uintm) * 8);
            commonmask = mask1 & mask2; // Bits in mask shared by both patterns
            if ((commonmask & val1) != (commonmask & val2)) {
                res->nonzerosize = -1; // Impossible pattern
                res->normalize();
                return res;
            }
            resmask = mask1 | mask2;
            resval = (mask1 & val1) | (mask2 & val2);
            res->maskvec.push_back(resmask);
            res->valvec.push_back(resval);
            offset += sizeof(uintm);
        }
        res->nonzerosize = maxlength;
        res->normalize();
        return res;
    }

    bool specializes(const PatternBlock* op2) const {
        // does every masked bit in -this- match the corresponding
        // masked bit in -op2-
        int4 length = 8 * op2->getLength();
        int4 tmplength;
        uintm mask1, mask2, value1, value2;
        int4 sbit = 0;
        while (sbit < length) {
            tmplength = length - sbit;
            if (tmplength > 8 * sizeof(uintm))
                tmplength = 8 * sizeof(uintm);
            mask1 = getMask(sbit, tmplength);
            value1 = getValue(sbit, tmplength);
            mask2 = op2->getMask(sbit, tmplength);
            value2 = op2->getValue(sbit, tmplength);
            if ((mask1 & mask2) != mask2)
                return false;
            if ((value1 & mask2) != (value2 & mask2))
                return false;
            sbit += tmplength;
        }
        return true;
    }

    bool identical(const PatternBlock* op2) const {
        // Do the mask and value match exactly
        int4 tmplength;
        int4 length = 8 * op2->getLength();
        tmplength = 8 * getLength();
        if (tmplength > length)
            length = tmplength; // Maximum of two lengths
        uintm mask1, mask2, value1, value2;
        int4 sbit = 0;
        while (sbit < length) {
            tmplength = length - sbit;
            if (tmplength > 8 * sizeof(uintm))
                tmplength = 8 * sizeof(uintm);
            mask1 = getMask(sbit, tmplength);
            value1 = getValue(sbit, tmplength);
            mask2 = op2->getMask(sbit, tmplength);
            value2 = op2->getValue(sbit, tmplength);
            if (mask1 != mask2)
                return false;
            if ((mask1 & value1) != (mask2 & value2))
                return false;
            sbit += tmplength;
        }
        return true;
    }

    PatternBlock* clone(void) const {
        PatternBlock* res = new PatternBlock(true);

        res->offset = offset;
        res->nonzerosize = nonzerosize;
        res->maskvec = maskvec;
        res->valvec = valvec;
        return res;
    }

    void shift(int4 sa) {
        offset += sa;
        normalize();
    }

    int4 getLength(void) const {
        return offset + nonzerosize;
    }

    uintm getMask(int4 startbit, int4 size) const {
        startbit -= 8 * offset;
        // Note the division and remainder here is unsigned.  Then it is recast to signed.
        // If startbit is negative, then wordnum1 is either negative or very big,
        // if (unsigned size is same as sizeof int)
        // In either case, shift should come out between 0 and 8*sizeof(uintm)-1
        int4 wordnum1 = startbit / (8 * sizeof(uintm));
        int4 shift = startbit % (8 * sizeof(uintm));
        int4 wordnum2 = (startbit + size - 1) / (8 * sizeof(uintm));
        uintm res;

        if ((wordnum1 < 0) || (wordnum1 >= maskvec.size()))
            res = 0;
        else
            res = maskvec[wordnum1];

        res <<= shift;
        if (wordnum1 != wordnum2) {
            uintm tmp;
            if ((wordnum2 < 0) || (wordnum2 >= maskvec.size()))
                tmp = 0;
            else
                tmp = maskvec[wordnum2];
            res |= (tmp >> (8 * sizeof(uintm) - shift));
        }
        res >>= (8 * sizeof(uintm) - size);

        return res;
    }

    uintm getValue(int4 startbit, int4 size) const {
        startbit -= 8 * offset;
        int4 wordnum1 = startbit / (8 * sizeof(uintm));
        int4 shift = startbit % (8 * sizeof(uintm));
        int4 wordnum2 = (startbit + size - 1) / (8 * sizeof(uintm));
        uintm res;

        if ((wordnum1 < 0) || (wordnum1 >= valvec.size()))
            res = 0;
        else
            res = valvec[wordnum1];
        res <<= shift;
        if (wordnum1 != wordnum2) {
            uintm tmp;
            if ((wordnum2 < 0) || (wordnum2 >= valvec.size()))
                tmp = 0;
            else
                tmp = valvec[wordnum2];
            res |= (tmp >> (8 * sizeof(uintm) - shift));
        }
        res >>= (8 * sizeof(uintm) - size);

        return res;
    }

    bool alwaysTrue(void) const {
        return (nonzerosize == 0);
    }

    bool alwaysFalse(void) const {
        return (nonzerosize == -1);
    }

    bool isInstructionMatch(ParserWalker& walker) const {
        if (nonzerosize <= 0)
            return (nonzerosize == 0);
        int4 off = offset;
        for (int4 i = 0; i < maskvec.size(); ++i) {
            uintm data = walker.getInstructionBytes(off, sizeof(uintm));
            if ((maskvec[i] & data) != valvec[i])
                return false;
            off += sizeof(uintm);
        }
        return true;
    }

    bool isContextMatch(ParserWalker& walker) const {
        if (nonzerosize <= 0)
            return (nonzerosize == 0);
        int4 off = offset;
        for (int4 i = 0; i < maskvec.size(); ++i) {
            uintm data = walker.getContextBytes(off, sizeof(uintm));
            if ((maskvec[i] & data) != valvec[i])
                return false;
            off += sizeof(uintm);
        }
        return true;
    }

    void encode(Encoder& encoder) const {
        encoder.openElement(sla::ELEM_PAT_BLOCK);
        encoder.writeSignedInteger(sla::ATTRIB_OFF, offset);
        encoder.writeSignedInteger(sla::ATTRIB_NONZERO, nonzerosize);
        for (int4 i = 0; i < maskvec.size(); ++i) {
            encoder.openElement(sla::ELEM_MASK_WORD);
            encoder.writeUnsignedInteger(sla::ATTRIB_MASK, maskvec[i]);
            encoder.writeUnsignedInteger(sla::ATTRIB_VAL, valvec[i]);
            encoder.closeElement(sla::ELEM_MASK_WORD);
        }
        encoder.closeElement(sla::ELEM_PAT_BLOCK);
    }

    void decode(Decoder& decoder) {
        uint4 el = decoder.openElement(sla::ELEM_PAT_BLOCK);
        offset = decoder.readSignedInteger(sla::ATTRIB_OFF);
        nonzerosize = decoder.readSignedInteger(sla::ATTRIB_NONZERO);
        while (decoder.peekElement() != 0) {
            uint4 subel = decoder.openElement(sla::ELEM_MASK_WORD);
            uintm mask = decoder.readUnsignedInteger(sla::ATTRIB_MASK);
            uintm val = decoder.readUnsignedInteger(sla::ATTRIB_VAL);
            maskvec.push_back(mask);
            valvec.push_back(val);
            decoder.closeElement(subel);
        }
        normalize();
        decoder.closeElement(el);
    }
};

class DisjointPattern;
class Pattern {
public:
    virtual ~Pattern(void) {}
    virtual Pattern* simplifyClone(void) const = 0;
    virtual void shiftInstruction(int4 sa) = 0;
    virtual Pattern* doOr(const Pattern* b, int4 sa) const = 0;
    virtual Pattern* doAnd(const Pattern* b, int4 sa) const = 0;
    virtual Pattern* commonSubPattern(const Pattern* b, int4 sa) const = 0;
    virtual bool isMatch(ParserWalker& walker) const = 0; // Does this pattern match context
    virtual int4 numDisjoint(void) const = 0;
    virtual DisjointPattern* getDisjoint(int4 i) const = 0;
    virtual bool alwaysTrue(void) const = 0;
    virtual bool alwaysFalse(void) const = 0;
    virtual bool alwaysInstructionTrue(void) const = 0;
    virtual void encode(Encoder& encoder) const = 0;
    virtual void decode(Decoder& decoder) = 0;
};

class DisjointPattern : public Pattern { // A pattern with no ORs in it
    virtual PatternBlock* getBlock(bool context) const = 0;

public:
    virtual int4 numDisjoint(void) const {
        return 0;
    }
    virtual DisjointPattern* getDisjoint(int4 i) const {
        return (DisjointPattern*)0;
    }
    uintm getMask(int4 startbit, int4 size, bool context) const {
        PatternBlock* block = getBlock(context);
        if (block != (PatternBlock*)0)
            return block->getMask(startbit, size);
        return 0;
    }
    uintm getValue(int4 startbit, int4 size, bool context) const {
        PatternBlock* block = getBlock(context);
        if (block != (PatternBlock*)0)
            return block->getValue(startbit, size);
        return 0;
    }
    int4 getLength(bool context) const {
        PatternBlock* block = getBlock(context);
        if (block != (PatternBlock*)0)
            return block->getLength();
        return 0;
    }
    bool specializes(const DisjointPattern* op2) const {
        // Return true, if everywhere this's mask is non-zero
        // op2's mask is non-zero and op2's value match this's
        PatternBlock *a, *b;

        a = getBlock(false);
        b = op2->getBlock(false);
        if ((b != (PatternBlock*)0) && (!b->alwaysTrue())) { // a must match existing block
            if (a == (PatternBlock*)0)
                return false;
            if (!a->specializes(b))
                return false;
        }
        a = getBlock(true);
        b = op2->getBlock(true);
        if ((b != (PatternBlock*)0) && (!b->alwaysTrue())) { // a must match existing block
            if (a == (PatternBlock*)0)
                return false;
            if (!a->specializes(b))
                return false;
        }
        return true;
    }
    bool identical(const DisjointPattern* op2) const {
        // Return true if patterns match exactly
        PatternBlock *a, *b;

        a = getBlock(false);
        b = op2->getBlock(false);
        if (b != (PatternBlock*)0) { // a must match existing block
            if (a == (PatternBlock*)0) {
                if (!b->alwaysTrue())
                    return false;
            } else if (!a->identical(b))
                return false;
        } else {
            if ((a != (PatternBlock*)0) && (!a->alwaysTrue()))
                return false;
        }
        a = getBlock(true);
        b = op2->getBlock(true);
        if (b != (PatternBlock*)0) { // a must match existing block
            if (a == (PatternBlock*)0) {
                if (!b->alwaysTrue())
                    return false;
            } else if (!a->identical(b))
                return false;
        } else {
            if ((a != (PatternBlock*)0) && (!a->alwaysTrue()))
                return false;
        }
        return true;
    }
    bool resolvesIntersect(const DisjointPattern* op1, const DisjointPattern* op2) const {
        // Is this pattern equal to the intersection of -op1- and -op2-
        if (!resolveIntersectBlock(op1->getBlock(false), op2->getBlock(false), getBlock(false)))
            return false;
        return resolveIntersectBlock(op1->getBlock(true), op2->getBlock(true), getBlock(true));
    }
    static DisjointPattern* decodeDisjoint(Decoder& decoder) {
        // DisjointPattern factory
        return decodeDisjointPattern(decoder);
    }
};

class OrPattern : public Pattern {
    vector<DisjointPattern*> orlist;

public:
    OrPattern(void) {} // For use with decode
    OrPattern(DisjointPattern* a, DisjointPattern* b) {
        orlist.push_back(a);
        orlist.push_back(b);
    }
    OrPattern(const vector<DisjointPattern*>& list) {
        vector<DisjointPattern*>::const_iterator iter;

        for (iter = list.begin(); iter != list.end(); ++iter)
            orlist.push_back(*iter);
    }
    virtual ~OrPattern(void) {
        vector<DisjointPattern*>::iterator iter;

        for (iter = orlist.begin(); iter != orlist.end(); ++iter)
            delete *iter;
    }
    virtual Pattern* simplifyClone(void) const {
        return orSimplifyClone(this);
    }
    virtual void shiftInstruction(int4 sa) {
        vector<DisjointPattern*>::iterator iter;

        for (iter = orlist.begin(); iter != orlist.end(); ++iter)
            (*iter)->shiftInstruction(sa);
    }
    virtual bool isMatch(ParserWalker& walker) const {
        for (int4 i = 0; i < orlist.size(); ++i)
            if (orlist[i]->isMatch(walker))
                return true;
        return false;
    }
    virtual int4 numDisjoint(void) const {
        return orlist.size();
    }
    virtual DisjointPattern* getDisjoint(int4 i) const {
        return orlist[i];
    }
    virtual bool alwaysTrue(void) const {
        // This isn't quite right because different branches
        // may cover the entire gamut
        vector<DisjointPattern*>::const_iterator iter;

        for (iter = orlist.begin(); iter != orlist.end(); ++iter)
            if ((*iter)->alwaysTrue())
                return true;
        return false;
    }
    virtual bool alwaysFalse(void) const {
        vector<DisjointPattern*>::const_iterator iter;

        for (iter = orlist.begin(); iter != orlist.end(); ++iter)
            if (!(*iter)->alwaysFalse())
                return false;
        return true;
    }
    virtual bool alwaysInstructionTrue(void) const {
        vector<DisjointPattern*>::const_iterator iter;

        for (iter = orlist.begin(); iter != orlist.end(); ++iter)
            if (!(*iter)->alwaysInstructionTrue())
                return false;
        return true;
    }
    virtual Pattern* doAnd(const Pattern* b, int4 sa) const {
        const OrPattern* b2 = dynamic_cast<const OrPattern*>(b);
        vector<DisjointPattern*> newlist;
        vector<DisjointPattern*>::const_iterator iter, iter2;
        DisjointPattern* tmp;
        OrPattern* tmpor;

        if (b2 == (const OrPattern*)0) {
            for (iter = orlist.begin(); iter != orlist.end(); ++iter) {
                tmp = (DisjointPattern*)(*iter)->doAnd(b, sa);
                newlist.push_back(tmp);
            }
        } else {
            for (iter = orlist.begin(); iter != orlist.end(); ++iter)
                for (iter2 = b2->orlist.begin(); iter2 != b2->orlist.end(); ++iter2) {
                    tmp = (DisjointPattern*)(*iter)->doAnd(*iter2, sa);
                    newlist.push_back(tmp);
                }
        }
        tmpor = new OrPattern(newlist);
        return tmpor;
    }
    virtual Pattern* commonSubPattern(const Pattern* b, int4 sa) const {
        vector<DisjointPattern*>::const_iterator iter;
        Pattern *res, *next;

        iter = orlist.begin();
        res = (*iter)->commonSubPattern(b, sa);
        iter++;

        if (sa > 0)
            sa = 0;
        while (iter != orlist.end()) {
            next = (*iter)->commonSubPattern(res, sa);
            delete res;
            res = next;
            ++iter;
        }
        return res;
    }
    virtual Pattern* doOr(const Pattern* b, int4 sa) const {
        const OrPattern* b2 = dynamic_cast<const OrPattern*>(b);
        vector<DisjointPattern*> newlist;
        vector<DisjointPattern*>::const_iterator iter;

        for (iter = orlist.begin(); iter != orlist.end(); ++iter)
            newlist.push_back((DisjointPattern*)(*iter)->simplifyClone());
        if (sa < 0)
            for (iter = orlist.begin(); iter != orlist.end(); ++iter)
                (*iter)->shiftInstruction(-sa);

        if (b2 == (const OrPattern*)0)
            newlist.push_back((DisjointPattern*)b->simplifyClone());
        else {
            for (iter = b2->orlist.begin(); iter != b2->orlist.end(); ++iter)
                newlist.push_back((DisjointPattern*)(*iter)->simplifyClone());
        }
        if (sa > 0)
            for (int4 i = 0; i < newlist.size(); ++i)
                newlist[i]->shiftInstruction(sa);

        OrPattern* tmpor = new OrPattern(newlist);
        return tmpor;
    }
    virtual void encode(Encoder& encoder) const {
        encoder.openElement(sla::ELEM_OR_PAT);
        for (int4 i = 0; i < orlist.size(); ++i)
            orlist[i]->encode(encoder);
        encoder.closeElement(sla::ELEM_OR_PAT);
    }
    virtual void decode(Decoder& decoder) {
        uint4 el = decoder.openElement(sla::ELEM_OR_PAT);
        while (decoder.peekElement() != 0) {
            DisjointPattern* pat = DisjointPattern::decodeDisjoint(decoder);
            orlist.push_back(pat);
        }
        decoder.closeElement(el);
    }
};

class InstructionPattern : public DisjointPattern { // Matches the instruction bitstream
    PatternBlock* maskvalue;
    virtual PatternBlock* getBlock(bool context) const {
        return context ? (PatternBlock*)0 : maskvalue;
    }

public:
    InstructionPattern(void) {
        maskvalue = (PatternBlock*)0;
    } // For use with decode
    InstructionPattern(PatternBlock* mv) {
        maskvalue = mv;
    }
    InstructionPattern(bool tf) {
        maskvalue = new PatternBlock(tf);
    }
    PatternBlock* getBlock(void) {
        return maskvalue;
    }
    virtual ~InstructionPattern(void) {
        if (maskvalue != (PatternBlock*)0)
            delete maskvalue;
    }
    virtual Pattern* simplifyClone(void) const {
        return new InstructionPattern(maskvalue->clone());
    }
    virtual void shiftInstruction(int4 sa) {
        maskvalue->shift(sa);
    }
    virtual Pattern* doOr(const Pattern* b, int4 sa) const {
        return instructionDoOr(this, b, sa);
    }
    virtual Pattern* doAnd(const Pattern* b, int4 sa) const {
        return instructionDoAnd(this, b, sa);
    }
    virtual Pattern* commonSubPattern(const Pattern* b, int4 sa) const {
        return instructionCommonSubPattern(this, b, sa);
    }
    virtual bool isMatch(ParserWalker& walker) const {
        return maskvalue->isInstructionMatch(walker);
    }
    virtual bool alwaysTrue(void) const {
        return maskvalue->alwaysTrue();
    }
    virtual bool alwaysFalse(void) const {
        return maskvalue->alwaysFalse();
    }
    virtual bool alwaysInstructionTrue(void) const {
        return maskvalue->alwaysTrue();
    }
    virtual void encode(Encoder& encoder) const {
        encoder.openElement(sla::ELEM_INSTRUCT_PAT);
        maskvalue->encode(encoder);
        encoder.closeElement(sla::ELEM_INSTRUCT_PAT);
    }
    virtual void decode(Decoder& decoder) {
        uint4 el = decoder.openElement(sla::ELEM_INSTRUCT_PAT);
        maskvalue = new PatternBlock(true);
        maskvalue->decode(decoder);
        decoder.closeElement(el);
    }
};

class ContextPattern : public DisjointPattern { // Matches the context bitstream
    PatternBlock* maskvalue;
    virtual PatternBlock* getBlock(bool context) const {
        return context ? maskvalue : (PatternBlock*)0;
    }

public:
    ContextPattern(void) {
        maskvalue = (PatternBlock*)0;
    } // For use with decode
    ContextPattern(PatternBlock* mv) {
        maskvalue = mv;
    }
    PatternBlock* getBlock(void) {
        return maskvalue;
    }
    virtual ~ContextPattern(void) {
        if (maskvalue != (PatternBlock*)0)
            delete maskvalue;
    }
    virtual Pattern* simplifyClone(void) const {
        return new ContextPattern(maskvalue->clone());
    }
    virtual void shiftInstruction(int4 sa) {} // do nothing
    virtual Pattern* doOr(const Pattern* b, int4 sa) const {
        const ContextPattern* b2 = dynamic_cast<const ContextPattern*>(b);
        if (b2 == (const ContextPattern*)0)
            return b->doOr(this, -sa);

        return new OrPattern((DisjointPattern*)simplifyClone(), (DisjointPattern*)b2->simplifyClone());
    }
    virtual Pattern* doAnd(const Pattern* b, int4 sa) const {
        const ContextPattern* b2 = dynamic_cast<const ContextPattern*>(b);
        if (b2 == (const ContextPattern*)0)
            return b->doAnd(this, -sa);

        PatternBlock* resblock = maskvalue->intersect(b2->maskvalue);
        return new ContextPattern(resblock);
    }
    virtual Pattern* commonSubPattern(const Pattern* b, int4 sa) const {
        const ContextPattern* b2 = dynamic_cast<const ContextPattern*>(b);
        if (b2 == (const ContextPattern*)0)
            return b->commonSubPattern(this, -sa);

        PatternBlock* resblock = maskvalue->commonSubPattern(b2->maskvalue);
        return new ContextPattern(resblock);
    }
    virtual bool isMatch(ParserWalker& walker) const {
        return maskvalue->isContextMatch(walker);
    }
    virtual bool alwaysTrue(void) const {
        return maskvalue->alwaysTrue();
    }
    virtual bool alwaysFalse(void) const {
        return maskvalue->alwaysFalse();
    }
    virtual bool alwaysInstructionTrue(void) const {
        return true;
    }
    virtual void encode(Encoder& encoder) const {
        encoder.openElement(sla::ELEM_CONTEXT_PAT);
        maskvalue->encode(encoder);
        encoder.closeElement(sla::ELEM_CONTEXT_PAT);
    }
    virtual void decode(Decoder& decoder) {
        uint4 el = decoder.openElement(sla::ELEM_CONTEXT_PAT);
        maskvalue = new PatternBlock(true);
        maskvalue->decode(decoder);
        decoder.closeElement(el);
    }
};

// A pattern with a context piece and an instruction piece
class CombinePattern : public DisjointPattern {
    ContextPattern* context;   // Context piece
    InstructionPattern* instr; // Instruction piece
    virtual PatternBlock* getBlock(bool cont) const {
        return cont ? context->getBlock() : instr->getBlock();
    }

public:
    CombinePattern(void) {
        context = (ContextPattern*)0;
        instr = (InstructionPattern*)0;
    }
    CombinePattern(ContextPattern* con, InstructionPattern* in) {
        context = con;
        instr = in;
    }
    virtual ~CombinePattern(void) {
        if (context != (ContextPattern*)0)
            delete context;
        if (instr != (InstructionPattern*)0)
            delete instr;
    }
    virtual Pattern* simplifyClone(void) const {
        // We should only have to think at "our" level
        if (context->alwaysTrue())
            return instr->simplifyClone();
        if (instr->alwaysTrue())
            return context->simplifyClone();
        if (context->alwaysFalse() || instr->alwaysFalse())
            return new InstructionPattern(false);
        return new CombinePattern((ContextPattern*)context->simplifyClone(),
                                  (InstructionPattern*)instr->simplifyClone());
    }
    virtual void shiftInstruction(int4 sa) {
        instr->shiftInstruction(sa);
    }
    virtual bool isMatch(ParserWalker& walker) const {
        if (!instr->isMatch(walker))
            return false;
        if (!context->isMatch(walker))
            return false;
        return true;
    }
    virtual bool alwaysTrue(void) const {
        return (context->alwaysTrue() && instr->alwaysTrue());
    }
    virtual bool alwaysFalse(void) const {
        return (context->alwaysFalse() || instr->alwaysFalse());
    }
    virtual bool alwaysInstructionTrue(void) const {
        return instr->alwaysInstructionTrue();
    }
    virtual Pattern* doOr(const Pattern* b, int4 sa) const {
        if (b->numDisjoint() != 0)
            return b->doOr(this, -sa);

        DisjointPattern* res1 = (DisjointPattern*)simplifyClone();
        DisjointPattern* res2 = (DisjointPattern*)b->simplifyClone();
        if (sa < 0)
            res1->shiftInstruction(-sa);
        else
            res2->shiftInstruction(sa);
        OrPattern* tmp = new OrPattern(res1, res2);
        return tmp;
    }
    virtual Pattern* doAnd(const Pattern* b, int4 sa) const {
        CombinePattern* tmp;

        if (b->numDisjoint() != 0)
            return b->doAnd(this, -sa);

        const CombinePattern* b2 = dynamic_cast<const CombinePattern*>(b);
        if (b2 != (CombinePattern*)0) {
            ContextPattern* c = (ContextPattern*)context->doAnd(b2->context, 0);
            InstructionPattern* i = (InstructionPattern*)instr->doAnd(b2->instr, sa);
            tmp = new CombinePattern(c, i);
        } else {
            const InstructionPattern* b3 = dynamic_cast<const InstructionPattern*>(b);
            if (b3 != (const InstructionPattern*)0) {
                InstructionPattern* i = (InstructionPattern*)instr->doAnd(b3, sa);
                tmp = new CombinePattern((ContextPattern*)context->simplifyClone(), i);
            } else { // Must be a ContextPattern
                ContextPattern* c = (ContextPattern*)context->doAnd(b, 0);
                InstructionPattern* newpat = (InstructionPattern*)instr->simplifyClone();
                if (sa < 0)
                    newpat->shiftInstruction(-sa);
                tmp = new CombinePattern(c, newpat);
            }
        }
        return tmp;
    }
    virtual Pattern* commonSubPattern(const Pattern* b, int4 sa) const {
        Pattern* tmp;

        if (b->numDisjoint() != 0)
            return b->commonSubPattern(this, -sa);

        const CombinePattern* b2 = dynamic_cast<const CombinePattern*>(b);
        if (b2 != (CombinePattern*)0) {
            ContextPattern* c = (ContextPattern*)context->commonSubPattern(b2->context, 0);
            InstructionPattern* i = (InstructionPattern*)instr->commonSubPattern(b2->instr, sa);
            tmp = new CombinePattern(c, i);
        } else {
            const InstructionPattern* b3 = dynamic_cast<const InstructionPattern*>(b);
            if (b3 != (const InstructionPattern*)0)
                tmp = instr->commonSubPattern(b3, sa);
            else // Must be a ContextPattern
                tmp = context->commonSubPattern(b, 0);
        }
        return tmp;
    }
    virtual void encode(Encoder& encoder) const {
        encoder.openElement(sla::ELEM_COMBINE_PAT);
        context->encode(encoder);
        instr->encode(encoder);
        encoder.closeElement(sla::ELEM_COMBINE_PAT);
    }
    virtual void decode(Decoder& decoder) {
        uint4 el = decoder.openElement(sla::ELEM_COMBINE_PAT);
        context = new ContextPattern();
        context->decode(decoder);
        instr = new InstructionPattern();
        instr->decode(decoder);
        decoder.closeElement(el);
    }
};

inline bool resolveIntersectBlock(PatternBlock* bl1, PatternBlock* bl2, PatternBlock* thisblock) {
    PatternBlock* inter;
    bool allocated = false;
    bool res = true;

    if (bl1 == (PatternBlock*)0)
        inter = bl2;
    else if (bl2 == (PatternBlock*)0)
        inter = bl1;
    else {
        allocated = true;
        inter = bl1->intersect(bl2);
    }
    if (inter == (PatternBlock*)0) {
        if (thisblock != (PatternBlock*)0)
            res = false;
    } else if (thisblock == (PatternBlock*)0)
        res = false;
    else
        res = thisblock->identical(inter);
    if (allocated)
        delete inter;
    return res;
}

DisjointPattern* decodeDisjointPattern(Decoder& decoder) {
    // DisjointPattern factory
    unique_ptr<DisjointPattern> res;
    uint4 el = decoder.peekElement();
    if (el == sla::ELEM_INSTRUCT_PAT)
        res.reset(new InstructionPattern());
    else if (el == sla::ELEM_CONTEXT_PAT)
        res.reset(new ContextPattern());
    else
        res.reset(new CombinePattern());
    res->decode(decoder);
    return res.release();
}

inline Pattern* instructionDoAnd(const InstructionPattern* self, const Pattern* b, int4 sa) {
    if (b->numDisjoint() > 0)
        return b->doAnd(self, -sa);

    const CombinePattern* b2 = dynamic_cast<const CombinePattern*>(b);
    if (b2 != (const CombinePattern*)0)
        return b->doAnd(self, -sa);

    const ContextPattern* b3 = dynamic_cast<const ContextPattern*>(b);
    if (b3 != (const ContextPattern*)0) {
        InstructionPattern* newpat = (InstructionPattern*)self->simplifyClone();
        if (sa < 0)
            newpat->shiftInstruction(-sa);
        return new CombinePattern((ContextPattern*)b3->simplifyClone(), newpat);
    }
    const InstructionPattern* b4 = (const InstructionPattern*)b;

    PatternBlock* respattern;
    if (sa < 0) {
        PatternBlock* a = const_cast<InstructionPattern*>(self)->getBlock()->clone();
        a->shift(-sa);
        respattern = a->intersect(const_cast<InstructionPattern*>(b4)->getBlock());
        delete a;
    } else {
        PatternBlock* c = const_cast<InstructionPattern*>(b4)->getBlock()->clone();
        c->shift(sa);
        respattern = const_cast<InstructionPattern*>(self)->getBlock()->intersect(c);
        delete c;
    }
    return new InstructionPattern(respattern);
}

inline Pattern* instructionCommonSubPattern(const InstructionPattern* self, const Pattern* b, int4 sa) {
    if (b->numDisjoint() > 0)
        return b->commonSubPattern(self, -sa);

    const CombinePattern* b2 = dynamic_cast<const CombinePattern*>(b);
    if (b2 != (const CombinePattern*)0)
        return b->commonSubPattern(self, -sa);

    const ContextPattern* b3 = dynamic_cast<const ContextPattern*>(b);
    if (b3 != (const ContextPattern*)0) {
        InstructionPattern* res = new InstructionPattern(true);
        return res;
    }
    const InstructionPattern* b4 = (const InstructionPattern*)b;

    PatternBlock* respattern;
    if (sa < 0) {
        PatternBlock* a = const_cast<InstructionPattern*>(self)->getBlock()->clone();
        a->shift(-sa);
        respattern = a->commonSubPattern(const_cast<InstructionPattern*>(b4)->getBlock());
        delete a;
    } else {
        PatternBlock* c = const_cast<InstructionPattern*>(b4)->getBlock()->clone();
        c->shift(sa);
        respattern = const_cast<InstructionPattern*>(self)->getBlock()->commonSubPattern(c);
        delete c;
    }
    return new InstructionPattern(respattern);
}

inline Pattern* instructionDoOr(const InstructionPattern* self, const Pattern* b, int4 sa) {
    if (b->numDisjoint() > 0)
        return b->doOr(self, -sa);

    const CombinePattern* b2 = dynamic_cast<const CombinePattern*>(b);
    if (b2 != (const CombinePattern*)0)
        return b->doOr(self, -sa);

    DisjointPattern *res1, *res2;
    res1 = (DisjointPattern*)self->simplifyClone();
    res2 = (DisjointPattern*)b->simplifyClone();
    if (sa < 0)
        res1->shiftInstruction(-sa);
    else
        res2->shiftInstruction(sa);
    return new OrPattern(res1, res2);
}

inline Pattern* orSimplifyClone(const OrPattern* self) {
    // Look for alwaysTrue eliminate alwaysFalse
    for (int4 i = 0; i < self->numDisjoint(); ++i) // Look for alwaysTrue
        if (self->getDisjoint(i)->alwaysTrue())
            return new InstructionPattern(true);

    vector<DisjointPattern*> newlist;
    for (int4 i = 0; i < self->numDisjoint(); ++i) // Look for alwaysFalse
        if (!self->getDisjoint(i)->alwaysFalse())
            newlist.push_back((DisjointPattern*)self->getDisjoint(i)->simplifyClone());

    if (newlist.empty())
        return new InstructionPattern(false);
    else if (newlist.size() == 1)
        return newlist[0];
    return new OrPattern(newlist);
}

} // End namespace ghidra
