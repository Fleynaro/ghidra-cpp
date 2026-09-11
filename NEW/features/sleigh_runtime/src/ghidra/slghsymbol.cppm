module;
#include <cmath>
#include <ostream>
#include <sstream>
#include <string>

#include <cmath>

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
#ifndef __SLGHSYMBOL_HH__
#define __SLGHSYMBOL_HH__

#include <ostream>
#include <string>

export module sleigh_runtime.ghidra:slghsymbol;
#include <ostream>
#include <string>

export import :semantics;
export import :slghpatexpress;

export namespace ghidra {

using std::log;

class SleighBase; // Forward declaration
class SleighSymbol;
class TripleSymbol;
class Constructor;
class SubtableSymbol;
class SymbolTable;
SleighSymbol* sleighBaseFindSymbol(SleighBase* base, uintm id);
AddrSpace* sleighBaseGetConstantSpace(SleighBase* base);
PatternExpression* sleighBaseDecodeExpression(Decoder& decoder, SleighBase* base);
void parserWalkerPrintConstructor(ParserWalker& walker, ostream& stream);
bool constructorTrySubtablePattern(TripleSymbol* triple, ostream& stream, vector<TokenPattern>& patterns, bool& recursion);
bool constructorTryPrintMnemonic(const Constructor& constructor, ostream& stream, ParserWalker& walker);
bool constructorTryPrintBody(const Constructor& constructor, ostream& stream, ParserWalker& walker);
int4 subtableGetNumConstructors(const SubtableSymbol* subtable);
Constructor* subtableGetConstructor(const SubtableSymbol* subtable, int4 index);
const string& subtableGetName(const SubtableSymbol* subtable);
uintm subtableGetId(const SubtableSymbol* subtable);
bool constructorIsRecursive(const Constructor& constructor);
void symbolTableDecodeSymbolHeader(SymbolTable& table, Decoder& decoder);
void symbolTablePurge(SymbolTable& table);
class SleighSymbol {
    friend class SymbolTable;
    friend void symbolTableDecodeSymbolHeader(SymbolTable&, Decoder&);
    friend void symbolTablePurge(SymbolTable&);

public:
    enum symbol_type {
        space_symbol,
        token_symbol,
        userop_symbol,
        value_symbol,
        valuemap_symbol,
        name_symbol,
        varnode_symbol,
        varnodelist_symbol,
        operand_symbol,
        start_symbol,
        end_symbol,
        next2_symbol,
        subtable_symbol,
        macro_symbol,
        section_symbol,
        bitrange_symbol,
        context_symbol,
        epsilon_symbol,
        label_symbol,
        flowdest_symbol,
        flowref_symbol,
        dummy_symbol
    };

private:
    string name;
    uintm id;      // Unique id across all symbols
    uintm scopeid; // Unique id of scope this symbol is in
public:
    SleighSymbol(void) {} // For use with decode
    SleighSymbol(const string& nm) {
        name = nm;
        id = 0;
    }
    virtual ~SleighSymbol(void) {}
    const string& getName(void) const {
        return name;
    }
    uintm getId(void) const {
        return id;
    }
    virtual symbol_type getType(void) const {
        return dummy_symbol;
    }
    virtual void encodeHeader(Encoder& encoder) const
{ // Save the basic attributes of a symbol
    encoder.writeString(sla::ATTRIB_NAME, name);
    encoder.writeUnsignedInteger(sla::ATTRIB_ID, id);
    encoder.writeUnsignedInteger(sla::ATTRIB_SCOPE, scopeid);
}
    void decodeHeader(Decoder& decoder)
{
    uint4 el = decoder.openElement();
    name = decoder.readString(sla::ATTRIB_NAME);
    id = decoder.readUnsignedInteger(sla::ATTRIB_ID);
    scopeid = decoder.readUnsignedInteger(sla::ATTRIB_SCOPE);
    decoder.closeElement(el);
}
    virtual void encode(Encoder& encoder) const
{
    throw LowlevelError("Symbol " + name + " cannot be encoded to stream directly");
}
    virtual void decode(Decoder& decoder, SleighBase* trans)
{
    throw LowlevelError("Symbol " + name + " cannot be decoded from stream directly");
}
};

struct SymbolCompare {
    bool operator()(const SleighSymbol* a, const SleighSymbol* b) const {
        return (a->getName() < b->getName());
    }
};

typedef set<SleighSymbol*, SymbolCompare> SymbolTree;
class SymbolScope {
    friend class SymbolTable;
    friend void symbolTablePurge(SymbolTable&);
    SymbolScope* parent;
    SymbolTree tree;
    uintm id;

public:
    SymbolScope(SymbolScope* p, uintm i) {
        parent = p;
        id = i;
    }
    SymbolScope* getParent(void) const {
        return parent;
    }
    SleighSymbol* addSymbol(SleighSymbol* a)
{
    pair<SymbolTree::iterator, bool> res;

    res = tree.insert(a);
    if (!res.second)
        return *res.first; // Symbol already exists in this table
    return a;
}
    SleighSymbol* findSymbol(const string& nm) const
{
    SleighSymbol dummy(nm);
    SymbolTree::const_iterator iter;

    iter = tree.find(&dummy);
    if (iter != tree.end())
        return *iter;
    return (SleighSymbol*)0;
}
    SymbolTree::const_iterator begin(void) const {
        return tree.begin();
    }
    SymbolTree::const_iterator end(void) const {
        return tree.end();
    }
    uintm getId(void) const {
        return id;
    }
    void removeSymbol(SleighSymbol* a) {
        tree.erase(a);
    }
};

class SymbolTable {
    friend void symbolTableDecodeSymbolHeader(SymbolTable&, Decoder&);
    friend void symbolTablePurge(SymbolTable&);
    vector<SleighSymbol*> symbollist;
    vector<SymbolScope*> table;
    SymbolScope* curscope;
    SymbolScope* skipScope(int4 i) const
{
    SymbolScope* res = curscope;
    while (i > 0) {
        if (res->parent == (SymbolScope*)0)
            return res;
        res = res->parent;
        --i;
    }
    return res;
}
    SleighSymbol* findSymbolInternal(SymbolScope* scope, const string& nm) const
{
    SleighSymbol* res;

    while (scope != (SymbolScope*)0) {
        res = scope->findSymbol(nm);
        if (res != (SleighSymbol*)0)
            return res;
        scope = scope->getParent(); // Try higher scope
    }
    return (SleighSymbol*)0;
}
    void renumber(void)
{ // Renumber all the scopes and symbols
  // so that there are no gaps
    vector<SymbolScope*> newtable;
    vector<SleighSymbol*> newsymbol;
    // First renumber the scopes
    SymbolScope* scope;
    for (int4 i = 0; i < table.size(); ++i) {
        scope = table[i];
        if (scope != (SymbolScope*)0) {
            scope->id = newtable.size();
            newtable.push_back(scope);
        }
    }
    // Now renumber the symbols
    SleighSymbol* sym;
    for (int4 i = 0; i < symbollist.size(); ++i) {
        sym = symbollist[i];
        if (sym != (SleighSymbol*)0) {
            sym->scopeid = table[sym->scopeid]->id;
            sym->id = newsymbol.size();
            newsymbol.push_back(sym);
        }
    }
    table = newtable;
    symbollist = newsymbol;
}
    static constexpr int8 MAX_TABLES = 0x100000;
    static constexpr int8 MAX_SYMBOLS = 0x1000000;

public:
    SymbolTable(void) {
        curscope = (SymbolScope*)0;
    }
    ~SymbolTable(void)
{
    vector<SymbolScope*>::iterator iter;
    for (iter = table.begin(); iter != table.end(); ++iter)
        if (*iter)
            delete *iter;
    vector<SleighSymbol*>::iterator siter;
    for (siter = symbollist.begin(); siter != symbollist.end(); ++siter)
        if (*siter)
            delete *siter;
}
    SymbolScope* getCurrentScope(void) {
        return curscope;
    }
    SymbolScope* getGlobalScope(void) {
        return table[0];
    }

