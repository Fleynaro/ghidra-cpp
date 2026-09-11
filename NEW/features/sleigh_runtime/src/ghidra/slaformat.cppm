module;
#include <memory>

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
/// \file slaformat.cppm
/// \brief Encoding values for the SLA file format
#ifndef __SLAFORMAT__
#define __SLAFORMAT__

#include <memory>

export module sleigh_runtime.ghidra:slaformat;
export import :compression;
export import :marshal;

export namespace ghidra {
namespace sla {

extern const int4 FORMAT_SCOPE;   ///< Grouping elements/attributes for SLA file format
extern const int4 FORMAT_VERSION; ///< Current version of the .sla file

extern AttributeId ATTRIB_VAL;       ///< SLA format attribute "val"
extern AttributeId ATTRIB_ID;        ///< SLA format attribute "id"
extern AttributeId ATTRIB_SPACE;     ///< SLA format attribute "space"
extern AttributeId ATTRIB_S;         ///< SLA format attribute "s"
extern AttributeId ATTRIB_OFF;       ///< SLA format attribute "off"
extern AttributeId ATTRIB_CODE;      ///< SLA format attribute "code"
extern AttributeId ATTRIB_MASK;      ///< SLA format attribute "mask"
extern AttributeId ATTRIB_INDEX;     ///< SLA format attribute "index"
extern AttributeId ATTRIB_NONZERO;   ///< SLA format attribute "nonzero"
extern AttributeId ATTRIB_PIECE;     ///< SLA format attribute "piece"
extern AttributeId ATTRIB_NAME;      ///< SLA format attribute "name"
extern AttributeId ATTRIB_SCOPE;     ///< SLA format attribute "scope"
extern AttributeId ATTRIB_STARTBIT;  ///< SLA format attribute "startbit"
extern AttributeId ATTRIB_SIZE;      ///< SLA format attribute "size"
extern AttributeId ATTRIB_TABLE;     ///< SLA format attribute "table"
extern AttributeId ATTRIB_CT;        ///< SLA format attribute "ct"
extern AttributeId ATTRIB_MINLEN;    ///< SLA format attribute "minlen"
extern AttributeId ATTRIB_BASE;      ///< SLA format attribute "base"
extern AttributeId ATTRIB_NUMBER;    ///< SLA format attribute "number"
extern AttributeId ATTRIB_CONTEXT;   ///< SLA format attribute "context"
extern AttributeId ATTRIB_PARENT;    ///< SLA format attribute "parent"
extern AttributeId ATTRIB_SUBSYM;    ///< SLA format attribute "subsym"
extern AttributeId ATTRIB_LINE;      ///< SLA format attribute "line"
extern AttributeId ATTRIB_SOURCE;    ///< SLA format attribute "source"
extern AttributeId ATTRIB_LENGTH;    ///< SLA format attribute "length"
extern AttributeId ATTRIB_FIRST;     ///< SLA format attribute "first"
extern AttributeId ATTRIB_PLUS;      ///< SLA format attribute "plus"
extern AttributeId ATTRIB_SHIFT;     ///< SLA format attribute "shift"
extern AttributeId ATTRIB_ENDBIT;    ///< SLA format attribute "endbit"
extern AttributeId ATTRIB_SIGNBIT;   ///< SLA format attribute "signbit"
extern AttributeId ATTRIB_ENDBYTE;   ///< SLA format attribute "endbyte"
extern AttributeId ATTRIB_STARTBYTE; ///< SLA format attribute "startbyte"

extern AttributeId ATTRIB_VERSION;      ///< SLA format attribute "version"
extern AttributeId ATTRIB_BIGENDIAN;    ///< SLA format attribute "bigendian"
extern AttributeId ATTRIB_ALIGN;        ///< SLA format attribute "align"
extern AttributeId ATTRIB_UNIQBASE;     ///< SLA format attribute "uniqbase"
extern AttributeId ATTRIB_MAXDELAY;     ///< SLA format attribute "maxdelay"
extern AttributeId ATTRIB_UNIQMASK;     ///< SLA format attribute "uniqmask"
extern AttributeId ATTRIB_NUMSECTIONS;  ///< SLA format attribute "numsections"
extern AttributeId ATTRIB_DEFAULTSPACE; ///< SLA format attribute "defaultspace"
extern AttributeId ATTRIB_DELAY;        ///< SLA format attribute "delay"
extern AttributeId ATTRIB_WORDSIZE;     ///< SLA format attribute "wordsize"
extern AttributeId ATTRIB_PHYSICAL;     ///< SLA format attribute "physical"
extern AttributeId ATTRIB_SCOPESIZE;    ///< SLA format attribute "scopesize"
extern AttributeId ATTRIB_SYMBOLSIZE;   ///< SLA format attribute "symbolsize"
extern AttributeId ATTRIB_VARNODE;      ///< SLA format attribute "varnode"
extern AttributeId ATTRIB_LOW;          ///< SLA format attribute "low"
extern AttributeId ATTRIB_HIGH;         ///< SLA format attribute "high"
extern AttributeId ATTRIB_FLOW;         ///< SLA format attribute "flow"
extern AttributeId ATTRIB_CONTAIN;      ///< SLA format attribute "contain"
extern AttributeId ATTRIB_I;            ///< SLA format attribute "i"
extern AttributeId ATTRIB_NUMCT;        ///< SLA format attribute "numct"
extern AttributeId ATTRIB_SECTION;      ///< SLA format attribute "section"
extern AttributeId ATTRIB_LABELS;       ///< SLA format attribute "labels"

extern ElementId ELEM_CONST_REAL;       ///< SLA format element "const_real"
extern ElementId ELEM_VARNODE_TPL;      ///< SLA format element "varnode_tpl"
extern ElementId ELEM_CONST_SPACEID;    ///< SLA format element "const_spaceid"
extern ElementId ELEM_CONST_HANDLE;     ///< SLA format element "const_handle"
extern ElementId ELEM_OP_TPL;           ///< SLA format element "op_tpl"
extern ElementId ELEM_MASK_WORD;        ///< SLA format element "mask_word"
extern ElementId ELEM_PAT_BLOCK;        ///< SLA format element "pat_block"
extern ElementId ELEM_PRINT;            ///< SLA format element "print"
extern ElementId ELEM_PAIR;             ///< SLA format element "pair"
extern ElementId ELEM_CONTEXT_PAT;      ///< SLA format element "context_pat"
extern ElementId ELEM_NULL;             ///< SLA format element "null"
extern ElementId ELEM_OPERAND_EXP;      ///< SLA format element "operand_exp"
extern ElementId ELEM_OPERAND_SYM;      ///< SLA format element "operand_sym"
extern ElementId ELEM_OPERAND_SYM_HEAD; ///< SLA format element "operand_sym_head"
extern ElementId ELEM_OPER;             ///< SLA format element "oper"
extern ElementId ELEM_DECISION;         ///< SLA format element "decision"
extern ElementId ELEM_OPPRINT;          ///< SLA format element "opprint"
extern ElementId ELEM_INSTRUCT_PAT;     ///< SLA format element "instruct_pat"
extern ElementId ELEM_COMBINE_PAT;      ///< SLA format element "combine_pat"
extern ElementId ELEM_CONSTRUCTOR;      ///< SLA format element "constructor"
extern ElementId ELEM_CONSTRUCT_TPL;    ///< SLA format element "construct_tpl"
extern ElementId ELEM_SCOPE;            ///< SLA format element "scope"
extern ElementId ELEM_VARNODE_SYM;      ///< SLA format element "varnode_sym"
extern ElementId ELEM_VARNODE_SYM_HEAD; ///< SLA format element "varnode_sym_head"
extern ElementId ELEM_USEROP;           ///< SLA format element "userop"
extern ElementId ELEM_USEROP_HEAD;      ///< SLA format element "userop_head"
extern ElementId ELEM_TOKENFIELD;       ///< SLA format element "tokenfield"
extern ElementId ELEM_VAR;              ///< SLA format element "var"
extern ElementId ELEM_CONTEXTFIELD;     ///< SLA format element "contextfield"
extern ElementId ELEM_HANDLE_TPL;       ///< SLA format element "handle_tpl"
extern ElementId ELEM_CONST_RELATIVE;   ///< SLA format element "const_relative"
extern ElementId ELEM_CONTEXT_OP;       ///< SLA format element "context_op"

extern ElementId ELEM_SLEIGH;              ///< SLA format element "sleigh"
extern ElementId ELEM_SPACES;              ///< SLA format element "spaces"
extern ElementId ELEM_SOURCEFILES;         ///< SLA format element "sourcefiles"
extern ElementId ELEM_SOURCEFILE;          ///< SLA format element "sourcefile"
extern ElementId ELEM_SPACE;               ///< SLA format element "space"
extern ElementId ELEM_SYMBOL_TABLE;        ///< SLA format element "symbol_table"
extern ElementId ELEM_VALUE_SYM;           ///< SLA format element "value_sym"
extern ElementId ELEM_VALUE_SYM_HEAD;      ///< SLA format element "value_sym_head"
extern ElementId ELEM_CONTEXT_SYM;         ///< SLA format element "context_sym"
extern ElementId ELEM_CONTEXT_SYM_HEAD;    ///< SLA format element "context_sym_head"
extern ElementId ELEM_END_SYM;             ///< SLA format element "end_sym"
extern ElementId ELEM_END_SYM_HEAD;        ///< SLA format element "end_sym_head"
extern ElementId ELEM_SPACE_OTHER;         ///< SLA format element "space_other"
extern ElementId ELEM_SPACE_UNIQUE;        ///< SLA format element "space_unique"
extern ElementId ELEM_AND_EXP;             ///< SLA format element "and_exp"
extern ElementId ELEM_DIV_EXP;             ///< SLA format element "div_exp"
extern ElementId ELEM_LSHIFT_EXP;          ///< SLA format element "lshift_exp"
extern ElementId ELEM_MINUS_EXP;           ///< SLA format element "minus_exp"
extern ElementId ELEM_MULT_EXP;            ///< SLA format element "mult_exp"
extern ElementId ELEM_NOT_EXP;             ///< SLA format element "not_exp"
extern ElementId ELEM_OR_EXP;              ///< SLA format element "or_exp"
extern ElementId ELEM_PLUS_EXP;            ///< SLA format element "plus_exp"
extern ElementId ELEM_RSHIFT_EXP;          ///< SLA format element "rshift_exp"
extern ElementId ELEM_SUB_EXP;             ///< SLA format element "sub_exp"
extern ElementId ELEM_XOR_EXP;             ///< SLA format element "xor_exp"
extern ElementId ELEM_INTB;                ///< SLA format element "intb"
extern ElementId ELEM_END_EXP;             ///< SLA format element "end_exp"
extern ElementId ELEM_NEXT2_EXP;           ///< SLA format element "next2_exp"
extern ElementId ELEM_START_EXP;           ///< SLA format element "start_exp"
extern ElementId ELEM_EPSILON_SYM;         ///< SLA format element "epsilon_sym"
extern ElementId ELEM_EPSILON_SYM_HEAD;    ///< SLA format element "epsilon_sym_head"
extern ElementId ELEM_NAME_SYM;            ///< SLA format element "name_sym"
extern ElementId ELEM_NAME_SYM_HEAD;       ///< SLA format element "name_sym_head"
extern ElementId ELEM_NAMETAB;             ///< SLA format element "nametab"
extern ElementId ELEM_NEXT2_SYM;           ///< SLA format element "next2_sym"
extern ElementId ELEM_NEXT2_SYM_HEAD;      ///< SLA format element "next2_sym_head"
extern ElementId ELEM_START_SYM;           ///< SLA format element "start_sym"
extern ElementId ELEM_START_SYM_HEAD;      ///< SLA format element "start_sym_head"
extern ElementId ELEM_SUBTABLE_SYM;        ///< SLA format element "subtable_sym"
extern ElementId ELEM_SUBTABLE_SYM_HEAD;   ///< SLA format element "subtable_sym_head"
extern ElementId ELEM_VALUEMAP_SYM;        ///< SLA format element "valuemap_sym"
extern ElementId ELEM_VALUEMAP_SYM_HEAD;   ///< SLA format element "valuemap_sym_head"
extern ElementId ELEM_VALUETAB;            ///< SLA format element "valuetab"
extern ElementId ELEM_VARLIST_SYM;         ///< SLA format element "varlist_sym"
extern ElementId ELEM_VARLIST_SYM_HEAD;    ///< SLA format element "varlist_sym_head"
extern ElementId ELEM_OR_PAT;              ///< SLA format element "or_pat"
extern ElementId ELEM_COMMIT;              ///< SLA format element "commit"
extern ElementId ELEM_CONST_START;         ///< SLA format element "const_start"
extern ElementId ELEM_CONST_NEXT;          ///< SLA format element "const_next"
extern ElementId ELEM_CONST_NEXT2;         ///< SLA format element "const_next2"
extern ElementId ELEM_CONST_CURSPACE;      ///< SLA format element "curspace"
extern ElementId ELEM_CONST_CURSPACE_SIZE; ///< SLA format element "curspace_size"
extern ElementId ELEM_CONST_FLOWREF;       ///< SLA format element "const_flowref"
extern ElementId ELEM_CONST_FLOWREF_SIZE;  ///< SLA format element "const_flowref_size"
extern ElementId ELEM_CONST_FLOWDEST;      ///< SLA format element "const_flowdest"
extern ElementId ELEM_CONST_FLOWDEST_SIZE; ///< SLA format element "const_flowdest_size"

extern bool isSlaFormat(istream& s); ///< Verify a .sla file header at the current point of the given stream

/// \brief The decoder for the .sla file format
///
/// This verifies the .sla file header, does decompression, and decodes the raw data elements/attributes.
class FormatDecode : public PackedDecode {
    static const int4 IN_BUFFER_SIZE;  ///< The size of the \e input buffer
    std::unique_ptr<uint1[]> inBuffer; ///< Owned \e input buffer
public:
    FormatDecode(const AddrSpaceManager* spcManager)
        : PackedDecode(spcManager), inBuffer(std::make_unique<uint1[]>(IN_BUFFER_SIZE)) {} ///< Initialize the decoder
    virtual ~FormatDecode(void) = default;
    virtual void ingestStream(istream& s)
    {
        if (!isSlaFormat(s))
            throw LowlevelError("Missing SLA format header");
        Decompress decompressor;
        uint1* outBuf;
        int4 outAvail = 0;

        while (!decompressor.isFinished()) {
            s.read(reinterpret_cast<char*>(inBuffer.get()), IN_BUFFER_SIZE);
            int4 gcount = s.gcount();
            if (gcount == 0)
                throw LowlevelError("Unexpected end of compressed SLA stream");
            decompressor.input(inBuffer.get(), gcount);
            do {
                if (outAvail == 0) {
                    outBuf = allocateNextInputBuffer(0);
                    outAvail = BUFFER_SIZE;
                }
                outAvail = decompressor.inflate(outBuf + (BUFFER_SIZE - outAvail), outAvail);
            } while (outAvail == 0);
        }
        endIngest(BUFFER_SIZE - outAvail);
    }
};

const int4 FORMAT_SCOPE = 1;
const int4 FORMAT_VERSION = 4;

// ATTRIB_CONTEXT = 1 is reserved
AttributeId ATTRIB_VAL = AttributeId("val", 2, FORMAT_SCOPE);
AttributeId ATTRIB_ID = AttributeId("id", 3, FORMAT_SCOPE);
AttributeId ATTRIB_SPACE = AttributeId("space", 4, FORMAT_SCOPE);
AttributeId ATTRIB_S = AttributeId("s", 5, FORMAT_SCOPE);
AttributeId ATTRIB_OFF = AttributeId("off", 6, FORMAT_SCOPE);
AttributeId ATTRIB_CODE = AttributeId("code", 7, FORMAT_SCOPE);
AttributeId ATTRIB_MASK = AttributeId("mask", 8, FORMAT_SCOPE);
AttributeId ATTRIB_INDEX = AttributeId("index", 9, FORMAT_SCOPE);
AttributeId ATTRIB_NONZERO = AttributeId("nonzero", 10, FORMAT_SCOPE);
AttributeId ATTRIB_PIECE = AttributeId("piece", 11, FORMAT_SCOPE);
AttributeId ATTRIB_NAME = AttributeId("name", 12, FORMAT_SCOPE);
AttributeId ATTRIB_SCOPE = AttributeId("scope", 13, FORMAT_SCOPE);
AttributeId ATTRIB_STARTBIT = AttributeId("startbit", 14, FORMAT_SCOPE);
AttributeId ATTRIB_SIZE = AttributeId("size", 15, FORMAT_SCOPE);
AttributeId ATTRIB_TABLE = AttributeId("table", 16, FORMAT_SCOPE);
AttributeId ATTRIB_CT = AttributeId("ct", 17, FORMAT_SCOPE);
AttributeId ATTRIB_MINLEN = AttributeId("minlen", 18, FORMAT_SCOPE);
AttributeId ATTRIB_BASE = AttributeId("base", 19, FORMAT_SCOPE);
AttributeId ATTRIB_NUMBER = AttributeId("number", 20, FORMAT_SCOPE);
AttributeId ATTRIB_CONTEXT = AttributeId("context", 21, FORMAT_SCOPE);
AttributeId ATTRIB_PARENT = AttributeId("parent", 22, FORMAT_SCOPE);
AttributeId ATTRIB_SUBSYM = AttributeId("subsym", 23, FORMAT_SCOPE);
AttributeId ATTRIB_LINE = AttributeId("line", 24, FORMAT_SCOPE);
AttributeId ATTRIB_SOURCE = AttributeId("source", 25, FORMAT_SCOPE);
AttributeId ATTRIB_LENGTH = AttributeId("length", 26, FORMAT_SCOPE);
AttributeId ATTRIB_FIRST = AttributeId("first", 27, FORMAT_SCOPE);
AttributeId ATTRIB_PLUS = AttributeId("plus", 28, FORMAT_SCOPE);
AttributeId ATTRIB_SHIFT = AttributeId("shift", 29, FORMAT_SCOPE);
AttributeId ATTRIB_ENDBIT = AttributeId("endbit", 30, FORMAT_SCOPE);
AttributeId ATTRIB_SIGNBIT = AttributeId("signbit", 31, FORMAT_SCOPE);
AttributeId ATTRIB_ENDBYTE = AttributeId("endbyte", 32, FORMAT_SCOPE);
AttributeId ATTRIB_STARTBYTE = AttributeId("startbyte", 33, FORMAT_SCOPE);

AttributeId ATTRIB_VERSION = AttributeId("version", 34, FORMAT_SCOPE);
AttributeId ATTRIB_BIGENDIAN = AttributeId("bigendian", 35, FORMAT_SCOPE);
AttributeId ATTRIB_ALIGN = AttributeId("align", 36, FORMAT_SCOPE);
AttributeId ATTRIB_UNIQBASE = AttributeId("uniqbase", 37, FORMAT_SCOPE);
AttributeId ATTRIB_MAXDELAY = AttributeId("maxdelay", 38, FORMAT_SCOPE);
AttributeId ATTRIB_UNIQMASK = AttributeId("uniqmask", 39, FORMAT_SCOPE);
AttributeId ATTRIB_NUMSECTIONS = AttributeId("numsections", 40, FORMAT_SCOPE);
AttributeId ATTRIB_DEFAULTSPACE = AttributeId("defaultspace", 41, FORMAT_SCOPE);
AttributeId ATTRIB_DELAY = AttributeId("delay", 42, FORMAT_SCOPE);
AttributeId ATTRIB_WORDSIZE = AttributeId("wordsize", 43, FORMAT_SCOPE);
AttributeId ATTRIB_PHYSICAL = AttributeId("physical", 44, FORMAT_SCOPE);
AttributeId ATTRIB_SCOPESIZE = AttributeId("scopesize", 45, FORMAT_SCOPE);
AttributeId ATTRIB_SYMBOLSIZE = AttributeId("symbolsize", 46, FORMAT_SCOPE);
AttributeId ATTRIB_VARNODE = AttributeId("varnode", 47, FORMAT_SCOPE);
AttributeId ATTRIB_LOW = AttributeId("low", 48, FORMAT_SCOPE);
AttributeId ATTRIB_HIGH = AttributeId("high", 49, FORMAT_SCOPE);
AttributeId ATTRIB_FLOW = AttributeId("flow", 50, FORMAT_SCOPE);
AttributeId ATTRIB_CONTAIN = AttributeId("contain", 51, FORMAT_SCOPE);
AttributeId ATTRIB_I = AttributeId("i", 52, FORMAT_SCOPE);
AttributeId ATTRIB_NUMCT = AttributeId("numct", 53, FORMAT_SCOPE);
AttributeId ATTRIB_SECTION = AttributeId("section", 54, FORMAT_SCOPE);
AttributeId ATTRIB_LABELS = AttributeId("labels", 55, FORMAT_SCOPE);

ElementId ELEM_CONST_REAL = ElementId("const_real", 1, FORMAT_SCOPE);
ElementId ELEM_VARNODE_TPL = ElementId("varnode_tpl", 2, FORMAT_SCOPE);
ElementId ELEM_CONST_SPACEID = ElementId("const_spaceid", 3, FORMAT_SCOPE);
ElementId ELEM_CONST_HANDLE = ElementId("const_handle", 4, FORMAT_SCOPE);
ElementId ELEM_OP_TPL = ElementId("op_tpl", 5, FORMAT_SCOPE);
ElementId ELEM_MASK_WORD = ElementId("mask_word", 6, FORMAT_SCOPE);
ElementId ELEM_PAT_BLOCK = ElementId("pat_block", 7, FORMAT_SCOPE);
ElementId ELEM_PRINT = ElementId("print", 8, FORMAT_SCOPE);
ElementId ELEM_PAIR = ElementId("pair", 9, FORMAT_SCOPE);
ElementId ELEM_CONTEXT_PAT = ElementId("context_pat", 10, FORMAT_SCOPE);
ElementId ELEM_NULL = ElementId("null", 11, FORMAT_SCOPE);
ElementId ELEM_OPERAND_EXP = ElementId("operand_exp", 12, FORMAT_SCOPE);
ElementId ELEM_OPERAND_SYM = ElementId("operand_sym", 13, FORMAT_SCOPE);
ElementId ELEM_OPERAND_SYM_HEAD = ElementId("operand_sym_head", 14, FORMAT_SCOPE);
ElementId ELEM_OPER = ElementId("oper", 15, FORMAT_SCOPE);
ElementId ELEM_DECISION = ElementId("decision", 16, FORMAT_SCOPE);
ElementId ELEM_OPPRINT = ElementId("opprint", 17, FORMAT_SCOPE);
ElementId ELEM_INSTRUCT_PAT = ElementId("instruct_pat", 18, FORMAT_SCOPE);
ElementId ELEM_COMBINE_PAT = ElementId("combine_pat", 19, FORMAT_SCOPE);
ElementId ELEM_CONSTRUCTOR = ElementId("constructor", 20, FORMAT_SCOPE);
ElementId ELEM_CONSTRUCT_TPL = ElementId("construct_tpl", 21, FORMAT_SCOPE);
ElementId ELEM_SCOPE = ElementId("scope", 22, FORMAT_SCOPE);
ElementId ELEM_VARNODE_SYM = ElementId("varnode_sym", 23, FORMAT_SCOPE);
ElementId ELEM_VARNODE_SYM_HEAD = ElementId("varnode_sym_head", 24, FORMAT_SCOPE);
ElementId ELEM_USEROP = ElementId("userop", 25, FORMAT_SCOPE);
ElementId ELEM_USEROP_HEAD = ElementId("userop_head", 26, FORMAT_SCOPE);
ElementId ELEM_TOKENFIELD = ElementId("tokenfield", 27, FORMAT_SCOPE);
ElementId ELEM_VAR = ElementId("var", 28, FORMAT_SCOPE);
ElementId ELEM_CONTEXTFIELD = ElementId("contextfield", 29, FORMAT_SCOPE);
ElementId ELEM_HANDLE_TPL = ElementId("handle_tpl", 30, FORMAT_SCOPE);
ElementId ELEM_CONST_RELATIVE = ElementId("const_relative", 31, FORMAT_SCOPE);
ElementId ELEM_CONTEXT_OP = ElementId("context_op", 32, FORMAT_SCOPE);

ElementId ELEM_SLEIGH = ElementId("sleigh", 33, FORMAT_SCOPE);
ElementId ELEM_SPACES = ElementId("spaces", 34, FORMAT_SCOPE);
ElementId ELEM_SOURCEFILES = ElementId("sourcefiles", 35, FORMAT_SCOPE);
ElementId ELEM_SOURCEFILE = ElementId("sourcefile", 36, FORMAT_SCOPE);
ElementId ELEM_SPACE = ElementId("space", 37, FORMAT_SCOPE);
ElementId ELEM_SYMBOL_TABLE = ElementId("symbol_table", 38, FORMAT_SCOPE);
ElementId ELEM_VALUE_SYM = ElementId("value_sym", 39, FORMAT_SCOPE);
ElementId ELEM_VALUE_SYM_HEAD = ElementId("value_sym_head", 40, FORMAT_SCOPE);
ElementId ELEM_CONTEXT_SYM = ElementId("context_sym", 41, FORMAT_SCOPE);
ElementId ELEM_CONTEXT_SYM_HEAD = ElementId("context_sym_head", 42, FORMAT_SCOPE);
ElementId ELEM_END_SYM = ElementId("end_sym", 43, FORMAT_SCOPE);
ElementId ELEM_END_SYM_HEAD = ElementId("end_sym_head", 44, FORMAT_SCOPE);
ElementId ELEM_SPACE_OTHER = ElementId("space_other", 45, FORMAT_SCOPE);
ElementId ELEM_SPACE_UNIQUE = ElementId("space_unique", 46, FORMAT_SCOPE);
ElementId ELEM_AND_EXP = ElementId("and_exp", 47, FORMAT_SCOPE);
ElementId ELEM_DIV_EXP = ElementId("div_exp", 48, FORMAT_SCOPE);
ElementId ELEM_LSHIFT_EXP = ElementId("lshift_exp", 49, FORMAT_SCOPE);
ElementId ELEM_MINUS_EXP = ElementId("minus_exp", 50, FORMAT_SCOPE);
ElementId ELEM_MULT_EXP = ElementId("mult_exp", 51, FORMAT_SCOPE);
ElementId ELEM_NOT_EXP = ElementId("not_exp", 52, FORMAT_SCOPE);
ElementId ELEM_OR_EXP = ElementId("or_exp", 53, FORMAT_SCOPE);
ElementId ELEM_PLUS_EXP = ElementId("plus_exp", 54, FORMAT_SCOPE);
ElementId ELEM_RSHIFT_EXP = ElementId("rshift_exp", 55, FORMAT_SCOPE);
ElementId ELEM_SUB_EXP = ElementId("sub_exp", 56, FORMAT_SCOPE);
ElementId ELEM_XOR_EXP = ElementId("xor_exp", 57, FORMAT_SCOPE);
ElementId ELEM_INTB = ElementId("intb", 58, FORMAT_SCOPE);
ElementId ELEM_END_EXP = ElementId("end_exp", 59, FORMAT_SCOPE);
ElementId ELEM_NEXT2_EXP = ElementId("next2_exp", 60, FORMAT_SCOPE);
ElementId ELEM_START_EXP = ElementId("start_exp", 61, FORMAT_SCOPE);
ElementId ELEM_EPSILON_SYM = ElementId("epsilon_sym", 62, FORMAT_SCOPE);
ElementId ELEM_EPSILON_SYM_HEAD = ElementId("epsilon_sym_head", 63, FORMAT_SCOPE);
ElementId ELEM_NAME_SYM = ElementId("name_sym", 64, FORMAT_SCOPE);
ElementId ELEM_NAME_SYM_HEAD = ElementId("name_sym_head", 65, FORMAT_SCOPE);
ElementId ELEM_NAMETAB = ElementId("nametab", 66, FORMAT_SCOPE);
ElementId ELEM_NEXT2_SYM = ElementId("next2_sym", 67, FORMAT_SCOPE);
ElementId ELEM_NEXT2_SYM_HEAD = ElementId("next2_sym_head", 68, FORMAT_SCOPE);
ElementId ELEM_START_SYM = ElementId("start_sym", 69, FORMAT_SCOPE);
ElementId ELEM_START_SYM_HEAD = ElementId("start_sym_head", 70, FORMAT_SCOPE);
ElementId ELEM_SUBTABLE_SYM = ElementId("subtable_sym", 71, FORMAT_SCOPE);
ElementId ELEM_SUBTABLE_SYM_HEAD = ElementId("subtable_sym_head", 72, FORMAT_SCOPE);
ElementId ELEM_VALUEMAP_SYM = ElementId("valuemap_sym", 73, FORMAT_SCOPE);
ElementId ELEM_VALUEMAP_SYM_HEAD = ElementId("valuemap_sym_head", 74, FORMAT_SCOPE);
ElementId ELEM_VALUETAB = ElementId("valuetab", 75, FORMAT_SCOPE);
ElementId ELEM_VARLIST_SYM = ElementId("varlist_sym", 76, FORMAT_SCOPE);
ElementId ELEM_VARLIST_SYM_HEAD = ElementId("varlist_sym_head", 77, FORMAT_SCOPE);
ElementId ELEM_OR_PAT = ElementId("or_pat", 78, FORMAT_SCOPE);
ElementId ELEM_COMMIT = ElementId("commit", 79, FORMAT_SCOPE);
ElementId ELEM_CONST_START = ElementId("const_start", 80, FORMAT_SCOPE);
ElementId ELEM_CONST_NEXT = ElementId("const_next", 81, FORMAT_SCOPE);
ElementId ELEM_CONST_NEXT2 = ElementId("const_next2", 82, FORMAT_SCOPE);
ElementId ELEM_CONST_CURSPACE = ElementId("const_curspace", 83, FORMAT_SCOPE);
ElementId ELEM_CONST_CURSPACE_SIZE = ElementId("const_curspace_size", 84, FORMAT_SCOPE);
ElementId ELEM_CONST_FLOWREF = ElementId("const_flowref", 85, FORMAT_SCOPE);
ElementId ELEM_CONST_FLOWREF_SIZE = ElementId("const_flowref_size", 86, FORMAT_SCOPE);
ElementId ELEM_CONST_FLOWDEST = ElementId("const_flowdest", 87, FORMAT_SCOPE);
ElementId ELEM_CONST_FLOWDEST_SIZE = ElementId("const_flowdest_size", 88, FORMAT_SCOPE);

/// The bytes of the header are read from the stream and verified against the required form and current version.
/// If the form matches, \b true is returned.  No additional bytes are read.
/// \param s is the given stream
/// \return \b true if a valid header is present
bool isSlaFormat(istream& s)

{
    uint1 header[4];
    s.read((char*)header, 4);
    if (!s)
        return false;
    if (header[0] != 's' || header[1] != 'l' || header[2] != 'a')
        return false;
    if (header[3] != FORMAT_VERSION)
        return false;
    return true;
}

const int4 FormatDecode::IN_BUFFER_SIZE = 4096;
} // End namespace sla
} // End namespace ghidra

#endif
