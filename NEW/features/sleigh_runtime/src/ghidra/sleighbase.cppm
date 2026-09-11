module;
#include <sstream>
#include <string>

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
/// \file sleighbase.cppm
/// \brief Base class for applications that process SLEIGH format specifications
#ifndef __SLEIGHBASE_HH__
#define __SLEIGHBASE_HH__

export module sleigh_runtime.ghidra:sleighbase;
export import :slaformat;
export import :slghsymbol;
export import :translate;

export namespace ghidra {

/// \brief class for recording source file information for SLEIGH constructors.

///
/// A SLEIGH specification may contain many source files.  This class is
/// used to associate each constructor in a SLEIGH language to the source
/// file where it is defined. This information is useful when debugging
/// SLEIGH specifications.  Sourcefiles are assigned a numeric index and
/// the mapping from indices to filenames is written to the generated .sla
/// file.  For each constructor, the data written to the .sla file includes
/// the source file index.
class SourceFileIndexer {
public:
    SourceFileIndexer() = default;
    ~SourceFileIndexer(void) {}
    /// Returns the index of the file.  If the file is not in the index it is added.
    void decode(Decoder& decoder)
{
    uint4 el = decoder.openElement(sla::ELEM_SOURCEFILES);
    while (decoder.peekElement() == sla::ELEM_SOURCEFILE) {
        int4 subel = decoder.openElement();
        string filename = decoder.readString(sla::ATTRIB_NAME);
        int4 index = decoder.readSignedInteger(sla::ATTRIB_INDEX);
        decoder.closeElement(subel);
        fileToIndex[filename] = index;
        indexToFile[index] = filename;
    }
    decoder.closeElement(el);
} ///< decode a stored index mapping from a stream

private:
    map<int4, string> indexToFile; ///< map from indices to files
    map<string, int4> fileToIndex; ///< map from files to indices
};

/// \brief Common core of classes that read or write SLEIGH specification files natively.

///
/// This class represents what's in common across the SLEIGH infrastructure between:
///   - Reading the various SLEIGH specification files
///   - Building and writing out SLEIGH specification files
class SleighBase : public Translate {
    vector<string> userop;                 ///< Names of user-define p-code ops for \b this Translate object
    map<VarnodeData, string> varnode_xref; ///< A map from Varnodes in the \e register space to register names
protected:
    SubtableSymbol* root;      ///< The root SLEIGH decoding symbol
    SymbolTable symtab;        ///< The SLEIGH symbol table
    uint4 maxdelayslotbytes;   ///< Maximum number of bytes in a delay-slot directive
    uint4 unique_allocatemask; ///< Bits that are guaranteed to be zero in the unique allocation scheme
    uint4 numSections;         ///< Number of \e named sections
    SourceFileIndexer indexer; ///< source file index used when generating SLEIGH constructor debug info
    void buildXrefs(vector<string>& errorPairs)
{
    SymbolScope* glb = symtab.getGlobalScope();
    SymbolTree::const_iterator iter;
    SleighSymbol* sym;
    ostringstream s;

    for (iter = glb->begin(); iter != glb->end(); ++iter) {
        sym = *iter;
        if (sym->getType() == SleighSymbol::varnode_symbol) {
            pair<VarnodeData, string> ins(((VarnodeSymbol*)sym)->getFixedVarnode(), sym->getName());
            pair<map<VarnodeData, string>::iterator, bool> res = varnode_xref.insert(ins);
            if (!res.second) {
                errorPairs.push_back(sym->getName());
                errorPairs.push_back((*(res.first)).second);
            }
        } else if (sym->getType() == SleighSymbol::userop_symbol) {
            int4 index = ((UserOpSymbol*)sym)->getIndex();
            while (userop.size() <= index)
                userop.push_back("");
            userop[index] = sym->getName();
        } else if (sym->getType() == SleighSymbol::context_symbol) {
            ContextSymbol* csym = (ContextSymbol*)sym;
            ContextField* field = (ContextField*)csym->getPatternValue();
            int4 startbit = field->getStartBit();
            int4 endbit = field->getEndBit();
            registerContext(csym->getName(), startbit, endbit);
        }
    }
} ///< Build register map. Collect user-ops and context-fields.
    void reregisterContext(void)
{
    SymbolScope* glb = symtab.getGlobalScope();
    SymbolTree::const_iterator iter;
    SleighSymbol* sym;
    for (iter = glb->begin(); iter != glb->end(); ++iter) {
        sym = *iter;
        if (sym->getType() == SleighSymbol::context_symbol) {
            ContextSymbol* csym = (ContextSymbol*)sym;
            ContextField* field = (ContextField*)csym->getPatternValue();
            int4 startbit = field->getStartBit();
            int4 endbit = field->getEndBit();
            registerContext(csym->getName(), startbit, endbit);
        }
    }
}                ///< Reregister context fields for a new executable
    AddrSpace* decodeSlaSpace(Decoder& decoder, const Translate* trans)
{
    uint4 elemId = decoder.openElement();
    AddrSpace* res;
    int4 index = 0;
    int4 addressSize = 0;
    int4 delay = -1;
    int4 deadcodedelay = -1;
    string name;
    int4 wordsize = 1;
    bool bigEnd = false;
    uint4 flags = 0;
    for (;;) {
        uint4 attribId = decoder.getNextAttributeId();
        if (attribId == 0)
            break;
        if (attribId == sla::ATTRIB_NAME) {
            name = decoder.readString();
        }
        if (attribId == sla::ATTRIB_INDEX)
            index = decoder.readSignedInteger();
        else if (attribId == sla::ATTRIB_SIZE)
            addressSize = decoder.readSignedInteger();
        else if (attribId == sla::ATTRIB_WORDSIZE)
            wordsize = decoder.readSignedInteger();
        else if (attribId == sla::ATTRIB_BIGENDIAN) {
            bigEnd = decoder.readBool();
        } else if (attribId == sla::ATTRIB_DELAY)
            delay = decoder.readSignedInteger();
        else if (attribId == sla::ATTRIB_PHYSICAL) {
            if (decoder.readBool())
                flags |= AddrSpace::hasphysical;
        }
    }
    decoder.closeElement(elemId);
    if (deadcodedelay == -1)
        deadcodedelay = delay; // If deadcodedelay attribute not present, set it to delay
    if (index == 0)
        throw LowlevelError("Expecting index attribute");
    if (elemId == sla::ELEM_SPACE_UNIQUE)
        res = new UniqueSpace(this, trans, index, flags);
    else if (elemId == sla::ELEM_SPACE_OTHER)
        res = new OtherSpace(this, trans, index);
    else {
        if (addressSize == 0 || delay == -1 || name.size() == 0)
            throw LowlevelError("Expecting size/delay/name attributes");
        res = new AddrSpace(this, trans, IPTR_PROCESSOR, name, bigEnd, addressSize, wordsize, index, flags, delay,
                            deadcodedelay);
    }

    return res;
} ///< Add a space parsed from a .sla file
    void decodeSlaSpaces(Decoder& decoder, const Translate* trans)
{
    // The first space should always be the constant space
    insertSpace(new ConstantSpace(this, trans));

    uint4 elemId = decoder.openElement(sla::ELEM_SPACES);
    string defname = decoder.readString(sla::ATTRIB_DEFAULTSPACE);
    while (decoder.peekElement() != 0) {
        AddrSpace* spc = decodeSlaSpace(decoder, trans);
        insertSpace(spc);
    }
    decoder.closeElement(elemId);
    AddrSpace* spc = getSpaceByName(defname);
    if (spc == (AddrSpace*)0)
        throw LowlevelError("Bad 'defaultspace' attribute: " + defname);
    setDefaultCodeSpace(spc->getIndex());
}      ///< Restore address spaces from a .sla file
    void decode(Decoder& decoder)
{
    maxdelayslotbytes = 0;
    unique_allocatemask = 0;
    numSections = 0;
    int4 version = 0;
    uint4 el = decoder.openElement(sla::ELEM_SLEIGH);
    uint4 attrib = decoder.getNextAttributeId();
    while (attrib != 0) {
        if (attrib == sla::ATTRIB_BIGENDIAN)
            setBigEndian(decoder.readBool());
        else if (attrib == sla::ATTRIB_ALIGN)
            alignment = decoder.readSignedInteger();
        else if (attrib == sla::ATTRIB_UNIQBASE)
            setUniqueBase(decoder.readUnsignedInteger());
        else if (attrib == sla::ATTRIB_MAXDELAY)
            maxdelayslotbytes = decoder.readUnsignedInteger();
        else if (attrib == sla::ATTRIB_UNIQMASK)
            unique_allocatemask = decoder.readUnsignedInteger();
        else if (attrib == sla::ATTRIB_NUMSECTIONS)
            numSections = decoder.readUnsignedInteger();
        else if (attrib == sla::ATTRIB_VERSION)
            version = decoder.readSignedInteger();
        attrib = decoder.getNextAttributeId();
    }
    if (version != sla::FORMAT_VERSION)
        throw LowlevelError(".sla file has wrong format");
    indexer.decode(decoder);
    decodeSlaSpaces(decoder, this);
    symtab.decode(decoder, this);
    decoder.closeElement(el);
    root = (SubtableSymbol*)symtab.getGlobalScope()->findSymbol("instruction");
    vector<string> errorPairs;
    buildXrefs(errorPairs);
    if (!errorPairs.empty())
        throw SleighError("Duplicate register pairs");
}                                       /// Decode a SELIGH specification from a stream
public:
    SleighBase(void)
{
    root = (SubtableSymbol*)0;
    maxdelayslotbytes = 0;
    unique_allocatemask = 0;
    numSections = 0;
} ///< Construct an uninitialized translator
    bool isInitialized(void) const {
        return (root != (SubtableSymbol*)0);
    } ///< Return \b true if \b this is initialized
    virtual ~SleighBase(void) {} ///< Destructor
    virtual const VarnodeData& getRegister(const string& nm) const
{
    VarnodeSymbol* sym = (VarnodeSymbol*)findSymbol(nm);
    if (sym == (VarnodeSymbol*)0)
        throw SleighError("Unknown register name: " + nm);
    if (sym->getType() != SleighSymbol::varnode_symbol)
        throw SleighError("Symbol is not a register: " + nm);
    return sym->getFixedVarnode();
}
    virtual string getRegisterName(AddrSpace* base, uintb off, int4 size) const
{
    VarnodeData sym;
    sym.space = base;
    sym.offset = off;
    sym.size = size;
    map<VarnodeData, string>::const_iterator iter = varnode_xref.upper_bound(sym); // First point greater than offset
    if (iter == varnode_xref.begin())
        return "";
    iter--;
    const VarnodeData& point((*iter).first);
    if (point.space != base)
        return "";
    uintb offbase = point.offset;
    if (point.offset + point.size >= off + size)
        return (*iter).second;

    while (iter != varnode_xref.begin()) {
        --iter;
        const VarnodeData& point((*iter).first);
        if ((point.space != base) || (point.offset != offbase))
            return "";
        if (point.offset + point.size >= off + size)
            return (*iter).second;
    }
    return "";
}
    virtual string getExactRegisterName(AddrSpace* base, uintb off, int4 size) const
{
    VarnodeData sym;
    sym.space = base;
    sym.offset = off;
    sym.size = size;
    map<VarnodeData, string>::const_iterator iter = varnode_xref.find(sym);
    if (iter == varnode_xref.end())
        return "";
    return (*iter).second;
}
    virtual void getAllRegisters(map<VarnodeData, string>& reglist) const
{
    reglist = varnode_xref;
}
    virtual void getUserOpNames(vector<string>& res) const
{
    res = userop; // Return list of all language defined user ops (with index)
}