    void setCurrentScope(SymbolScope* scope) {
        curscope = scope;
    }
    void addScope(void)
{
    curscope = new SymbolScope(curscope, table.size());
    table.push_back(curscope);
} // Add new scope off of current scope, make it current
    void popScope(void)
{
    if (curscope != (SymbolScope*)0)
        curscope = curscope->getParent();
} // Make parent of current scope current
    void addGlobalSymbol(SleighSymbol* a)
{
    a->id = symbollist.size();
    symbollist.push_back(a);
    SymbolScope* scope = getGlobalScope();
    a->scopeid = scope->getId();
    SleighSymbol* res = scope->addSymbol(a);
    if (res != a)
        throw SleighError("Duplicate symbol name '" + a->getName() + "'");
}
    void addSymbol(SleighSymbol* a)
{
    a->id = symbollist.size();
    symbollist.push_back(a);
    a->scopeid = curscope->getId();
    SleighSymbol* res = curscope->addSymbol(a);
    if (res != a)
        throw SleighError("Duplicate symbol name: " + a->getName());
}
    SleighSymbol* findSymbol(const string& nm) const {
        return findSymbolInternal(curscope, nm);
    }
    SleighSymbol* findSymbol(const string& nm, int4 skip) const {
        return findSymbolInternal(skipScope(skip), nm);
    }
    SleighSymbol* findGlobalSymbol(const string& nm) const {
        return findSymbolInternal(table[0], nm);
    }
    SleighSymbol* findSymbol(uintm id) const {
        if (id >= symbollist.size())
            throw SleighError("Bad symbol id");
        return symbollist[id];
    }
    void replaceSymbol(SleighSymbol* a, SleighSymbol* b)
{ // Replace symbol a with symbol b
  // assuming a and b have the same name
    SleighSymbol* sym;
    int4 i = table.size() - 1;

    while (i >= 0) { // Find the particular symbol
        sym = table[i]->findSymbol(a->getName());
        if (sym == a) {
            table[i]->removeSymbol(a);
            b->id = a->id;
            b->scopeid = a->scopeid;
            symbollist[b->id] = b;
            table[i]->addSymbol(b);
            delete a;
            return;
        }
        --i;
    }
}
    void encode(Encoder& encoder) const
{
    encoder.openElement(sla::ELEM_SYMBOL_TABLE);
    encoder.writeSignedInteger(sla::ATTRIB_SCOPESIZE, table.size());
    encoder.writeSignedInteger(sla::ATTRIB_SYMBOLSIZE, symbollist.size());
    for (int4 i = 0; i < table.size(); ++i) {
        encoder.openElement(sla::ELEM_SCOPE);
        encoder.writeUnsignedInteger(sla::ATTRIB_ID, table[i]->getId());
        if (table[i]->getParent() == (SymbolScope*)0)
            encoder.writeUnsignedInteger(sla::ATTRIB_PARENT, 0);
        else
            encoder.writeUnsignedInteger(sla::ATTRIB_PARENT, table[i]->getParent()->getId());
        encoder.closeElement(sla::ELEM_SCOPE);
    }

    // First save the headers
    for (int4 i = 0; i < symbollist.size(); ++i)
        symbollist[i]->encodeHeader(encoder);

    // Now save the content of each symbol
    for (int4 i = 0; i < symbollist.size(); ++i) // Must save IN ORDER
        symbollist[i]->encode(encoder);
    encoder.closeElement(sla::ELEM_SYMBOL_TABLE);
}
    void decode(Decoder& decoder, SleighBase* trans)
{
    int4 el = decoder.openElement(sla::ELEM_SYMBOL_TABLE);
    int8 tableSize = decoder.readSignedInteger(sla::ATTRIB_SCOPESIZE);
    int8 symbolSize = decoder.readSignedInteger(sla::ATTRIB_SYMBOLSIZE);
    if (tableSize < 0 || symbolSize < 0)
        throw SleighError("Bad symbol table size");
    if (tableSize > MAX_TABLES)
        throw SleighError("Maximum scopes exceeded");
    if (symbolSize > MAX_SYMBOLS)
        throw SleighError("Maximum symbols exceeded");
    table.resize(tableSize, (SymbolScope*)0);
    symbollist.resize(symbolSize, (SleighSymbol*)0);
    for (int4 i = 0; i < table.size(); ++i) { // Decode the scopes
        int4 subel = decoder.openElement(sla::ELEM_SCOPE);
        uintm id = decoder.readUnsignedInteger(sla::ATTRIB_ID);
        if (id >= table.size()) {
            throw SleighError("Bad symbol scope id: exceeds symbol scope table size");
        }

        uintm parent = decoder.readUnsignedInteger(sla::ATTRIB_PARENT);
        if (parent >= table.size()) {
            throw SleighError("Bad symbol scope parent id: exceeds symbol scope table size");
        }

        SymbolScope* parscope = (parent == id) ? (SymbolScope*)0 : table[parent];
        if (table[id]) {
            throw SleighError("Bad symbol scope parent id: not unique");
        }

        table[id] = new SymbolScope(parscope, id);
        decoder.closeElement(subel);
    }
    curscope = table[0]; // Current scope is global

    // Now decode the symbol shells
    for (int4 i = 0; i < symbollist.size(); ++i)
        decodeSymbolHeader(decoder);
    // Now decode the symbol content
    while (decoder.peekElement() != 0) {
        decoder.openElement();
        uintm id = decoder.readUnsignedInteger(sla::ATTRIB_ID);
        SleighSymbol* sym;
        sym = findSymbol(id);
        sym->decode(decoder, trans);
        // Tag closed by decode method
        // decoder.closeElement(subel);
    }
    decoder.closeElement(el);
}
    void decodeSymbolHeader(Decoder& decoder)
{ // Put the shell of a symbol in the symbol table
    symbolTableDecodeSymbolHeader(*this, decoder);
}
    void purge(void)
{ // Get rid of unsavable symbols and scopes
    symbolTablePurge(*this);
}
};

class SpaceSymbol : public SleighSymbol {
    AddrSpace* space;

public:
    SpaceSymbol(AddrSpace* spc) : SleighSymbol(spc->getName()) {
        space = spc;
    }
    AddrSpace* getSpace(void) const {
        return space;
    }
    virtual symbol_type getType(void) const {
        return space_symbol;
    }
};

class TokenSymbol : public SleighSymbol {
    Token* tok;

public:
    TokenSymbol(Token* t) : SleighSymbol(t->getName()) {
        tok = t;
    }
    ~TokenSymbol(void) {
        delete tok;
    }
    Token* getToken(void) const {
        return tok;
    }
    virtual symbol_type getType(void) const {
        return token_symbol;
    }
};

class SectionSymbol : public SleighSymbol { // Named p-code sections
    int4 templateid;                        // Index into the ConstructTpl array
    int4 define_count;                      // Number of definitions of this named section
    int4 ref_count;                         // Number of references to this named section
public:
    SectionSymbol(const string& nm, int4 id) : SleighSymbol(nm) {
        templateid = id;
        define_count = 0;
        ref_count = 0;
    }
    int4 getTemplateId(void) const {
        return templateid;
    }
    void incrementDefineCount(void) {
        define_count += 1;
    }
    void incrementRefCount(void) {
        ref_count += 1;
    }
    int4 getDefineCount(void) const {
        return define_count;
    }
    int4 getRefCount(void) const {
        return ref_count;
    }
    virtual symbol_type getType(void) const {
        return section_symbol;
    }
};

class UserOpSymbol : public SleighSymbol { // A user-defined pcode-op
    uint4 index;

public:
    UserOpSymbol(void) {} // For use with decode
    UserOpSymbol(const string& nm) : SleighSymbol(nm) {
        index = 0;
    }
    void setIndex(uint4 ind) {
        index = ind;
    }
    uint4 getIndex(void) const {
        return index;
    }
    virtual symbol_type getType(void) const {
        return userop_symbol;
    }
    virtual void encode(Encoder& encoder) const
{
    encoder.openElement(sla::ELEM_USEROP);
    encoder.writeUnsignedInteger(sla::ATTRIB_ID, getId());
    encoder.writeSignedInteger(sla::ATTRIB_INDEX, index);
    encoder.closeElement(sla::ELEM_USEROP);
}
    virtual void encodeHeader(Encoder& encoder) const
{
    encoder.openElement(sla::ELEM_USEROP_HEAD);
    SleighSymbol::encodeHeader(encoder);
    encoder.closeElement(sla::ELEM_USEROP_HEAD);
}
    virtual void decode(Decoder& decoder, SleighBase* trans)
{
    index = decoder.readSignedInteger(sla::ATTRIB_INDEX);
    decoder.closeElement(sla::ELEM_USEROP.getId());
}
};

class Constructor; // Forward declaration
// This is the central sleigh object
class TripleSymbol : public SleighSymbol {
public:
    TripleSymbol(void) {}
    TripleSymbol(const string& nm) : SleighSymbol(nm) {}
    virtual Constructor* resolve(ParserWalker& walker) {
        return (Constructor*)0;
    }
    virtual PatternExpression* getPatternExpression(void) const = 0;
    virtual void getFixedHandle(FixedHandle& hand, ParserWalker& walker) const = 0;
    virtual int4 getSize(void) const {
        return 0;
    } // Size out of context
    virtual void print(ostream& s, ParserWalker& walker) const = 0;
    virtual void collectLocalValues(vector<uintb>& results) const {}
};

class FamilySymbol : public TripleSymbol {
public:
    FamilySymbol(void) {}
    FamilySymbol(const string& nm) : TripleSymbol(nm) {}
    virtual PatternValue* getPatternValue(void) const = 0;
};

class SpecificSymbol : public TripleSymbol {
public:
    SpecificSymbol(void) {}
    SpecificSymbol(const string& nm) : TripleSymbol(nm) {}
    virtual VarnodeTpl* getVarnode(void) const = 0;
};

class PatternlessSymbol : public SpecificSymbol { // Behaves like constant 0 pattern
    ConstantValue* patexp;

public:
    PatternlessSymbol(void)
{ // The void constructor must explicitly build the ConstantValue. It is not decode (or encoded)
    patexp = new ConstantValue((intb)0);
    patexp->layClaim();
} // For use with decode
    PatternlessSymbol(const string& nm) : SpecificSymbol(nm)
{
    patexp = new ConstantValue((intb)0);
    patexp->layClaim();
}
    virtual ~PatternlessSymbol(void)
{
    PatternExpression::release(patexp);
}
    virtual PatternExpression* getPatternExpression(void) const {
        return patexp;
    }
};

class EpsilonSymbol : public PatternlessSymbol { // Another name for zero pattern/value
    AddrSpace* const_space;

public:
    EpsilonSymbol(void) {} // For use with decode
    EpsilonSymbol(const string& nm, AddrSpace* spc) : PatternlessSymbol(nm) {
        const_space = spc;
    }
    virtual void getFixedHandle(FixedHandle& hand, ParserWalker& walker) const
{
    hand.space = const_space;
    hand.offset_space = (AddrSpace*)0; // Not a dynamic value
    hand.offset_offset = 0;
    hand.size = 0; // Cannot provide size
}
    virtual void print(ostream& s, ParserWalker& walker) const
{
    s << '0';
}
    virtual symbol_type getType(void) const {
        return epsilon_symbol;
    }
    virtual VarnodeTpl* getVarnode(void) const
{
    VarnodeTpl* res = new VarnodeTpl(ConstTpl(const_space), ConstTpl(ConstTpl::real, 0), ConstTpl(ConstTpl::real, 0));
    return res;
}
    virtual void encode(Encoder& encoder) const
{
    encoder.openElement(sla::ELEM_EPSILON_SYM);
    encoder.writeUnsignedInteger(sla::ATTRIB_ID, getId());
    encoder.closeElement(sla::ELEM_EPSILON_SYM);
}
    virtual void encodeHeader(Encoder& encoder) const
{
    encoder.openElement(sla::ELEM_EPSILON_SYM_HEAD);
    SleighSymbol::encodeHeader(encoder);
    encoder.closeElement(sla::ELEM_EPSILON_SYM_HEAD);
}
    virtual void decode(Decoder& decoder, SleighBase* trans)
{
    const_space = sleighBaseGetConstantSpace(trans);
    decoder.closeElement(sla::ELEM_EPSILON_SYM.getId());
}
};

class ValueSymbol : public FamilySymbol {
protected:
    PatternValue* patval;

public:
    ValueSymbol(void) {
        patval = (PatternValue*)0;
    } // For use with decode
    ValueSymbol(const string& nm, PatternValue* pv) : FamilySymbol(nm)
{
    (patval = pv)->layClaim();
}
    virtual ~ValueSymbol(void)
{
    if (patval != (PatternValue*)0)
        PatternExpression::release(patval);
}
    virtual PatternValue* getPatternValue(void) const {
        return patval;
    }
    virtual PatternExpression* getPatternExpression(void) const {
        return patval;
    }
    virtual void getFixedHandle(FixedHandle& hand, ParserWalker& walker) const
{
    hand.space = walker.getConstSpace();
    hand.offset_space = (AddrSpace*)0;
    hand.offset_offset = (uintb)patval->getValue(walker);
    hand.size = 0; // Cannot provide size
}
    virtual void print(ostream& s, ParserWalker& walker) const
{
    intb val = patval->getValue(walker);
    if (val >= 0)
        s << "0x" << hex << val;
    else
        s << "-0x" << hex << -val;
}
    virtual symbol_type getType(void) const {
        return value_symbol;
    }
    virtual void encode(Encoder& encoder) const
{
    encoder.openElement(sla::ELEM_VALUE_SYM);
    encoder.writeUnsignedInteger(sla::ATTRIB_ID, getId());
    patval->encode(encoder);
    encoder.closeElement(sla::ELEM_VALUE_SYM);
}
    virtual void encodeHeader(Encoder& encoder) const
{
    encoder.openElement(sla::ELEM_VALUE_SYM_HEAD);
    SleighSymbol::encodeHeader(encoder);
    encoder.closeElement(sla::ELEM_VALUE_SYM_HEAD);
}
    virtual void decode(Decoder& decoder, SleighBase* trans)
{
    if (patval)
        throw DecoderError("Already decoded symbol");

    patval = (PatternValue*)sleighBaseDecodeExpression(decoder, trans);
    patval->layClaim();
    decoder.closeElement(sla::ELEM_VALUE_SYM.getId());
}
};

class ValueMapSymbol : public ValueSymbol {
    vector<intb> valuetable;
    bool tableisfilled;
    void checkTableFill(void)
{ // Check if all possible entries in the table have been filled
    intb min = patval->minValue();
    intb max = patval->maxValue();
    tableisfilled = (min >= 0) && (max < valuetable.size());
    for (uint4 i = 0; i < valuetable.size(); ++i) {
        if (valuetable[i] == 0xBADBEEF)
            tableisfilled = false;
    }
}

public:
    ValueMapSymbol(void) {} // For use with decode
    ValueMapSymbol(const string& nm, PatternValue* pv, const vector<intb>& vt) : ValueSymbol(nm, pv) {
        valuetable = vt;
        checkTableFill();
    }
    virtual Constructor* resolve(ParserWalker& walker)
{
    if (!tableisfilled) {
        intb ind = patval->getValue(walker);
        if ((ind >= valuetable.size()) || (ind < 0) || (valuetable[ind] == 0xBADBEEF)) {
            ostringstream s;
            s << walker.getAddr().getShortcut();
            walker.getAddr().printRaw(s);
            s << ": No corresponding entry in valuetable";
            throw BadDataError(s.str());
        }
    }
    return (Constructor*)0;
}
    virtual void getFixedHandle(FixedHandle& hand, ParserWalker& walker) const
{
    uint4 ind = (uint4)patval->getValue(walker);
    // The resolve routine has checked that -ind- must be a valid index
    hand.space = walker.getConstSpace();
    hand.offset_space = (AddrSpace*)0; // Not a dynamic value
    hand.offset_offset = (uintb)valuetable[ind];
    hand.size = 0; // Cannot provide size
}
    virtual void print(ostream& s, ParserWalker& walker) const
{
    uint4 ind = (uint4)patval->getValue(walker);
    // ind is already checked to be in range by the resolve routine
    intb val = valuetable[ind];
    if (val >= 0)
        s << "0x" << hex << val;
    else
        s << "-0x" << hex << -val;
}
    virtual symbol_type getType(void) const {
        return valuemap_symbol;
    }
    virtual void encode(Encoder& encoder) const
{
    encoder.openElement(sla::ELEM_VALUEMAP_SYM);
    encoder.writeUnsignedInteger(sla::ATTRIB_ID, getId());
    patval->encode(encoder);
    for (uint4 i = 0; i < valuetable.size(); ++i) {
        encoder.openElement(sla::ELEM_VALUETAB);
        encoder.writeSignedInteger(sla::ATTRIB_VAL, valuetable[i]);
        encoder.closeElement(sla::ELEM_VALUETAB);
    }
    encoder.closeElement(sla::ELEM_VALUEMAP_SYM);
}
    virtual void encodeHeader(Encoder& encoder) const
{
    encoder.openElement(sla::ELEM_VALUEMAP_SYM_HEAD);
    SleighSymbol::encodeHeader(encoder);
    encoder.closeElement(sla::ELEM_VALUEMAP_SYM_HEAD);
}
    virtual void decode(Decoder& decoder, SleighBase* trans)
{
    if (patval)
        throw DecoderError("Already decoded symbol");

    patval = (PatternValue*)sleighBaseDecodeExpression(decoder, trans);
    patval->layClaim();
    while (decoder.peekElement() != 0) {
        uint4 subel = decoder.openElement();
        intb val = decoder.readSignedInteger(sla::ATTRIB_VAL);
        valuetable.push_back(val);
        decoder.closeElement(subel);
    }
    decoder.closeElement(sla::ELEM_VALUEMAP_SYM.getId());
    checkTableFill();
}
};

class NameSymbol : public ValueSymbol {
    vector<string> nametable;
    bool tableisfilled;
    void checkTableFill(void)
{ // Check if all possible entries in the table have been filled
    intb min = patval->minValue();
    intb max = patval->maxValue();
    tableisfilled = (min >= 0) && (max < nametable.size());
    for (uint4 i = 0; i < nametable.size(); ++i) {
        if ((nametable[i] == "_") || (nametable[i] == "\t")) {
            nametable[i] = "\t"; // TAB indicates illegal index
            tableisfilled = false;
        }
    }
}

public:
    NameSymbol(void) {} // For use with decode
    NameSymbol(const string& nm, PatternValue* pv, const vector<string>& nt) : ValueSymbol(nm, pv) {
        nametable = nt;
        checkTableFill();
    }
    virtual Constructor* resolve(ParserWalker& walker)
{
    if (!tableisfilled) {
        intb ind = patval->getValue(walker);
        if ((ind >= nametable.size()) || (ind < 0) || ((nametable[ind].size() == 1) && (nametable[ind][0] == '\t'))) {
            ostringstream s;
            s << walker.getAddr().getShortcut();
            walker.getAddr().printRaw(s);
            s << ": No corresponding entry in nametable";
            throw BadDataError(s.str());
        }
    }
    return (Constructor*)0;
}
    virtual void print(ostream& s, ParserWalker& walker) const
{
    uint4 ind = (uint4)patval->getValue(walker);
    // ind is already checked to be in range by the resolve routine
    s << nametable[ind];
}
    virtual symbol_type getType(void) const {
        return name_symbol;
    }
    virtual void encode(Encoder& encoder) const
{
    encoder.openElement(sla::ELEM_NAME_SYM);
    encoder.writeUnsignedInteger(sla::ATTRIB_ID, getId());
    patval->encode(encoder);
    for (int4 i = 0; i < nametable.size(); ++i) {
        encoder.openElement(sla::ELEM_NAMETAB);
        if (nametable[i] == "\t") { // TAB indicates an illegal index
                                    // Emit tag with no name attribute
        } else
            encoder.writeString(sla::ATTRIB_NAME, nametable[i]);
        encoder.closeElement(sla::ELEM_NAMETAB);
    }
    encoder.closeElement(sla::ELEM_NAME_SYM);
}
    virtual void encodeHeader(Encoder& encoder) const
{
    encoder.openElement(sla::ELEM_NAME_SYM_HEAD);
    SleighSymbol::encodeHeader(encoder);
    encoder.closeElement(sla::ELEM_NAME_SYM_HEAD);
}
    virtual void decode(Decoder& decoder, SleighBase* trans)
{
    if (patval)
        throw DecoderError("Already decoded symbol");

    patval = (PatternValue*)sleighBaseDecodeExpression(decoder, trans);
    patval->layClaim();
    while (decoder.peekElement() != 0) {
        uint4 subel = decoder.openElement();
        if (decoder.getNextAttributeId() == sla::ATTRIB_NAME)
            nametable.push_back(decoder.readString());
        else
            nametable.push_back("\t"); // TAB indicates an illegal index
        decoder.closeElement(subel);
    }
    decoder.closeElement(sla::ELEM_NAME_SYM.getId());
    checkTableFill();
}
};

class VarnodeSymbol : public PatternlessSymbol { // A global varnode
    VarnodeData fix;
    bool context_bits;

public:
    VarnodeSymbol(void) {} // For use with decode
    VarnodeSymbol(const string& nm, AddrSpace* base, uintb offset, int4 size) : PatternlessSymbol(nm)
{
    fix.space = base;
    fix.offset = offset;
    fix.size = size;
    context_bits = false;
}
    void markAsContext(void) {
        context_bits = true;
    }
    const VarnodeData& getFixedVarnode(void) const {
        return fix;
    }
    virtual VarnodeTpl* getVarnode(void) const
{
    return new VarnodeTpl(ConstTpl(fix.space), ConstTpl(ConstTpl::real, fix.offset),
                          ConstTpl(ConstTpl::real, fix.size));
}
    virtual void getFixedHandle(FixedHandle& hand, ParserWalker& walker) const
{
    hand.space = fix.space;
    hand.offset_space = (AddrSpace*)0; // Not a dynamic symbol
    hand.offset_offset = fix.offset;
    hand.size = fix.size;
}
    virtual int4 getSize(void) const {
        return fix.size;
    }
    virtual void print(ostream& s, ParserWalker& walker) const {
        s << getName();
    }
    virtual void collectLocalValues(vector<uintb>& results) const
{
    if (fix.space->getType() == IPTR_INTERNAL)
        results.push_back(fix.offset);
}
    virtual symbol_type getType(void) const {
        return varnode_symbol;
    }
    virtual void encode(Encoder& encoder) const
{
    encoder.openElement(sla::ELEM_VARNODE_SYM);
    encoder.writeUnsignedInteger(sla::ATTRIB_ID, getId());
    encoder.writeSpace(sla::ATTRIB_SPACE, fix.space);
    encoder.writeUnsignedInteger(sla::ATTRIB_OFF, fix.offset);
    encoder.writeSignedInteger(sla::ATTRIB_SIZE, fix.size);
    encoder.closeElement(sla::ELEM_VARNODE_SYM);
}
    virtual void encodeHeader(Encoder& encoder) const
{
    encoder.openElement(sla::ELEM_VARNODE_SYM_HEAD);
    SleighSymbol::encodeHeader(encoder);
    encoder.closeElement(sla::ELEM_VARNODE_SYM_HEAD);
}
    virtual void decode(Decoder& decoder, SleighBase* trans)
{
    fix.space = decoder.readSpace(sla::ATTRIB_SPACE);
    fix.offset = decoder.readUnsignedInteger(sla::ATTRIB_OFF);
    fix.size = decoder.readSignedInteger(sla::ATTRIB_SIZE);
    // PatternlessSymbol does not need restoring
    decoder.closeElement(sla::ELEM_VARNODE_SYM.getId());
}
};

class BitrangeSymbol : public SleighSymbol { // A smaller bitrange within a varnode
    VarnodeSymbol* varsym;                   // Varnode containing the bitrange
    uint4 bitoffset;                         // least significant bit of range
    uint4 numbits;                           // number of bits in the range
public:
    BitrangeSymbol(void) {} // For use with decode
    BitrangeSymbol(const string& nm, VarnodeSymbol* sym, uint4 bitoff, uint4 num) : SleighSymbol(nm) {
        varsym = sym;
        bitoffset = bitoff;
        numbits = num;
    }
    VarnodeSymbol* getParentSymbol(void) const {
        return varsym;
    }
    uint4 getBitOffset(void) const {
        return bitoffset;
    }
    uint4 numBits(void) const {
        return numbits;
    }
    virtual symbol_type getType(void) const {
        return bitrange_symbol;
    }
};

class ContextSymbol : public ValueSymbol {
    VarnodeSymbol* vn;
    uint4 low, high; // into a varnode
    bool flow;

public:
    ContextSymbol(void) {} // For use with decode
    ContextSymbol(const string& nm, ContextField* pate, VarnodeSymbol* v, uint4 l, uint4 h, bool fl)
    : ValueSymbol(nm, pate)
{
    vn = v;
    low = l;
    high = h;
    flow = fl;
}
    VarnodeSymbol* getVarnode(void) const {
        return vn;
    }
    uint4 getLow(void) const {
        return low;
    }
    uint4 getHigh(void) const {
        return high;
    }
    bool getFlow(void) const {
        return flow;
    }
    virtual symbol_type getType(void) const {
        return context_symbol;
    }
    virtual void encode(Encoder& encoder) const
{
    encoder.openElement(sla::ELEM_CONTEXT_SYM);
    encoder.writeUnsignedInteger(sla::ATTRIB_ID, getId());
    encoder.writeUnsignedInteger(sla::ATTRIB_VARNODE, vn->getId());
    encoder.writeSignedInteger(sla::ATTRIB_LOW, low);
    encoder.writeSignedInteger(sla::ATTRIB_HIGH, high);
    encoder.writeBool(sla::ATTRIB_FLOW, flow);
    patval->encode(encoder);
    encoder.closeElement(sla::ELEM_CONTEXT_SYM);
}
    virtual void encodeHeader(Encoder& encoder) const
{
    encoder.openElement(sla::ELEM_CONTEXT_SYM_HEAD);
    SleighSymbol::encodeHeader(encoder);
    encoder.closeElement(sla::ELEM_CONTEXT_SYM_HEAD);
}
    virtual void decode(Decoder& decoder, SleighBase* trans)
{
    // SleighSymbol::decodeHeader(decoder);	// Already filled in by the header tag
    flow = false;
    bool highMissing = true;
    bool lowMissing = true;
    uint4 attrib = decoder.getNextAttributeId();
    while (attrib != 0) {
        if (attrib == sla::ATTRIB_VARNODE) {
            uintm id = decoder.readUnsignedInteger();
            vn = (VarnodeSymbol*)sleighBaseFindSymbol(trans, id);
        } else if (attrib == sla::ATTRIB_LOW) {
            low = decoder.readSignedInteger();
            lowMissing = false;
        } else if (attrib == sla::ATTRIB_HIGH) {
            high = decoder.readSignedInteger();
            highMissing = false;
        } else if (attrib == sla::ATTRIB_FLOW) {
            flow = decoder.readBool();
        }
        attrib = decoder.getNextAttributeId();
    }
    if (lowMissing || highMissing) {
        throw DecoderError("Missing high/low attributes");
    }

    if (patval)
        throw DecoderError("Already decoded symbol");

    patval = (PatternValue*)sleighBaseDecodeExpression(decoder, trans);
    patval->layClaim();
    decoder.closeElement(sla::ELEM_CONTEXT_SYM.getId());
}
};

class VarnodeListSymbol : public ValueSymbol {
    vector<VarnodeSymbol*> varnode_table;
    bool tableisfilled;
    void checkTableFill(void)
{
    intb min = patval->minValue();
    intb max = patval->maxValue();
    tableisfilled = (min >= 0) && (max < varnode_table.size());
    for (uint4 i = 0; i < varnode_table.size(); ++i) {
        if (varnode_table[i] == (VarnodeSymbol*)0)
            tableisfilled = false;
    }
}

public:
    VarnodeListSymbol(void) {} // For use with decode
    VarnodeListSymbol(const string& nm, PatternValue* pv, const vector<SleighSymbol*>& vt)
    : ValueSymbol(nm, pv)
{
    for (int4 i = 0; i < vt.size(); ++i)
        varnode_table.push_back((VarnodeSymbol*)vt[i]);
    checkTableFill();
}
    virtual Constructor* resolve(ParserWalker& walker)
{
    if (!tableisfilled) {
        intb ind = patval->getValue(walker);
        if ((ind < 0) || (ind >= varnode_table.size()) || (varnode_table[ind] == (VarnodeSymbol*)0)) {
            ostringstream s;
            s << walker.getAddr().getShortcut();
            walker.getAddr().printRaw(s);
            s << ": No corresponding entry in varnode list";
            throw BadDataError(s.str());
        }
    }
    return (Constructor*)0;
}
    virtual void getFixedHandle(FixedHandle& hand, ParserWalker& walker) const
{
    uint4 ind = (uint4)patval->getValue(walker);
    // The resolve routine has checked that -ind- must be a valid index
    const VarnodeData& fix(varnode_table[ind]->getFixedVarnode());
    hand.space = fix.space;
    hand.offset_space = (AddrSpace*)0; // Not a dynamic value
    hand.offset_offset = fix.offset;
    hand.size = fix.size;
}
    virtual int4 getSize(void) const
{
    for (int4 i = 0; i < varnode_table.size(); ++i) {
        VarnodeSymbol* vnsym = varnode_table[i]; // Assume all are same size
        if (vnsym != (VarnodeSymbol*)0)
            return vnsym->getSize();
    }
    throw SleighError("No register attached to: " + getName());
}
    virtual void print(ostream& s, ParserWalker& walker) const
{
    uint4 ind = (uint4)patval->getValue(walker);
    if (ind >= varnode_table.size())
        throw SleighError("Value out of range for varnode table");
    s << varnode_table[ind]->getName();
}
    virtual symbol_type getType(void) const {
        return varnodelist_symbol;
    }
    virtual void encode(Encoder& encoder) const
{
    encoder.openElement(sla::ELEM_VARLIST_SYM);
    encoder.writeUnsignedInteger(sla::ATTRIB_ID, getId());
    patval->encode(encoder);
    for (int4 i = 0; i < varnode_table.size(); ++i) {
        if (varnode_table[i] == (VarnodeSymbol*)0) {
            encoder.openElement(sla::ELEM_NULL);
            encoder.closeElement(sla::ELEM_NULL);
        } else {
            encoder.openElement(sla::ELEM_VAR);
            encoder.writeUnsignedInteger(sla::ATTRIB_ID, varnode_table[i]->getId());
            encoder.closeElement(sla::ELEM_VAR);
        }
    }
    encoder.closeElement(sla::ELEM_VARLIST_SYM);
}
    virtual void encodeHeader(Encoder& encoder) const
{
    encoder.openElement(sla::ELEM_VARLIST_SYM_HEAD);
    SleighSymbol::encodeHeader(encoder);
    encoder.closeElement(sla::ELEM_VARLIST_SYM_HEAD);
}
    virtual void decode(Decoder& decoder, SleighBase* trans)
{
    if (patval)
        throw DecoderError("Already decoded symbol");

    patval = (PatternValue*)sleighBaseDecodeExpression(decoder, trans);
    patval->layClaim();
    while (decoder.peekElement() != 0) {
        uint4 subel = decoder.openElement();
        if (subel == sla::ELEM_VAR) {
            uintm id = decoder.readUnsignedInteger(sla::ATTRIB_ID);
            varnode_table.push_back((VarnodeSymbol*)sleighBaseFindSymbol(trans, id));
        } else
            varnode_table.push_back((VarnodeSymbol*)0);
        decoder.closeElement(subel);
    }
    decoder.closeElement(sla::ELEM_VARLIST_SYM.getId());
    checkTableFill();
}
};

class OperandSymbol : public SpecificSymbol {
    friend class Constructor;
    friend class OperandEquation;
    friend bool operandEquationResolve(OperandResolve&, int4);

public:
    enum { code_address = 1, offset_irrel = 2, variable_len = 4, marked = 8 };

private:
    uint4 reloffset;    // Relative offset
    int4 offsetbase;    // Base operand to which offset is relative (-1=constructor start)
    int4 minimumlength; // Minimum size of operand (within instruction tokens)
    int4 hand;          // Handle index
    OperandValue* localexp;
    TripleSymbol* triple;      // Defining symbol
    PatternExpression* defexp; // OR defining expression
    uint4 flags;
    void setVariableLength(void) {
        flags |= variable_len;
    }
    bool isVariableLength(void) const {
        return ((flags & variable_len) != 0);
    }

public:
    OperandSymbol(void) : localexp(nullptr), defexp(nullptr) {} // For use with decode
    OperandSymbol(const string& nm, int4 index, Constructor* ct) : SpecificSymbol(nm)
{
    flags = 0;
    hand = index;
    localexp = new OperandValue(index, ct);
    localexp->layClaim();
    defexp = (PatternExpression*)0;
    triple = (TripleSymbol*)0;
}
    uint4 getRelativeOffset(void) const {
        return reloffset;
    }
    int4 getOffsetBase(void) const {
        return offsetbase;
    }
    int4 getMinimumLength(void) const {
        return minimumlength;
    }
    PatternExpression* getDefiningExpression(void) const {
        return defexp;
    }
    TripleSymbol* getDefiningSymbol(void) const {
        return triple;
    }
    int4 getIndex(void) const {
        return hand;
    }
    void defineOperand(PatternExpression* pe)
{
    if ((defexp != (PatternExpression*)0) || (triple != (TripleSymbol*)0))
        throw SleighError("Redefining operand");
    defexp = pe;
    defexp->layClaim();
}
    void defineOperand(TripleSymbol* tri)
{
    if ((defexp != (PatternExpression*)0) || (triple != (TripleSymbol*)0))
        throw SleighError("Redefining operand");
    triple = tri;
}
    void setCodeAddress(void) {
        flags |= code_address;
    }
    bool isCodeAddress(void) const {
        return ((flags & code_address) != 0);
    }
    void setOffsetIrrelevant(void) {
        flags |= offset_irrel;
    }
    bool isOffsetIrrelevant(void) const {
        return ((flags & offset_irrel) != 0);
    }
    void setMark(void) {
        flags |= marked;
    }
    void clearMark(void) {
        flags &= ~((uint4)marked);
    }
    bool isMarked(void) const {
        return ((flags & marked) != 0);
    }
    virtual ~OperandSymbol(void)
{
    if (localexp != (PatternExpression*)0)
        PatternExpression::release(localexp);

    if (defexp != (PatternExpression*)0)
        PatternExpression::release(defexp);
}
    virtual VarnodeTpl* getVarnode(void) const
{
    VarnodeTpl* res;
    if (defexp != (PatternExpression*)0)
        res = new VarnodeTpl(hand, true); // Definite constant handle
    else {
        SpecificSymbol* specsym = dynamic_cast<SpecificSymbol*>(triple);
        if (specsym != (SpecificSymbol*)0)
            res = specsym->getVarnode();
        else if ((triple != (TripleSymbol*)0) &&
                 ((triple->getType() == valuemap_symbol) || (triple->getType() == name_symbol)))
            res = new VarnodeTpl(hand, true); // Zero-size symbols
        else
            res = new VarnodeTpl(hand, false); // Possible dynamic handle
    }
    return res;
}
    virtual PatternExpression* getPatternExpression(void) const {
        return localexp;
    }
    virtual void getFixedHandle(FixedHandle& hnd, ParserWalker& walker) const
{
    hnd = walker.getFixedHandle(hand);
}
    virtual int4 getSize(void) const
{
    if (triple != (TripleSymbol*)0)
        return triple->getSize();
    return 0;
}
    virtual void print(ostream& s, ParserWalker& walker) const
{
    walker.pushOperand(getIndex());
    if (triple != (TripleSymbol*)0) {
        if (triple->getType() == SleighSymbol::subtable_symbol)
            parserWalkerPrintConstructor(walker, s);
        else
            triple->print(s, walker);
    } else {
        intb val = defexp->getValue(walker);
        if (val >= 0)
            s << "0x" << hex << val;
        else
            s << "-0x" << hex << -val;
    }
    walker.popOperand();
}
    virtual void collectLocalValues(vector<uintb>& results) const
{
    if (triple != (TripleSymbol*)0)
        triple->collectLocalValues(results);
}
    virtual symbol_type getType(void) const {
        return operand_symbol;
    }
    virtual void encode(Encoder& encoder) const
{
    encoder.openElement(sla::ELEM_OPERAND_SYM);
    encoder.writeUnsignedInteger(sla::ATTRIB_ID, getId());
    if (triple != (TripleSymbol*)0)
        encoder.writeUnsignedInteger(sla::ATTRIB_SUBSYM, triple->getId());
    encoder.writeSignedInteger(sla::ATTRIB_OFF, reloffset);
    encoder.writeSignedInteger(sla::ATTRIB_BASE, offsetbase);
    encoder.writeSignedInteger(sla::ATTRIB_MINLEN, minimumlength);
    if (isCodeAddress())
        encoder.writeBool(sla::ATTRIB_CODE, true);
    encoder.writeSignedInteger(sla::ATTRIB_INDEX, hand);
    localexp->encode(encoder);
    if (defexp != (PatternExpression*)0)
        defexp->encode(encoder);
    encoder.closeElement(sla::ELEM_OPERAND_SYM);
}
    virtual void encodeHeader(Encoder& encoder) const
{
    encoder.openElement(sla::ELEM_OPERAND_SYM_HEAD);
    SleighSymbol::encodeHeader(encoder);
    encoder.closeElement(sla::ELEM_OPERAND_SYM_HEAD);
}
    virtual void decode(Decoder& decoder, SleighBase* trans)
{
    if (defexp || localexp)
        throw DecoderError("Already decoded symbol");

    defexp = (PatternExpression*)0;
    triple = (TripleSymbol*)0;
    flags = 0;
    uint4 attrib = decoder.getNextAttributeId();
    while (attrib != 0) {
        attrib = decoder.getNextAttributeId();
        if (attrib == sla::ATTRIB_INDEX)
            hand = decoder.readSignedInteger();
        else if (attrib == sla::ATTRIB_OFF)
            reloffset = decoder.readSignedInteger();
        else if (attrib == sla::ATTRIB_BASE)
            offsetbase = decoder.readSignedInteger();
        else if (attrib == sla::ATTRIB_MINLEN)
            minimumlength = decoder.readSignedInteger();
        else if (attrib == sla::ATTRIB_SUBSYM) {
            uintm id = decoder.readUnsignedInteger();
            triple = (TripleSymbol*)sleighBaseFindSymbol(trans, id);
        } else if (attrib == sla::ATTRIB_CODE) {
            if (decoder.readBool())
                flags |= code_address;
        }
    }
    localexp = (OperandValue*)sleighBaseDecodeExpression(decoder, trans);
    localexp->layClaim();
    if (decoder.peekElement() != 0) {
        defexp = sleighBaseDecodeExpression(decoder, trans);
        defexp->layClaim();
    }
    decoder.closeElement(sla::ELEM_OPERAND_SYM.getId());
}
};

class StartSymbol : public SpecificSymbol {
    AddrSpace* const_space;
    PatternExpression* patexp;

public:
    StartSymbol(void) {
        patexp = (PatternExpression*)0;
    } // For use with decode
    StartSymbol(const string& nm, AddrSpace* cspc)
    : SpecificSymbol(nm)
{
    const_space = cspc;
    patexp = new StartInstructionValue();
    patexp->layClaim();
}
    virtual ~StartSymbol(void)
{
    if (patexp != (PatternExpression*)0)
        PatternExpression::release(patexp);
}
    virtual VarnodeTpl* getVarnode(void) const
{ // Returns current instruction offset as a constant
    ConstTpl spc(const_space);
    ConstTpl off(ConstTpl::j_start);
    ConstTpl sz_zero;
    return new VarnodeTpl(spc, off, sz_zero);
}
    virtual PatternExpression* getPatternExpression(void) const {
        return patexp;
    }
    virtual void getFixedHandle(FixedHandle& hand, ParserWalker& walker) const
{
    hand.space = walker.getCurSpace();
    hand.offset_space = (AddrSpace*)0;
    hand.offset_offset = walker.getAddr().getOffset(); // Get starting address of instruction
    hand.size = hand.space->getAddrSize();
}
    virtual void print(ostream& s, ParserWalker& walker) const
{
    intb val = (intb)walker.getAddr().getOffset();
    s << "0x" << hex << val;
}
    virtual symbol_type getType(void) const {
        return start_symbol;
    }
    virtual void encode(Encoder& encoder) const
{
    encoder.openElement(sla::ELEM_START_SYM);
    encoder.writeUnsignedInteger(sla::ATTRIB_ID, getId());
    encoder.closeElement(sla::ELEM_START_SYM);
}
    virtual void encodeHeader(Encoder& encoder) const
{
    encoder.openElement(sla::ELEM_START_SYM_HEAD);
    SleighSymbol::encodeHeader(encoder);
    encoder.closeElement(sla::ELEM_START_SYM_HEAD);
}
    virtual void decode(Decoder& decoder, SleighBase* trans)
{
    const_space = sleighBaseGetConstantSpace(trans);
    patexp = new StartInstructionValue();
    patexp->layClaim();
    decoder.closeElement(sla::ELEM_START_SYM.getId());
}
};

class EndSymbol : public SpecificSymbol {
    AddrSpace* const_space;
    PatternExpression* patexp;

public:
    EndSymbol(void) {
        patexp = (PatternExpression*)0;
    } // For use with decode
    EndSymbol(const string& nm, AddrSpace* cspc)
    : SpecificSymbol(nm)
{
    const_space = cspc;
    patexp = new EndInstructionValue();
    patexp->layClaim();
}
    virtual ~EndSymbol(void)
{
    if (patexp != (PatternExpression*)0)
        PatternExpression::release(patexp);
}
    virtual VarnodeTpl* getVarnode(void) const
{ // Return next instruction offset as a constant
    ConstTpl spc(const_space);
    ConstTpl off(ConstTpl::j_next);
    ConstTpl sz_zero;
    return new VarnodeTpl(spc, off, sz_zero);
}
    virtual PatternExpression* getPatternExpression(void) const {
        return patexp;
    }
    virtual void getFixedHandle(FixedHandle& hand, ParserWalker& walker) const
{
    hand.space = walker.getCurSpace();
    hand.offset_space = (AddrSpace*)0;
    hand.offset_offset = walker.getNaddr().getOffset(); // Get starting address of next instruction
    hand.size = hand.space->getAddrSize();
}
    virtual void print(ostream& s, ParserWalker& walker) const
{
    intb val = (intb)walker.getNaddr().getOffset();
    s << "0x" << hex << val;
}
    virtual symbol_type getType(void) const {
        return end_symbol;
    }
    virtual void encode(Encoder& encoder) const
{
    encoder.openElement(sla::ELEM_END_SYM);
    encoder.writeUnsignedInteger(sla::ATTRIB_ID, getId());
    encoder.closeElement(sla::ELEM_END_SYM);
}
    virtual void encodeHeader(Encoder& encoder) const
{
    encoder.openElement(sla::ELEM_END_SYM_HEAD);
    SleighSymbol::encodeHeader(encoder);
    encoder.closeElement(sla::ELEM_END_SYM_HEAD);
}
    virtual void decode(Decoder& decoder, SleighBase* trans)
{
    const_space = sleighBaseGetConstantSpace(trans);
    patexp = new EndInstructionValue();
    patexp->layClaim();
    decoder.closeElement(sla::ELEM_END_SYM.getId());
}
};

class Next2Symbol : public SpecificSymbol {
    AddrSpace* const_space;
    PatternExpression* patexp;

public:
    Next2Symbol(void) {
        patexp = (PatternExpression*)0;
    } // For use with decode
    Next2Symbol(const string& nm, AddrSpace* cspc)
    : SpecificSymbol(nm)
{
    const_space = cspc;
    patexp = new Next2InstructionValue();
    patexp->layClaim();
}
    virtual ~Next2Symbol(void)
{
    if (patexp != (PatternExpression*)0)
        PatternExpression::release(patexp);
}
    virtual VarnodeTpl* getVarnode(void) const
{ // Return instruction offset after next instruction offset as a constant
    ConstTpl spc(const_space);
    ConstTpl off(ConstTpl::j_next2);
    ConstTpl sz_zero;
    return new VarnodeTpl(spc, off, sz_zero);
}
    virtual PatternExpression* getPatternExpression(void) const {
        return patexp;
    }
    virtual void getFixedHandle(FixedHandle& hand, ParserWalker& walker) const
{
    hand.space = walker.getCurSpace();
    hand.offset_space = (AddrSpace*)0;
    hand.offset_offset = walker.getN2addr().getOffset(); // Get instruction address after next instruction
    hand.size = hand.space->getAddrSize();
}
    virtual void print(ostream& s, ParserWalker& walker) const
{
    intb val = (intb)walker.getN2addr().getOffset();
    s << "0x" << hex << val;
}
    virtual symbol_type getType(void) const {
        return next2_symbol;
    }
    virtual void encode(Encoder& encoder) const
{
    encoder.openElement(sla::ELEM_NEXT2_SYM);
    encoder.writeUnsignedInteger(sla::ATTRIB_ID, getId());
    encoder.closeElement(sla::ELEM_NEXT2_SYM);
}
    virtual void encodeHeader(Encoder& encoder) const
{
    encoder.openElement(sla::ELEM_NEXT2_SYM_HEAD);
    SleighSymbol::encodeHeader(encoder);
    encoder.closeElement(sla::ELEM_NEXT2_SYM_HEAD);
}
    virtual void decode(Decoder& decoder, SleighBase* trans)
{
    const_space = sleighBaseGetConstantSpace(trans);
    patexp = new Next2InstructionValue();
    patexp->layClaim();
    decoder.closeElement(sla::ELEM_NEXT2_SYM.getId());
}
};

class FlowDestSymbol : public SpecificSymbol {
    AddrSpace* const_space;

public:
    FlowDestSymbol(void) {} // For use with decode
    FlowDestSymbol(const string& nm, AddrSpace* cspc)
    : SpecificSymbol(nm)
{
    const_space = cspc;
}
    virtual VarnodeTpl* getVarnode(void) const
{
    ConstTpl spc(const_space);
    ConstTpl off(ConstTpl::j_flowdest);
    ConstTpl sz_zero;
    return new VarnodeTpl(spc, off, sz_zero);
}
    virtual PatternExpression* getPatternExpression(void) const {
        throw SleighError("Cannot use symbol in pattern");
    }
    virtual void getFixedHandle(FixedHandle& hand, ParserWalker& walker) const
{
    Address refAddr = walker.getDestAddr();
    hand.space = const_space;
    hand.offset_space = (AddrSpace*)0;
    hand.offset_offset = refAddr.getOffset();
    hand.size = refAddr.getAddrSize();
}
    virtual void print(ostream& s, ParserWalker& walker) const
{
    intb val = (intb)walker.getDestAddr().getOffset();
    s << "0x" << hex << val;
}
    virtual symbol_type getType(void) const {
        return flowdest_symbol;
    }
};

class FlowRefSymbol : public SpecificSymbol {
    AddrSpace* const_space;

public:
    FlowRefSymbol(void) {} // For use with decode
    FlowRefSymbol(const string& nm, AddrSpace* cspc)
    : SpecificSymbol(nm)
{
    const_space = cspc;
}
    virtual VarnodeTpl* getVarnode(void) const
{
    ConstTpl spc(const_space);
    ConstTpl off(ConstTpl::j_flowref);
    ConstTpl sz_zero;
    return new VarnodeTpl(spc, off, sz_zero);
}
    virtual PatternExpression* getPatternExpression(void) const {
        throw SleighError("Cannot use symbol in pattern");
    }
    virtual void getFixedHandle(FixedHandle& hand, ParserWalker& walker) const
{
    Address refAddr = walker.getRefAddr();
    hand.space = const_space;
    hand.offset_space = (AddrSpace*)0;
    hand.offset_offset = refAddr.getOffset();
    hand.size = refAddr.getAddrSize();
}
    virtual void print(ostream& s, ParserWalker& walker) const
{
    intb val = (intb)walker.getRefAddr().getOffset();
    s << "0x" << hex << val;
}
    virtual symbol_type getType(void) const {
        return flowref_symbol;
    }
};

class ContextChange { // Change to context command
public:
    virtual ~ContextChange(void) {}
    virtual void validate(void) const = 0;
    virtual void encode(Encoder& encoder) const = 0;
    virtual void decode(Decoder& decoder, SleighBase* trans) = 0;
    virtual void apply(ParserWalkerChange& walker) const = 0;
    virtual ContextChange* clone(void) const = 0;
};

inline void calc_maskword(int4 sbit, int4 ebit, int4& num, int4& shift, uintm& mask)

{
    num = sbit / (8 * sizeof(uintm));
    if (num != ebit / (8 * sizeof(uintm)))
        throw SleighError("Context field not contained within one machine int");
    sbit -= num * 8 * sizeof(uintm);
    ebit -= num * 8 * sizeof(uintm);

    shift = 8 * sizeof(uintm) - ebit - 1;
    mask = (~((uintm)0)) >> (sbit + shift);
    mask <<= shift;
}

class ContextOp : public ContextChange {
    PatternExpression* patexp; // Expression determining value
    int4 num;                  // index of word containing context variable to set
    uintm mask;                // Mask off size of variable
    int4 shift;                // Number of bits to shift value into place
public:
    ContextOp(int4 startbit, int4 endbit, PatternExpression* pe)
{
    calc_maskword(startbit, endbit, num, shift, mask);
    patexp = pe;
    patexp->layClaim();
}
    ContextOp(void) : patexp(nullptr) {} // For use with decode
    virtual ~ContextOp(void) {
        if (patexp)
            PatternExpression::release(patexp);
    }
    virtual void validate(void) const
{ // Throw an exception if the PatternExpression is not valid
    vector<const PatternValue*> values;

    patexp->listValues(values); // Get all the expression tokens
    for (int4 i = 0; i < values.size(); ++i) {
        const OperandValue* val = dynamic_cast<const OperandValue*>(values[i]);
        if (val == (const OperandValue*)0)
            continue;
        // Certain operands cannot be used in context expressions
        // because these are evaluated BEFORE the operand offset
        // has been recovered. If the offset is not relative to
        // the base constructor, then we throw an error
        if (!val->isConstructorRelative())
            throw SleighError(val->getName() + ": cannot be used in context expression");
    }
}
    virtual void encode(Encoder& encoder) const
{
    encoder.openElement(sla::ELEM_CONTEXT_OP);
    encoder.writeSignedInteger(sla::ATTRIB_I, num);
    encoder.writeSignedInteger(sla::ATTRIB_SHIFT, shift);
    encoder.writeUnsignedInteger(sla::ATTRIB_MASK, mask);
    patexp->encode(encoder);
    encoder.closeElement(sla::ELEM_CONTEXT_OP);
}
    virtual void decode(Decoder& decoder, SleighBase* trans)
{
    uint4 el = decoder.openElement(sla::ELEM_CONTEXT_OP);
    num = decoder.readSignedInteger(sla::ATTRIB_I);
    shift = decoder.readSignedInteger(sla::ATTRIB_SHIFT);
    mask = decoder.readUnsignedInteger(sla::ATTRIB_MASK);
    patexp = sleighBaseDecodeExpression(decoder, trans);
    patexp->layClaim();
    decoder.closeElement(el);
}
    virtual void apply(ParserWalkerChange& walker) const
{
    uintm val = patexp->getValue(walker); // Get our value based on context
    val <<= shift;
    walker.getParserContext()->setContextWord(num, val, mask);
}
    virtual ContextChange* clone(void) const
{
    ContextOp* res = new ContextOp();
    (res->patexp = patexp)->layClaim();
    res->mask = mask;
    res->num = num;
    res->shift = shift;
    return res;
}
};

class ContextCommit : public ContextChange {
    TripleSymbol* sym;
    int4 num;   // Index of word containing context commit
    uintm mask; // mask of bits in word being committed
    bool flow;  // Whether the context "flows" from the point of change
public:
    ContextCommit(void) {} // For use with decode
    ContextCommit(TripleSymbol* s, int4 sbit, int4 ebit, bool fl)
{
    sym = s;
    flow = fl;

    int4 shift;
    calc_maskword(sbit, ebit, num, shift, mask);
}
    virtual void validate(void) const {}
    virtual void encode(Encoder& encoder) const
{
    encoder.openElement(sla::ELEM_COMMIT);
    encoder.writeUnsignedInteger(sla::ATTRIB_ID, sym->getId());
    encoder.writeSignedInteger(sla::ATTRIB_NUMBER, num);
    encoder.writeUnsignedInteger(sla::ATTRIB_MASK, mask);
    encoder.writeBool(sla::ATTRIB_FLOW, flow);
    encoder.closeElement(sla::ELEM_COMMIT);
}
    virtual void decode(Decoder& decoder, SleighBase* trans)
{
    uint4 el = decoder.openElement(sla::ELEM_COMMIT);
    uintm id = decoder.readUnsignedInteger(sla::ATTRIB_ID);
    sym = (TripleSymbol*)sleighBaseFindSymbol(trans, id);
    num = decoder.readSignedInteger(sla::ATTRIB_NUMBER);
    mask = decoder.readUnsignedInteger(sla::ATTRIB_MASK);
    flow = decoder.readBool(sla::ATTRIB_FLOW);
    decoder.closeElement(el);
}
    virtual void apply(ParserWalkerChange& walker) const
{
    walker.getParserContext()->addCommit(sym, num, mask, flow, walker.getPoint());
}
    virtual ContextChange* clone(void) const
{
    ContextCommit* res = new ContextCommit();
    res->sym = sym;
    res->flow = flow;
    res->mask = mask;
    res->num = num;
    return res;
}
};

class SubtableSymbol;
class Constructor { // This is NOT a symbol
    friend bool constructorTrySubtablePattern(TripleSymbol*, ostream&, vector<TokenPattern>&, bool&);
    friend bool constructorTryPrintMnemonic(const Constructor&, ostream&, ParserWalker&);
    friend bool constructorTryPrintBody(const Constructor&, ostream&, ParserWalker&);
    friend bool constructorIsRecursive(const Constructor&);
    TokenPattern* pattern;
    SubtableSymbol* parent;
    PatternEquation* pateq;
    vector<OperandSymbol*> operands;
    vector<string> printpiece;
    vector<ContextChange*> context;   // Context commands
    ConstructTpl* templ;              // The main p-code section
    vector<ConstructTpl*> namedtempl; // Other named p-code sections
    int4 minimumlength;               // Minimum length taken up by this constructor in bytes
    uintm id;                         // Unique id of constructor within subtable
    int4 firstwhitespace;             // Index of first whitespace piece in -printpiece-
    int4 flowthruindex;               // if >=0 then print only a single operand no markup
    int4 lineno;
    int4 src_index;       // source file index
    mutable bool inerror; // An error is associated with this Constructor
    void orderOperands(void)
{
    OperandSymbol* sym;
    vector<OperandSymbol*> patternorder;
    vector<OperandSymbol*> newops; // New order of the operands
    int4 lastsize;

    pateq->operandOrder(this, patternorder);
    for (int4 i = 0; i < operands.size(); ++i) { // Make sure patternorder contains all operands
        sym = operands[i];
        if (!sym->isMarked()) {
            patternorder.push_back(sym);
            sym->setMark(); // Make sure all operands are marked
        }
    }
    do {
        lastsize = newops.size();
        for (int4 i = 0; i < patternorder.size(); ++i) {
            sym = patternorder[i];
            if (!sym->isMarked())
                continue; // "unmarked" means it is already in newops
            if (sym->isOffsetIrrelevant())
                continue; // expression Operands come last
            if ((sym->offsetbase == -1) || (!operands[sym->offsetbase]->isMarked())) {
                newops.push_back(sym);
                sym->clearMark();
            }
        }
    } while (newops.size() != lastsize);
    for (int4 i = 0; i < patternorder.size(); ++i) { // Tack on expression Operands
        sym = patternorder[i];
        if (sym->isOffsetIrrelevant()) {
            newops.push_back(sym);
            sym->clearMark();
        }
    }

    if (newops.size() != operands.size())
        throw SleighError("Circular offset dependency between operands");

    for (int4 i = 0; i < newops.size(); ++i) { // Fix up operand indices
        newops[i]->hand = i;
        newops[i]->localexp->changeIndex(i);
    }
    vector<int4> handmap; // Create index translation map
    for (int4 i = 0; i < operands.size(); ++i)
        handmap.push_back(operands[i]->hand);

    // Fix up offsetbase
    for (int4 i = 0; i < newops.size(); ++i) {
        sym = newops[i];
        if (sym->offsetbase == -1)
            continue;
        sym->offsetbase = handmap[sym->offsetbase];
    }

    if (templ != (ConstructTpl*)0) // Fix up templates
        templ->changeHandleIndex(handmap);
    for (int4 i = 0; i < namedtempl.size(); ++i) {
        ConstructTpl* ntempl = namedtempl[i];
        if (ntempl != (ConstructTpl*)0)
            ntempl->changeHandleIndex(handmap);
    }

    // Fix up printpiece operand refs
    for (int4 i = 0; i < printpiece.size(); ++i) {
        if (printpiece[i][0] == '\n') {
            int4 index = printpiece[i][1] - 'A';
            index = handmap[index];
            printpiece[i][1] = 'A' + index;
        }
    }
    operands = newops;
}

public:
    Constructor(void)
{
    pattern = (TokenPattern*)0;
    parent = (SubtableSymbol*)0;
    pateq = (PatternEquation*)0;
    templ = (ConstructTpl*)0;
    firstwhitespace = -1;
    flowthruindex = -1;
    inerror = false;
} // For use with decode
    Constructor(SubtableSymbol* p)
{
    pattern = (TokenPattern*)0;
    parent = p;
    pateq = (PatternEquation*)0;
    templ = (ConstructTpl*)0;
    firstwhitespace = -1;
    inerror = false;
}
    ~Constructor(void)
{
    if (pattern != (TokenPattern*)0)
        delete pattern;
    if (pateq != (PatternEquation*)0)
        PatternEquation::release(pateq);
    if (templ != (ConstructTpl*)0)
        delete templ;
    for (int4 i = 0; i < namedtempl.size(); ++i) {
        ConstructTpl* ntpl = namedtempl[i];
        if (ntpl != (ConstructTpl*)0)
            delete ntpl;
    }
    vector<ContextChange*>::iterator iter;
    for (iter = context.begin(); iter != context.end(); ++iter)
        delete *iter;
}
    TokenPattern* buildPattern(ostream& s)
{
    if (pattern != (TokenPattern*)0)
        return pattern; // Already built

    pattern = new TokenPattern();
    vector<TokenPattern> oppattern;
    bool recursion = false;
    // Generate pattern for each operand, store in oppattern
    for (int4 i = 0; i < operands.size(); ++i) {
        OperandSymbol* sym = operands[i];
        TripleSymbol* triple = sym->getDefiningSymbol();
        PatternExpression* defexp = sym->getDefiningExpression();
        if (triple != (TripleSymbol*)0) {
            if (!constructorTrySubtablePattern(triple, s, oppattern, recursion))
                oppattern.push_back(triple->getPatternExpression()->genMinPattern(oppattern));
        } else if (defexp != (PatternExpression*)0)
            oppattern.push_back(defexp->genMinPattern(oppattern));
        else {
            throw SleighError(sym->getName() + ": operand is undefined");
        }
        TokenPattern& sympat(oppattern.back());
        sym->minimumlength = sympat.getMinimumLength();
        if (sympat.getLeftEllipsis() || sympat.getRightEllipsis())
            sym->setVariableLength();
    }

    if (pateq == (PatternEquation*)0)
        throw SleighError("Missing equation");

    // Build the entire pattern
    pateq->genPattern(oppattern);
    *pattern = pateq->getTokenPattern();
    if (pattern->alwaysFalse())
        throw SleighError("Impossible pattern");
    if (recursion)
        pattern->setRightEllipsis(true);
    minimumlength = pattern->getMinimumLength(); // Get length of the pattern in bytes

    // Resolve offsets of the operands
    OperandResolve resolve(operands);
    if (!pateq->resolveOperandLeft(resolve))
        throw SleighError("Unable to resolve operand offsets");

    for (int4 i = 0; i < operands.size(); ++i) { // Unravel relative offsets to absolute (if possible)
        int4 base, offset;
        OperandSymbol* sym = operands[i];
        if (sym->isOffsetIrrelevant()) {
            sym->offsetbase = -1;
            sym->reloffset = 0;
            continue;
        }
        base = sym->offsetbase;
        offset = sym->reloffset;
        while (base >= 0) {
            sym = operands[base];
            if (sym->isVariableLength())
                break; // Cannot resolve to absolute
            base = sym->offsetbase;
            offset += sym->getMinimumLength();
            offset += sym->reloffset;
            if (base < 0) {
                operands[i]->offsetbase = base;
                operands[i]->reloffset = offset;
            }
        }
    }

    // Make sure context expressions are valid
    for (int4 i = 0; i < context.size(); ++i)
        context[i]->validate();

    orderOperands(); // Order the operands based on offset dependency
    return pattern;
}
    TokenPattern* getPattern(void) const {
        return pattern;
    }
    void setMinimumLength(int4 l) {
        minimumlength = l;
    }
    int4 getMinimumLength(void) const {
        return minimumlength;
    }
    void setId(uintm i) {
        id = i;
    }
    uintm getId(void) const {
        return id;
    }
    void setLineno(int4 ln) {
        lineno = ln;
    }
    int4 getLineno(void) const {
        return lineno;
    }
    void setSrcIndex(int4 index) {
        src_index = index;
    }
    int4 getSrcIndex(void) {
        return src_index;
    }
    void addContext(const vector<ContextChange*>& vec) {
        context = vec;
    }
    void addOperand(OperandSymbol* sym)
{
    string operstring = "\n ";               // Indicater character for operand
    operstring[1] = ('A' + operands.size()); // Encode index of operand
    operands.push_back(sym);
    printpiece.push_back(operstring); // Placeholder for operand's string
}
    void addInvisibleOperand(OperandSymbol* sym)
{
    operands.push_back(sym);
}
    void addSyntax(const string& syn)
{
    string syntrim;

    if (syn.size() == 0)
        return;
    bool hasNonSpace = false;
    for (int4 i = 0; i < syn.size(); ++i) {
        if (syn[i] != ' ') {
            hasNonSpace = true;
            break;
        }
    }
    if (hasNonSpace)
        syntrim = syn;
    else
        syntrim = " ";
    if ((firstwhitespace == -1) && (syntrim == " "))
        firstwhitespace = printpiece.size();
    if (printpiece.empty())
        printpiece.push_back(syntrim);
    else if (printpiece.back() == " " && syntrim == " ") {
        // Don't add more whitespace
    } else if (printpiece.back()[0] == '\n' || printpiece.back() == " " || syntrim == " ")
        printpiece.push_back(syntrim);
    else {
        printpiece.back() += syntrim;
    }
}
    void addEquation(PatternEquation* pe)
{
    (pateq = pe)->layClaim();
}
    void setMainSection(ConstructTpl* tpl) {
        templ = tpl;
    }
    void setNamedSection(ConstructTpl* tpl, int4 id)
{ // Add a named section to the constructor
    while (namedtempl.size() <= id)
        namedtempl.push_back((ConstructTpl*)0);
    namedtempl[id] = tpl;
}
    SubtableSymbol* getParent(void) const {
        return parent;
    }
    int4 getNumOperands(void) const {
        return operands.size();
    }
    OperandSymbol* getOperand(int4 i) const {
        return operands[i];
    }
    PatternEquation* getPatternEquation(void) const {
        return pateq;
    }
    ConstructTpl* getTempl(void) const {
        return templ;
    }
    ConstructTpl* getNamedTempl(int4 secnum) const
{
    if (secnum < namedtempl.size())
        return namedtempl[secnum];
    return (ConstructTpl*)0;
}
    int4 getNumSections(void) const {
        return namedtempl.size();
    }
    void printInfo(ostream& s) const
{ // Print identifying information about constructor
  // for use in error messages
    s << "table \"" << subtableGetName(parent);
    s << "\" constructor starting at line " << dec << lineno;
}
    void print(ostream& s, ParserWalker& walker) const
{
    vector<string>::const_iterator piter;

    for (piter = printpiece.begin(); piter != printpiece.end(); ++piter) {
        if ((*piter)[0] == '\n') {
            int4 index = (*piter)[1] - 'A';
            operands[index]->print(s, walker);
        } else
            s << *piter;
    }
}
    void printMnemonic(ostream& s, ParserWalker& walker) const
{
    if (constructorTryPrintMnemonic(*this, s, walker))
        return;
    int4 endind = (firstwhitespace == -1) ? printpiece.size() : firstwhitespace;
    for (int4 i = 0; i < endind; ++i) {
        if (printpiece[i][0] == '\n') {
            int4 index = printpiece[i][1] - 'A';
            operands[index]->print(s, walker);
        } else
            s << printpiece[i];
    }
}
    void printBody(ostream& s, ParserWalker& walker) const
{
    if (constructorTryPrintBody(*this, s, walker))
        return;
    if (firstwhitespace == -1)
        return; // Nothing to print after firstwhitespace
    for (int4 i = firstwhitespace + 1; i < printpiece.size(); ++i) {
        if (printpiece[i][0] == '\n') {
            int4 index = printpiece[i][1] - 'A';
            operands[index]->print(s, walker);
        } else
            s << printpiece[i];
    }
}
    void removeTrailingSpace(void)
{
    // Allow for user to force extra space at end of printing
    if ((!printpiece.empty()) && (printpiece.back() == " "))
        printpiece.pop_back();
    //  while((!printpiece.empty())&&(printpiece.back()==" "))
    //    printpiece.pop_back();
}
    void applyContext(ParserWalkerChange& walker) const {
        vector<ContextChange*>::const_iterator iter;
        for (iter = context.begin(); iter != context.end(); ++iter)
            (*iter)->apply(walker);
    }
    void markSubtableOperands(vector<int4>& check) const
{ // Adjust -check- so it has one entry for every operand, a 0 if it is a subtable, a 2 if it is not
    check.resize(operands.size());
    for (int4 i = 0; i < operands.size(); ++i) {
        TripleSymbol* sym = operands[i]->getDefiningSymbol();
        if ((sym != (TripleSymbol*)0) && (sym->getType() == SleighSymbol::subtable_symbol))
            check[i] = 0;
        else
            check[i] = 2;
    }
}
    void collectLocalExports(vector<uintb>& results) const
{
    if (templ == (ConstructTpl*)0)
        return;
    HandleTpl* handle = templ->getResult();
    if (handle == (HandleTpl*)0)
        return;
    if (handle->getSpace().isConstSpace())
        return; // Even if the value is dynamic, the pointed to value won't get used
    if (handle->getPtrSpace().getType() != ConstTpl::real) {
        if (handle->getTempSpace().isUniqueSpace())
            results.push_back(handle->getTempOffset().getReal());
        return;
    }
    if (handle->getSpace().isUniqueSpace()) {
        results.push_back(handle->getPtrOffset().getReal());
        return;
    }
    if (handle->getSpace().getType() == ConstTpl::handle) {
        int4 handleIndex = handle->getSpace().getHandleIndex();
        OperandSymbol* opSym = getOperand(handleIndex);
        opSym->collectLocalValues(results);
    }
}
    void setError(bool val) const {
        inerror = val;
    }
    bool isError(void) const {
        return inerror;
    }
    bool isRecursive(void) const
{ // Does this constructor cause recursion with its table
    return constructorIsRecursive(*this);
}
    void encode(Encoder& encoder) const
{
    encoder.openElement(sla::ELEM_CONSTRUCTOR);
    encoder.writeUnsignedInteger(sla::ATTRIB_PARENT, subtableGetId(parent));
    encoder.writeSignedInteger(sla::ATTRIB_FIRST, firstwhitespace);
    encoder.writeSignedInteger(sla::ATTRIB_LENGTH, minimumlength);
    encoder.writeSignedInteger(sla::ATTRIB_SOURCE, src_index);
    encoder.writeSignedInteger(sla::ATTRIB_LINE, lineno);
    for (int4 i = 0; i < operands.size(); ++i) {
        encoder.openElement(sla::ELEM_OPER);
        encoder.writeUnsignedInteger(sla::ATTRIB_ID, operands[i]->getId());
        encoder.closeElement(sla::ELEM_OPER);
    }
    for (int4 i = 0; i < printpiece.size(); ++i) {
        if (printpiece[i][0] == '\n') {
            int4 index = printpiece[i][1] - 'A';
            encoder.openElement(sla::ELEM_OPPRINT);
            encoder.writeSignedInteger(sla::ATTRIB_ID, index);
            encoder.closeElement(sla::ELEM_OPPRINT);
        } else {
            encoder.openElement(sla::ELEM_PRINT);
            encoder.writeString(sla::ATTRIB_PIECE, printpiece[i]);
            encoder.closeElement(sla::ELEM_PRINT);
        }
    }
    for (int4 i = 0; i < context.size(); ++i)
        context[i]->encode(encoder);
    if (templ != (ConstructTpl*)0)
        templ->encode(encoder, -1);
    for (int4 i = 0; i < namedtempl.size(); ++i) {
        if (namedtempl[i] == (ConstructTpl*)0) // Some sections may be NULL
            continue;
        namedtempl[i]->encode(encoder, i);
    }
    encoder.closeElement(sla::ELEM_CONSTRUCTOR);
}
    void decode(Decoder& decoder, SleighBase* trans)
{
    uint4 el = decoder.openElement(sla::ELEM_CONSTRUCTOR);
    uintm id = decoder.readUnsignedInteger(sla::ATTRIB_PARENT);
    parent = (SubtableSymbol*)sleighBaseFindSymbol(trans, id);
    firstwhitespace = decoder.readSignedInteger(sla::ATTRIB_FIRST);
    minimumlength = decoder.readSignedInteger(sla::ATTRIB_LENGTH);
    src_index = decoder.readSignedInteger(sla::ATTRIB_SOURCE);
    lineno = decoder.readSignedInteger(sla::ATTRIB_LINE);
    uint4 subel = decoder.peekElement();
    while (subel != 0) {
        if (subel == sla::ELEM_OPER) {
            decoder.openElement();
            uintm id = decoder.readUnsignedInteger(sla::ATTRIB_ID);
            OperandSymbol* sym = (OperandSymbol*)sleighBaseFindSymbol(trans, id);
            operands.push_back(sym);
            decoder.closeElement(subel);
        } else if (subel == sla::ELEM_PRINT) {
            decoder.openElement();
            printpiece.push_back(decoder.readString(sla::ATTRIB_PIECE));
            decoder.closeElement(subel);
        } else if (subel == sla::ELEM_OPPRINT) {
            decoder.openElement();
            int4 index = decoder.readSignedInteger(sla::ATTRIB_ID);
            string operstring = "\n ";
            operstring[1] = ('A' + index);
            printpiece.push_back(operstring);
            decoder.closeElement(subel);
        } else if (subel == sla::ELEM_CONTEXT_OP) {
            ContextOp* c_op = new ContextOp();
            context.push_back(c_op);
            c_op->decode(decoder, trans);
        } else if (subel == sla::ELEM_COMMIT) {
            ContextCommit* c_op = new ContextCommit();
            context.push_back(c_op);
            c_op->decode(decoder, trans);
        } else {
            unique_ptr<ConstructTpl> cur(new ConstructTpl());
            int4 sectionid = cur->decode(decoder);
            if (sectionid < 0) {
                if (templ != (ConstructTpl*)0)
                    throw LowlevelError("Duplicate main section");
                templ = cur.release();
            } else {
                while (namedtempl.size() <= sectionid)
                    namedtempl.push_back((ConstructTpl*)0);
                if (namedtempl[sectionid] != (ConstructTpl*)0)
                    throw LowlevelError("Duplicate named section");
                namedtempl[sectionid] = cur.release();
            }
        }
        subel = decoder.peekElement();
    }
    pattern = (TokenPattern*)0;
    if ((printpiece.size() == 1) && (printpiece[0][0] == '\n'))
        flowthruindex = printpiece[0][1] - 'A';
    else
        flowthruindex = -1;
    decoder.closeElement(el);
}
};

class DecisionProperties {
    vector<pair<Constructor*, Constructor*>> identerrors;
    vector<pair<Constructor*, Constructor*>> conflicterrors;

public:
    void identicalPattern(Constructor* a, Constructor* b)
{ // Note that -a- and -b- have identical patterns
    if ((!a->isError()) && (!b->isError())) {
        a->setError(true);
        b->setError(true);

        identerrors.push_back(make_pair(a, b));
    }
}
    void conflictingPattern(Constructor* a, Constructor* b)
{ // Note that -a- and -b- have (potentially) conflicting patterns
    if ((!a->isError()) && (!b->isError())) {
        a->setError(true);
        b->setError(true);

        conflicterrors.push_back(make_pair(a, b));
    }
}
    const vector<pair<Constructor*, Constructor*>>& getIdentErrors(void) const {
        return identerrors;
    }
    const vector<pair<Constructor*, Constructor*>>& getConflictErrors(void) const {
        return conflicterrors;
    }
};

class DecisionNode {
    vector<pair<DisjointPattern*, Constructor*>> list;
    vector<DecisionNode*> children;
    int4 num;               // Total number of patterns we distinguish
    bool contextdecision;   // True if this is decision based on context
    int4 startbit, bitsize; // Bits in the stream on which to base the decision
    DecisionNode* parent;
    void chooseOptimalField(void)
{
    double score = 0.0;

    int4 sbit, size; // The current field
    bool context;
    double sc;

    int4 maxlength, numfixed, maxfixed;

    maxfixed = 1;
    context = true;
    do {
        maxlength = 8 * getMaximumLength(context);
        for (sbit = 0; sbit < maxlength; ++sbit) {
            numfixed = getNumFixed(sbit, 1, context); // How may patterns specify this bit
            if (numfixed < maxfixed)
                continue; // Skip this bit, if we don't have maximum specification
            sc = getScore(sbit, 1, context);

            // if we got more patterns this time than previously, and a positive score, reset
            // the high score (we prefer this bit, because it has a higher numfixed, regardless
            // of the difference in score, as long as the new score is positive).
            if ((numfixed > maxfixed) && (sc > 0.0)) {
                score = sc;
                maxfixed = numfixed;
                startbit = sbit;
                bitsize = 1;
                contextdecision = context;
                continue;
            }
            // We have maximum patterns
            if (sc > score) {
                score = sc;
                startbit = sbit;
                bitsize = 1;
                contextdecision = context;
            }
        }
        context = !context;
    } while (!context);

    context = true;
    do {
        maxlength = 8 * getMaximumLength(context);
        for (size = 2; size <= 8; ++size) {
            for (sbit = 0; sbit < maxlength - size + 1; ++sbit) {
                if (getNumFixed(sbit, size, context) < maxfixed)
                    continue; // Consider only maximal fields
                sc = getScore(sbit, size, context);
                if (sc > score) {
                    score = sc;
                    startbit = sbit;
                    bitsize = size;
                    contextdecision = context;
                }
            }
        }
        context = !context;
    } while (!context);
    if (score <= 0.0) // If we failed to get a positive score
        bitsize = 0;  // treat the node as terminal
}
    double getScore(int4 low, int4 size, bool context)
{
    int4 numBins = 1 << size; // size is between 1 and 8
    int4 i;
    uintm val, mask;
    uintm m = ((uintm)1) << size;
    m = m - 1;

    int4 total = 0;
    vector<int4> count(numBins, 0);

    for (i = 0; i < list.size(); ++i) {
        mask = list[i].first->getMask(low, size, context);
        if ((mask & m) != m)
            continue; // Skip if field not fully specified
        val = list[i].first->getValue(low, size, context);
        total += 1;
        count[val] += 1;
    }
    if (total <= 0)
        return -1.0;
    double sc = 0.0;
    for (i = 0; i < numBins; ++i) {
        if (count[i] <= 0)
            continue;
        if (count[i] >= list.size())
            return -1.0;
        double p = ((double)count[i]) / total;
        sc -= p * log(p);
    }
    return (sc / log(2.0));
}
    int4 getNumFixed(int4 low, int4 size, bool context)
{ // Get number of patterns that specify this field
    int4 count = 0;
    uintm mask;
    // Bits which must be specified in the mask
    uintm m = (size == 8 * sizeof(uintm)) ? 0 : (((uintm)1) << size);
    m = m - 1;

    for (int4 i = 0; i < list.size(); ++i) {
        mask = list[i].first->getMask(low, size, context);
        if ((mask & m) == m)
            count += 1;
    }
    return count;
}
    int4 getMaximumLength(bool context)
{ // Get maximum length of instruction pattern in bytes
    int4 max = 0;
    int4 val, i;

    for (i = 0; i < list.size(); ++i) {
        val = list[i].first->getLength(context);
        if (val > max)
            max = val;
    }
    return max;
}
    void consistentValues(vector<uint4>& bins, DisjointPattern* pat)
{ // Produce all possible values of -pat- by
  // iterating through all possible values of the
  // "don't care" bits within the value of -pat-
  // that intersects with this node (startbit,bitsize,context)
    uintm m = (bitsize == 8 * sizeof(uintm)) ? 0 : (((uintm)1) << bitsize);
    m = m - 1;
    uintm commonMask = m & pat->getMask(startbit, bitsize, contextdecision);
    uintm commonValue = commonMask & pat->getValue(startbit, bitsize, contextdecision);
    uintm dontCareMask = m ^ commonMask;

    for (uintm i = 0; i <= dontCareMask; ++i) { // Iterate over values that contain all don't care bits
        if ((i & dontCareMask) != i)
            continue;                    // If all 1 bits in the value are don't cares
        bins.push_back(commonValue | i); // add 1 bits into full value and store
    }
}

public:
    DecisionNode(void) {} // For use with decode
    DecisionNode(DecisionNode* p)
{
    parent = p;
    num = 0;
    startbit = 0;
    bitsize = 0;
    contextdecision = false;
}
    ~DecisionNode(void)
{ // We own sub nodes
    vector<DecisionNode*>::iterator iter;
    for (iter = children.begin(); iter != children.end(); ++iter)
        delete *iter;
    vector<pair<DisjointPattern*, Constructor*>>::iterator piter;
    for (piter = list.begin(); piter != list.end(); ++piter)
        delete (*piter).first; // Delete the patterns
}
    Constructor* resolve(ParserWalker& walker) const
{
    if (bitsize == 0) { // The node is terminal
        vector<pair<DisjointPattern*, Constructor*>>::const_iterator iter;
        for (iter = list.begin(); iter != list.end(); ++iter)
            if ((*iter).first->isMatch(walker))
                return (*iter).second;
        ostringstream s;
        s << walker.getAddr().getShortcut();
        walker.getAddr().printRaw(s);
        s << ": Unable to resolve constructor";
        throw BadDataError(s.str());
    }
    uintm val;
    if (contextdecision)
        val = walker.getContextBits(startbit, bitsize);
    else
        val = walker.getInstructionBits(startbit, bitsize);
    return children[val]->resolve(walker);
}
    void addConstructorPair(const DisjointPattern* pat, Constructor* ct)
{
    DisjointPattern* clone = (DisjointPattern*)pat->simplifyClone(); // We need to own pattern
    list.push_back(pair<DisjointPattern*, Constructor*>(clone, ct));
    num += 1;
}
    void split(DecisionProperties& props)
{
    if (list.size() <= 1) {
        bitsize = 0; // Only one pattern, terminal node by default
        return;
    }

    chooseOptimalField();
    if (bitsize == 0) {
        orderPatterns(props);
        return;
    }
    if ((parent != (DecisionNode*)0) && (list.size() >= parent->num))
        throw LowlevelError("Child has as many Patterns as parent");

    int4 numChildren = 1 << bitsize;

    for (int4 i = 0; i < numChildren; ++i) {
        DecisionNode* nd = new DecisionNode(this);
        children.push_back(nd);
    }
    for (int4 i = 0; i < list.size(); ++i) {
        vector<uint4> vals; // Bins this pattern belongs in
                            // If the pattern does not care about some
                            // bits in the field we are splitting on, that
                            // pattern will get put into multiple bins
        consistentValues(vals, list[i].first);
        for (int4 j = 0; j < vals.size(); ++j)
            children[vals[j]]->addConstructorPair(list[i].first, list[i].second);
        delete list[i].first; // We no longer need original pattern
    }
    list.clear();

    for (int4 i = 0; i < numChildren; ++i)
        children[i]->split(props);
}
    void orderPatterns(DecisionProperties& props)
{
    // This is a tricky routine.  When this routine is called, the patterns remaining in the
    // the decision node can no longer be distinguished by examining additional bits. The basic
    // idea here is that the patterns should be ordered so that the most specialized should come
    // first in the list. Pattern 1 is a specialization of pattern 2, if the set of instructions
    // matching 1 is contained in the set matching 2.  So in the simplest case, the pattern order
    // should represent a strict nesting.  Unfortunately, there are many potential situations where
    // patterns don't necessarily nest.
    //   1) An "or" of two patterns.  This can be an explicit '|' operator in the Constructor, in
    //      which case this can be detected because the two patterns point to the same constructor
    //      But the "or" can be implied across two constructors that do the same thing.  This should
    //      probably be flagged as an error except in the following case.
    //   2) Two patterns aren't properly nested, but they are "resolved" by a third pattern which
    //      covers the intersection of the first two patterns.  Sometimes its easier to specify
    //      three cases that need to be distinguished in this way.
    //   3) Recursive constructors that use a "guard" context bit.  The guard bit is used to prevent
    //      the recursive constructor from matching repeatedly, but it's too much work to put a
    //      constraint an the bit for every other pattern.
    //   4) Other situations where the ability to distinguish between constructors is hidden in
    //      the subconstructors.
    // This routine can determine if an intersection results from case 1) or case 2)
    int4 i, j, k;
    vector<pair<DisjointPattern*, Constructor*>> newlist;
    vector<pair<DisjointPattern*, Constructor*>> conflictlist;

    // Check for identical patterns
    for (i = 0; i < list.size(); ++i) {
        for (j = 0; j < i; ++j) {
            DisjointPattern* ipat = list[i].first;
            DisjointPattern* jpat = list[j].first;
            if (ipat->identical(jpat))
                props.identicalPattern(list[i].second, list[j].second);
        }
    }

    newlist = list;
    for (i = 0; i < list.size(); ++i) {
        for (j = 0; j < i; ++j) {
            DisjointPattern* ipat = newlist[i].first;
            DisjointPattern* jpat = list[j].first;
            if (ipat->specializes(jpat))
                break;
            if (!jpat->specializes(ipat)) { // We have a potential conflict
                Constructor* iconst = newlist[i].second;
                Constructor* jconst = list[j].second;
                if (iconst == jconst) { // This is an OR in the pattern for ONE constructor
                                        // So there is no conflict
                } else {                // A true conflict that needs to be resolved
                    conflictlist.push_back(pair<DisjointPattern*, Constructor*>(ipat, iconst));
                    conflictlist.push_back(pair<DisjointPattern*, Constructor*>(jpat, jconst));
                }
            }
        }
        for (k = i - 1; k >= j; --k)
            list[k + 1] = list[k];
        list[j] = newlist[i];
    }

    // Check if intersection patterns are present, which resolve conflicts
    for (i = 0; i < conflictlist.size(); i += 2) {
        DisjointPattern *pat1, *pat2;
        Constructor *const1, *const2;
        pat1 = conflictlist[i].first;
        const1 = conflictlist[i].second;
        pat2 = conflictlist[i + 1].first;
        const2 = conflictlist[i + 1].second;
        bool resolved = false;
        for (j = 0; j < list.size(); ++j) {
            DisjointPattern* tpat = list[j].first;
            Constructor* tconst = list[j].second;
            if ((tpat == pat1) && (tconst == const1))
                break; // Ran out of possible specializations
            if ((tpat == pat2) && (tconst == const2))
                break;
            if (tpat->resolvesIntersect(pat1, pat2)) {
                resolved = true;
                break;
            }
        }
        if (!resolved)
            props.conflictingPattern(const1, const2);
    }
}
    void encode(Encoder& encoder) const
{
    encoder.openElement(sla::ELEM_DECISION);
    encoder.writeSignedInteger(sla::ATTRIB_NUMBER, num);
    encoder.writeBool(sla::ATTRIB_CONTEXT, contextdecision);
    encoder.writeSignedInteger(sla::ATTRIB_STARTBIT, startbit);
    encoder.writeSignedInteger(sla::ATTRIB_SIZE, bitsize);
    for (int4 i = 0; i < list.size(); ++i) {
        encoder.openElement(sla::ELEM_PAIR);
        encoder.writeSignedInteger(sla::ATTRIB_ID, list[i].second->getId());
        list[i].first->encode(encoder);
        encoder.closeElement(sla::ELEM_PAIR);
    }
    for (int4 i = 0; i < children.size(); ++i)
        children[i]->encode(encoder);
    encoder.closeElement(sla::ELEM_DECISION);
}
    void decode(Decoder& decoder, DecisionNode* par, SubtableSymbol* sub)
{
    uint4 el = decoder.openElement(sla::ELEM_DECISION);
    parent = par;
    num = decoder.readSignedInteger(sla::ATTRIB_NUMBER);
    contextdecision = decoder.readBool(sla::ATTRIB_CONTEXT);
    startbit = decoder.readSignedInteger(sla::ATTRIB_STARTBIT);
    bitsize = decoder.readSignedInteger(sla::ATTRIB_SIZE);
    uint4 subel = decoder.peekElement();
    while (subel != 0) {
        if (subel == sla::ELEM_PAIR) {
            decoder.openElement();
            uintm id = decoder.readSignedInteger(sla::ATTRIB_ID);
            if (id >= subtableGetNumConstructors(sub)) {
                throw DecoderError("Invalid constructor id");
            }
            Constructor* ct = subtableGetConstructor(sub, id);
            DisjointPattern* pat = decodeDisjointPattern(decoder);
            list.push_back(pair<DisjointPattern*, Constructor*>(pat, ct));
            decoder.closeElement(subel);
        } else if (subel == sla::ELEM_DECISION) {
            DecisionNode* subnode = new DecisionNode();
            children.push_back(subnode);
            subnode->decode(decoder, this, sub);
        }
        subel = decoder.peekElement();
    }
    decoder.closeElement(el);
}
};

class SubtableSymbol : public TripleSymbol {
    TokenPattern* pattern;
    bool beingbuilt, errors;
    vector<Constructor*> construct; // All the Constructors in this table
    DecisionNode* decisiontree;

public:
    SubtableSymbol(void) {
        pattern = (TokenPattern*)0;
        decisiontree = (DecisionNode*)0;
    } // For use with decode
    SubtableSymbol(const string& nm)
    : TripleSymbol(nm)
{
    beingbuilt = false;
    pattern = (TokenPattern*)0;
    decisiontree = (DecisionNode*)0;
    errors = 0;
}
    virtual ~SubtableSymbol(void)
{
    if (pattern != (TokenPattern*)0)
        delete pattern;
    if (decisiontree != (DecisionNode*)0)
        delete decisiontree;
    vector<Constructor*>::iterator iter;
    for (iter = construct.begin(); iter != construct.end(); ++iter)
        delete *iter;
}
    bool isBeingBuilt(void) const {
        return beingbuilt;
    }
    bool isError(void) const {
        return errors;
    }
    void addConstructor(Constructor* ct) {
        ct->setId(construct.size());
        construct.push_back(ct);
    }
    void buildDecisionTree(DecisionProperties& props)
{ // Associate pattern disjoints to constructors
    if (pattern == (TokenPattern*)0)
        return; // Pattern not fully formed
    Pattern* pat;
    decisiontree = new DecisionNode((DecisionNode*)0);
    for (int4 i = 0; i < construct.size(); ++i) {
        pat = construct[i]->getPattern()->getPattern();
        if (pat->numDisjoint() == 0)
            decisiontree->addConstructorPair((const DisjointPattern*)pat, construct[i]);
        else
            for (int4 j = 0; j < pat->numDisjoint(); ++j)
                decisiontree->addConstructorPair(pat->getDisjoint(j), construct[i]);
    }
    decisiontree->split(props); // Create the decision strategy
}
    TokenPattern* buildPattern(ostream& s)
{
    if (pattern != (TokenPattern*)0)
        return pattern; // Already built

    errors = false;
    beingbuilt = true;
    pattern = new TokenPattern();
    if (construct.empty()) {
        s << "Error: There are no constructors in table: " + getName() << endl;
        errors = true;
        return pattern;
    }
    try {
        construct.front()->buildPattern(s);
    } catch (SleighError& err) {
        s << "Error: " << err.explain << ": for ";
        construct.front()->printInfo(s);
        s << endl;
        errors = true;
    }
    *pattern = *construct.front()->getPattern();
    for (int4 i = 1; i < construct.size(); ++i) {
        try {
            construct[i]->buildPattern(s);
        } catch (SleighError& err) {
            s << "Error: " << err.explain << ": for ";
            construct[i]->printInfo(s);
            s << endl;
            errors = true;
        }
        *pattern = construct[i]->getPattern()->commonSubPattern(*pattern);
    }
    beingbuilt = false;
    return pattern;
}
    TokenPattern* getPattern(void) const {
        return pattern;
    }
    int4 getNumConstructors(void) const {
        return construct.size();
    }
    Constructor* getConstructor(uintm id) const {
        return construct[id];
    }
    virtual Constructor* resolve(ParserWalker& walker) {
        return decisiontree->resolve(walker);
    }
    virtual PatternExpression* getPatternExpression(void) const {
        throw SleighError("Cannot use subtable in expression");
    }
    virtual void getFixedHandle(FixedHandle& hand, ParserWalker& walker) const {
        throw SleighError("Cannot use subtable in expression");
    }
    virtual int4 getSize(void) const {
        return -1;
    }
    virtual void print(ostream& s, ParserWalker& walker) const {
        throw SleighError("Cannot use subtable in expression");
    }
    virtual void collectLocalValues(vector<uintb>& results) const
{
    for (int4 i = 0; i < construct.size(); ++i)
        construct[i]->collectLocalExports(results);
}
    virtual symbol_type getType(void) const {
        return subtable_symbol;
    }
    virtual void encode(Encoder& encoder) const
{
    if (decisiontree == (DecisionNode*)0)
        return; // Not fully formed
    encoder.openElement(sla::ELEM_SUBTABLE_SYM);
    encoder.writeUnsignedInteger(sla::ATTRIB_ID, getId());
    encoder.writeSignedInteger(sla::ATTRIB_NUMCT, construct.size());
    for (int4 i = 0; i < construct.size(); ++i)
        construct[i]->encode(encoder);
    decisiontree->encode(encoder);
    encoder.closeElement(sla::ELEM_SUBTABLE_SYM);
}
    virtual void encodeHeader(Encoder& encoder) const
{
    encoder.openElement(sla::ELEM_SUBTABLE_SYM_HEAD);
    SleighSymbol::encodeHeader(encoder);
    encoder.closeElement(sla::ELEM_SUBTABLE_SYM_HEAD);
}
    virtual void decode(Decoder& decoder, SleighBase* trans)
{
    int4 numct = decoder.readSignedInteger(sla::ATTRIB_NUMCT);
    construct.reserve(numct);
    uint4 subel = decoder.peekElement();
    while (subel != 0) {
        if (subel == sla::ELEM_CONSTRUCTOR) {
            Constructor* ct = new Constructor();
            addConstructor(ct);
            ct->decode(decoder, trans);
        } else if (subel == sla::ELEM_DECISION) {
            decisiontree = new DecisionNode();
            decisiontree->decode(decoder, (DecisionNode*)0, this);
        }
        subel = decoder.peekElement();
    }
    pattern = (TokenPattern*)0;
    beingbuilt = false;
    errors = 0;
    decoder.closeElement(sla::ELEM_SUBTABLE_SYM.getId());
}
};

class MacroSymbol : public SleighSymbol { // A user-defined pcode-macro
    int4 index;
    ConstructTpl* construct;
    vector<OperandSymbol*> operands;

public:
    MacroSymbol(const string& nm, int4 i) : SleighSymbol(nm) {
        index = i;
        construct = (ConstructTpl*)0;
    }
    int4 getIndex(void) const {
        return index;
    }
    void setConstruct(ConstructTpl* ct) {
        construct = ct;
    }
    ConstructTpl* getConstruct(void) const {
        return construct;
    }
    void addOperand(OperandSymbol* sym) {
        operands.push_back(sym);
    }
    int4 getNumOperands(void) const {
        return operands.size();
    }
    OperandSymbol* getOperand(int4 i) const {
        return operands[i];
    }
    virtual ~MacroSymbol(void) {
        if (construct != (ConstructTpl*)0)
            delete construct;
    }
    virtual symbol_type getType(void) const {
        return macro_symbol;
    }
};

class LabelSymbol : public SleighSymbol { // A branch label
    uint4 index;                          // Local 1 up index of label
    bool isplaced;                        // Has the label been placed (not just referenced)
    uint4 refcount;                       // Number of references to this label
public:
    LabelSymbol(const string& nm, uint4 i) : SleighSymbol(nm) {
        index = i;
        refcount = 0;
        isplaced = false;
    }
    uint4 getIndex(void) const {
        return index;
    }
    void incrementRefCount(void) {
        refcount += 1;
    }
    uint4 getRefCount(void) const {
        return refcount;
    }
    void setPlaced(void) {
        isplaced = true;
    }
    bool isPlaced(void) const {
        return isplaced;
    }
    virtual symbol_type getType(void) const {
        return label_symbol;
    }
};

/// Initializes a walker from a constructor and operand once symbol definitions
/// are available, avoiding a cyclic import from context to slghsymbol.
void parserWalkerSetOutOfBandState(ParserWalker& walker, Constructor* constructor, int4 index,
                                          ConstructState* temporaryState, const ParserWalker& otherWalker) {
    const ConstructState* point = otherWalker.point;
    int4 depth = otherWalker.depth;
    while (point->ct != constructor) {
        if (depth <= 0)
            return;
        depth -= 1;
        point = point->parent;
    }
    OperandSymbol* symbol = constructor->getOperand(index);
    int4 offsetBase = symbol->getOffsetBase();
    if (offsetBase < 0)
        temporaryState->offset = point->offset + symbol->getRelativeOffset();
    else
        temporaryState->offset = point->resolve[index]->offset;

    temporaryState->ct = constructor;
    temporaryState->length = point->length;
    walker.point = temporaryState;
    walker.depth = 0;
    walker.breadcrumb[0] = 0;
}

/// Applies pending context changes after the complete symbol hierarchy is
/// available to resolve operands and fixed handles.
void parserContextApplyCommits(ParserContext& context) {
    if (context.contextcommit.empty())
        return;
    ParserWalker walker(&context);
    walker.baseState();

    for (vector<ContextSet>::iterator iter = context.contextcommit.begin(); iter != context.contextcommit.end(); ++iter) {
        TripleSymbol* symbol = (*iter).sym;
        Address commitAddress;
        if (symbol->getType() == SleighSymbol::operand_symbol) {
            int4 index = ((OperandSymbol*)symbol)->getIndex();
            FixedHandle& handle((*iter).point->resolve[index]->hand);
            commitAddress = Address(handle.space, handle.offset_offset);
        } else {
            FixedHandle handle;
            symbol->getFixedHandle(handle, walker);
            commitAddress = Address(handle.space, handle.offset_offset);
        }
        if (commitAddress.isConstant()) {
            uintb newOffset = AddrSpace::addressToByte(commitAddress.getOffset(), context.addr.getSpace()->getWordSize());
            commitAddress = Address(context.addr.getSpace(), newOffset);
        }

        if ((*iter).flow)
            context.contcache->setContext(commitAddress, (*iter).num, (*iter).mask, (*iter).value);
        else {
            Address nextAddress = commitAddress + 1;
            if (nextAddress.getOffset() < commitAddress.getOffset())
                context.contcache->setContext(commitAddress, (*iter).num, (*iter).mask, (*iter).value);
            else
                context.contcache->setContext(commitAddress, nextAddress, (*iter).num, (*iter).mask, (*iter).value);
        }
    }
}

/// Resolves whether an operand's offset is relative to its constructor.
bool operandValueIsConstructorRelative(const OperandValue& value) {
    OperandSymbol* symbol = value.ct->getOperand(value.index);
    return symbol->getOffsetBase() == -1;
}

/// Returns the source-level name of an operand value.
const string& operandValueGetName(const OperandValue& value) {
    return value.ct->getOperand(value.index)->getName();
}

/// Evaluates an operand's defining pattern expression in a simulated walker.
intb operandValueGetValue(const OperandValue& value, ParserWalker& walker) {
    OperandSymbol* symbol = value.ct->getOperand(value.index);
    PatternExpression* expression = symbol->getDefiningExpression();
    if (expression == (PatternExpression*)0) {
        TripleSymbol* definingSymbol = symbol->getDefiningSymbol();
        if (definingSymbol != (TripleSymbol*)0)
            expression = definingSymbol->getPatternExpression();
        if (expression == (PatternExpression*)0)
            return 0;
    }
    ConstructState temporaryState;
    ParserWalker newWalker(walker.getParserContext());
    newWalker.setOutOfBandState(value.ct, value.index, &temporaryState, walker);
    return expression->getValue(newWalker);
}

/// Returns a selected subvalue from an operand's defining expression.
intb operandValueGetSubValue(const OperandValue& value, const vector<intb>& replace, int4& listpos) {
    return value.ct->getOperand(value.index)->getDefiningExpression()->getSubValue(replace, listpos);
}

/// Encodes an operand expression reference and its constructor identity.
void operandValueEncode(const OperandValue& value, Encoder& encoder) {
    encoder.openElement(sla::ELEM_OPERAND_EXP);
    encoder.writeSignedInteger(sla::ATTRIB_INDEX, value.index);
    encoder.writeUnsignedInteger(sla::ATTRIB_TABLE, value.ct->getParent()->getId());
    encoder.writeUnsignedInteger(sla::ATTRIB_CT, value.ct->getId());
    encoder.closeElement(sla::ELEM_OPERAND_EXP);
}

/// Resolves the relative placement of one operand in a pattern equation.
bool operandEquationResolve(OperandResolve& state, int4 index) {
    OperandSymbol* symbol = state.operands[index];
    if (symbol->isOffsetIrrelevant()) {
        symbol->offsetbase = -1;
        symbol->reloffset = 0;
        return true;
    }
    if (state.base == -2)
        return false;
    symbol->offsetbase = state.base;
    symbol->reloffset = state.offset;
    state.cur_rightmost = index;
    state.size = 0;
    return true;
}

/// Appends an operand to the left-to-right symbol order exactly once.
void operandEquationOrder(Constructor* constructor, int4 index, vector<OperandSymbol*>* order) {
    OperandSymbol* symbol = constructor->getOperand(index);
    if (!symbol->isMarked()) {
        order->push_back(symbol);
        symbol->setMark();
    }
}

/// Restores a concrete symbol shell after all symbol subclasses are defined.
void symbolTableDecodeSymbolHeader(SymbolTable& table, Decoder& decoder) {
    unique_ptr<SleighSymbol> symbol;
    uint4 element = decoder.peekElement();
    if (element == sla::ELEM_USEROP_HEAD)
        symbol.reset(new UserOpSymbol());
    else if (element == sla::ELEM_EPSILON_SYM_HEAD)
        symbol.reset(new EpsilonSymbol());
    else if (element == sla::ELEM_VALUE_SYM_HEAD)
        symbol.reset(new ValueSymbol());
    else if (element == sla::ELEM_VALUEMAP_SYM_HEAD)
        symbol.reset(new ValueMapSymbol());
    else if (element == sla::ELEM_NAME_SYM_HEAD)
        symbol.reset(new NameSymbol());
    else if (element == sla::ELEM_VARNODE_SYM_HEAD)
        symbol.reset(new VarnodeSymbol());
    else if (element == sla::ELEM_CONTEXT_SYM_HEAD)
        symbol.reset(new ContextSymbol());
    else if (element == sla::ELEM_VARLIST_SYM_HEAD)
        symbol.reset(new VarnodeListSymbol());
    else if (element == sla::ELEM_OPERAND_SYM_HEAD)
        symbol.reset(new OperandSymbol());
    else if (element == sla::ELEM_START_SYM_HEAD)
        symbol.reset(new StartSymbol());
    else if (element == sla::ELEM_END_SYM_HEAD)
        symbol.reset(new EndSymbol());
    else if (element == sla::ELEM_NEXT2_SYM_HEAD)
        symbol.reset(new Next2Symbol());
    else if (element == sla::ELEM_SUBTABLE_SYM_HEAD)
        symbol.reset(new SubtableSymbol());
    else
        throw SleighError("Bad symbol xml");

    symbol->decodeHeader(decoder);
    if (symbol->id >= table.symbollist.size())
        throw SleighError("Bad symbol id: exceeds symbollist table size");
    if (table.symbollist[symbol->id] != (SleighSymbol*)0)
        throw SleighError("Bad symbol id: not unique");
    if (symbol->scopeid >= table.table.size())
        throw SleighError("Bad symbol scope id: too large");
    if (table.table[symbol->scopeid] == (SymbolScope*)0)
        throw SleighError("Bad symbol scope id: undefined");

    SleighSymbol* result = symbol.release();
    table.symbollist[result->id] = result;
    table.table[result->scopeid]->addSymbol(result);
}

/// Removes unsavable symbols and empty scopes after all symbol subclasses are defined.
void symbolTablePurge(SymbolTable& table) {
    for (int4 i = 0; i < table.symbollist.size(); ++i) {
        SleighSymbol* symbol = table.symbollist[i];
        if (symbol == (SleighSymbol*)0)
            continue;
        if (symbol->scopeid != 0) {
            if (symbol->getType() == SleighSymbol::operand_symbol)
                continue;
        } else {
            switch (symbol->getType()) {
                case SleighSymbol::space_symbol:
                case SleighSymbol::token_symbol:
                case SleighSymbol::epsilon_symbol:
                case SleighSymbol::section_symbol:
                case SleighSymbol::bitrange_symbol:
                    break;
                case SleighSymbol::macro_symbol: {
                    MacroSymbol* macro = (MacroSymbol*)symbol;
                    for (int4 j = 0; j < macro->getNumOperands(); ++j) {
                        SleighSymbol* operand = macro->getOperand(j);
                        table.table[operand->scopeid]->removeSymbol(operand);
                        table.symbollist[operand->id] = (SleighSymbol*)0;
                        delete operand;
                    }
                    break;
                }
                case SleighSymbol::subtable_symbol: {
                    SubtableSymbol* subtable = (SubtableSymbol*)symbol;
                    if (subtable->getPattern() != (TokenPattern*)0)
                        continue;
                    for (int4 j = 0; j < subtable->getNumConstructors(); ++j) {
                        Constructor* constructor = subtable->getConstructor(j);
                        for (int4 k = 0; k < constructor->getNumOperands(); ++k) {
                            OperandSymbol* operand = constructor->getOperand(k);
                            table.table[operand->scopeid]->removeSymbol(operand);
                            table.symbollist[operand->id] = (SleighSymbol*)0;
                            delete operand;
                        }
                    }
                    break;
                }
                default:
                    continue;
            }
        }
        table.table[symbol->scopeid]->removeSymbol(symbol);
        table.symbollist[i] = (SleighSymbol*)0;
        delete symbol;
    }
    for (int4 i = 1; i < table.table.size(); ++i) {
        if (table.table[i]->tree.empty()) {
            delete table.table[i];
            table.table[i] = (SymbolScope*)0;
        }
    }
    table.renumber();
}

/// Prints the current constructor after its complete definition is available.
void parserWalkerPrintConstructor(ParserWalker& walker, ostream& stream) {
    walker.getConstructor()->print(stream, walker);
}

/// Builds a subtable operand pattern once SubtableSymbol is complete.
bool constructorTrySubtablePattern(TripleSymbol* triple, ostream& stream, vector<TokenPattern>& patterns,
                                           bool& recursion) {
    SubtableSymbol* subtable = dynamic_cast<SubtableSymbol*>(triple);
    if (subtable == (SubtableSymbol*)0)
        return false;
    if (subtable->isBeingBuilt()) {
        if (recursion)
            throw SleighError("Illegal recursion");
        recursion = true;
        patterns.emplace_back();
    } else {
        patterns.push_back(*subtable->buildPattern(stream));
    }
    return true;
}

/// Handles flow-through constructor mnemonic printing.
bool constructorTryPrintMnemonic(const Constructor& constructor, ostream& stream, ParserWalker& walker) {
    if (constructor.flowthruindex == -1)
        return false;
    SubtableSymbol* subtable = dynamic_cast<SubtableSymbol*>(constructor.operands[constructor.flowthruindex]->getDefiningSymbol());
    if (subtable == (SubtableSymbol*)0)
        return false;
    walker.pushOperand(constructor.flowthruindex);
    walker.getConstructor()->printMnemonic(stream, walker);
    walker.popOperand();
    return true;
}

/// Handles flow-through constructor body printing.
bool constructorTryPrintBody(const Constructor& constructor, ostream& stream, ParserWalker& walker) {
    if (constructor.flowthruindex == -1)
        return false;
    SubtableSymbol* subtable = dynamic_cast<SubtableSymbol*>(constructor.operands[constructor.flowthruindex]->getDefiningSymbol());
    if (subtable == (SubtableSymbol*)0)
        return false;
    walker.pushOperand(constructor.flowthruindex);
    walker.getConstructor()->printBody(stream, walker);
    walker.popOperand();
    return true;
}

/// Returns the number of constructors in a complete subtable.
int4 subtableGetNumConstructors(const SubtableSymbol* subtable) {
    return subtable->getNumConstructors();
}

/// Returns one constructor from a complete subtable.
Constructor* subtableGetConstructor(const SubtableSymbol* subtable, int4 index) {
    return subtable->getConstructor(index);
}

/// Returns a complete subtable's name for constructor diagnostics.
const string& subtableGetName(const SubtableSymbol* subtable) {
    return subtable->getName();
}

/// Returns a complete subtable's symbol id for serialization.
uintm subtableGetId(const SubtableSymbol* subtable) {
    return subtable->getId();
}

/// Determines whether a constructor directly references its parent subtable.
bool constructorIsRecursive(const Constructor& constructor) {
    for (int4 i = 0; i < constructor.operands.size(); ++i) {
        TripleSymbol* symbol = constructor.operands[i]->getDefiningSymbol();
        if (symbol == constructor.parent)
            return true;
    }
    return false;
}

} // End namespace ghidra
#endif