    SleighSymbol* findSymbol(const string& nm) const {
        return symtab.findSymbol(nm);
    } ///< Find a specific SLEIGH symbol by name in the current scope
    SleighSymbol* findSymbol(uintm id) const {
        return symtab.findSymbol(id);
    } ///< Find a specific SLEIGH symbol by id
    SleighSymbol* findGlobalSymbol(const string& nm) const {
        return symtab.findGlobalSymbol(nm);
    } ///< Find a specific global SLEIGH symbol by name
    void encodeSlaSpace(Encoder& encoder, AddrSpace* spc) const
{
    if (spc->getType() == IPTR_INTERNAL)
        encoder.openElement(sla::ELEM_SPACE_UNIQUE);
    else if (spc->isOtherSpace())
        encoder.openElement(sla::ELEM_SPACE_OTHER);
    else
        encoder.openElement(sla::ELEM_SPACE);
    encoder.writeString(sla::ATTRIB_NAME, spc->getName());
    encoder.writeSignedInteger(sla::ATTRIB_INDEX, spc->getIndex());
    encoder.writeBool(sla::ATTRIB_BIGENDIAN, isBigEndian());
    encoder.writeSignedInteger(sla::ATTRIB_DELAY, spc->getDelay());
    //  if (spc->getDelay() != spc->getDeadcodeDelay())
    //    encoder.writeSignedInteger(sla::ATTRIB_DEADCODEDELAY, spc->getDeadcodeDelay());
    encoder.writeSignedInteger(sla::ATTRIB_SIZE, spc->getAddrSize());
    if (spc->getWordSize() > 1)
        encoder.writeSignedInteger(sla::ATTRIB_WORDSIZE, spc->getWordSize());
    encoder.writeBool(sla::ATTRIB_PHYSICAL, spc->hasPhysical());
    if (spc->getType() == IPTR_INTERNAL)
        encoder.closeElement(sla::ELEM_SPACE_UNIQUE);
    else if (spc->isOtherSpace())
        encoder.closeElement(sla::ELEM_SPACE_OTHER);
    else
        encoder.closeElement(sla::ELEM_SPACE);
} ///< Write the details of given space in .sla format
    void encode(Encoder& encoder) const
{
    encoder.openElement(sla::ELEM_SLEIGH);
    encoder.writeSignedInteger(sla::ATTRIB_VERSION, sla::FORMAT_VERSION);
    encoder.writeBool(sla::ATTRIB_BIGENDIAN, isBigEndian());
    encoder.writeSignedInteger(sla::ATTRIB_ALIGN, alignment);
    encoder.writeUnsignedInteger(sla::ATTRIB_UNIQBASE, getUniqueBase());
    if (maxdelayslotbytes > 0)
        encoder.writeUnsignedInteger(sla::ATTRIB_MAXDELAY, maxdelayslotbytes);
    if (unique_allocatemask != 0)
        encoder.writeUnsignedInteger(sla::ATTRIB_UNIQMASK, unique_allocatemask);
    if (numSections != 0)
        encoder.writeUnsignedInteger(sla::ATTRIB_NUMSECTIONS, numSections);
    encoder.openElement(sla::ELEM_SPACES);
    encoder.writeString(sla::ATTRIB_DEFAULTSPACE, getDefaultCodeSpace()->getName());
    for (int4 i = 0; i < numSpaces(); ++i) {
        AddrSpace* spc = getSpace(i);
        if (spc == (AddrSpace*)0)
            continue;
        if ((spc->getType() == IPTR_CONSTANT) || (spc->getType() == IPTR_FSPEC) || (spc->getType() == IPTR_IOP) ||
            (spc->getType() == IPTR_JOIN))
            continue;
        encodeSlaSpace(encoder, spc);
    }
    encoder.closeElement(sla::ELEM_SPACES);
    symtab.encode(encoder);
    encoder.closeElement(sla::ELEM_SLEIGH);
} ///< Write out the SLEIGH specification as a \<sleigh> tag.
};

/// Decodes an operand expression after the complete Sleigh symbol table is
/// available in the base implementation partition.
void operandValueDecode(OperandValue& value, Decoder& decoder, Translate* trans) {
    uint4 element = decoder.openElement(sla::ELEM_OPERAND_EXP);
    value.index = decoder.readSignedInteger(sla::ATTRIB_INDEX);
    uintm tableId = decoder.readUnsignedInteger(sla::ATTRIB_TABLE);
    uintm constructorId = decoder.readUnsignedInteger(sla::ATTRIB_CT);
    SleighBase* sleigh = static_cast<SleighBase*>(trans);
    SubtableSymbol* table = dynamic_cast<SubtableSymbol*>(sleigh->findSymbol(tableId));
    if (constructorId >= table->getNumConstructors())
        throw DecoderError("Invalid constructor id");
    value.ct = table->getConstructor(constructorId);
    decoder.closeElement(element);
}

/// Resolves a symbol id through the complete SleighBase type.
SleighSymbol* sleighBaseFindSymbol(SleighBase* base, uintm id) {
    return base->findSymbol(id);
}

/// Returns the constant space owned by a complete SleighBase instance.
AddrSpace* sleighBaseGetConstantSpace(SleighBase* base) {
    return base->getConstantSpace();
}

/// Adapts the SleighBase translator to the pattern-expression decoder.
PatternExpression* sleighBaseDecodeExpression(Decoder& decoder, SleighBase* base) {
    return PatternExpression::decodeExpression(decoder, static_cast<Translate*>(base));
}

} // End namespace ghidra
#endif
